// Copyright 2020 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_DEBUG_WASM_GDB_SERVER_TARGET_H_
#define V8_DEBUG_WASM_GDB_SERVER_TARGET_H_

#include <atomic>
#include <map>
#include "src/base/macros.h"
#include "src/debug/wasm/gdb-server/gdb-remote-util.h"

namespace v8 {
namespace internal {
namespace wasm {
namespace gdb_server {

class GdbServer;
class Packet;
class Session;

// Class Target represents a debugging target. It contains the logic to decode
// incoming GDB-remote packets, execute them forwarding the debugger commands
// and queries to the Wasm engine, and send back GDB-remote packets.
class Target {
 public:
  // Contruct a Target object.
  explicit Target(GdbServer* gdb_server);

  // This function spin on a debugging session, until it closes.
  void Run(Session* ses);

  void Terminate();
  bool IsTerminated() const { return status_ == Status::Terminated; }

 private:
  // Initializes a map used to make fast lookups when handling query packets
  // that have a constant response.
  void InitQueryPropertyMap();

  // Blocks waiting for one of these two events to occur:
  // - A network packet arrives from the debugger, or the debugger connection is
  //   closed;
  // - The debuggee suspends execution because of a trap or breakpoint.
  void WaitForDebugEvent();

  // Processes GDB-remote packets that arrive from the debugger.
  // This method should be called when the debuggee has suspended its execution.
  void ProcessCommands();

  enum class ErrorCode { None = 0, BadFormat = 1, BadArgs = 2, Failed = 3 };

  enum class ProcessPacketResult {
    Paused,    // The command was processed, debuggee still paused.
    Continue,  // The debuggee should resume execution.
    Detach,    // Request to detach from the debugger.
    Kill       // Request to terminate the debuggee process.
  };
  // This function always succeedes, since all errors are reported as an error
  // string "Exx" where xx is a two digit number.
  // The return value indicates if the target can resume execution or it is
  // still paused.
  ProcessPacketResult ProcessPacket(Packet* pkt_in, Packet* pkt_out);

  // Processes a general query packet
  ErrorCode ProcessQueryPacket(const Packet* pkt_in, Packet* pkt_out);

  // Formats a 'Stop-reply' packet, which is sent in response of a 'c'
  // (continue), 's' (step) and '?' (query halt reason) commands.
  void SetStopReply(Packet* pkt_out) const;

  wasm_addr_t GetCurrentPc() const;

  GdbServer* gdb_server_;

  enum class Status { Running, Terminated };
  std::atomic<Status> status_;

  // Signal being processed.
  int8_t cur_signal_;

  Session* session_;  // Session object not owned by the Target.

  // Map used to make fast lookups when handling query packets.
  typedef std::map<std::string, std::string> QueryPropertyMap;
  QueryPropertyMap query_properties_;

  DISALLOW_COPY_AND_ASSIGN(Target);
};

}  // namespace gdb_server
}  // namespace wasm
}  // namespace internal
}  // namespace v8

#endif  // V8_DEBUG_WASM_GDB_SERVER_TARGET_H_
