/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "blackboard.h"
#include "frame_buffer_watcher.h"
#include "jpeg_compressor.h"
#include "tcp_socket.h"
#include "virtual_inputs.h"
#include "vnc_client_connection.h"
#include "vnc_server.h"
#include "vnc_utils.h"

using cvd::vnc::VncServer;

VncServer::VncServer(int port, bool aggressive)
    : server_(port), frame_buffer_watcher_{&bb_}, aggressive_{aggressive} {}

void VncServer::MainLoop() {
  while (true) {
    auto connection = server_.Accept();
    StartClient(std::move(connection));
  }
}

void VncServer::StartClient(ClientSocket sock) {
  std::thread t(&VncServer::StartClientThread, this, std::move(sock));
  t.detach();
}

void VncServer::StartClientThread(ClientSocket sock) {
  // NOTE if VncServer is expected to be destroyed, we have a problem here.
  // All of the client threads will be pointing to the VncServer's
  // data members. In the current setup, if the VncServer is destroyed with
  // clients still running, the clients will all be left with dangling
  // pointers.
  VncClientConnection client(std::move(sock), &virtual_inputs_, &bb_,
                             aggressive_);
  client.StartSession();
}
