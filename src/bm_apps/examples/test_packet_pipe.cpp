/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#include <bm/bm_apps/packet_pipe.h>

#include <iostream>
#include <string>

void my_receiver(int port, const char *buffer, int len, void *cookie) {
  (void) buffer;
  (void) cookie;
  std::cout << "Received a packet of length " << len
            << " on port " << port << std::endl;
}

/* Frame (98 bytes) */
/* Ping request */
static unsigned char pkt78[98] = {
  0x00, 0xaa, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x04, /* ........ */
  0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x45, 0x00, /* ......E. */
  0x00, 0x54, 0x41, 0x55, 0x40, 0x00, 0x40, 0x01, /* .TAU@.@. */
  0xe4, 0x40, 0x0a, 0x00, 0x00, 0x0a, 0x0a, 0x00, /* .@...... */
  0x01, 0x0a, 0x08, 0x00, 0xe9, 0x7c, 0x07, 0xe4, /* .....|.. */
  0x00, 0x05, 0x35, 0x03, 0x06, 0x56, 0x00, 0x00, /* ..5..V.. */
  0x00, 0x00, 0x0c, 0x6e, 0x00, 0x00, 0x00, 0x00, /* ...n.... */
  0x00, 0x00, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, /* ........ */
  0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, /* ........ */
  0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, /* .. !"#$% */
  0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, /* &'()*+,- */
  0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, /* ./012345 */
  0x36, 0x37                                      /* 67 */
};

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cout << "Specify the IPC address to use\n";
    return 1;
  }

  std::string addr(argv[1]);
  bm_apps::PacketInject packet_inject(addr);
  packet_inject.set_packet_receiver(my_receiver, nullptr);
  packet_inject.start();  // starts the receiving thread
  int port = 1;
  std::cout << "Sending packet on port " << port << std::endl;
  packet_inject.send(port, reinterpret_cast<char *>(pkt78), sizeof(pkt78));
}
