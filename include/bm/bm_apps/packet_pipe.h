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

#ifndef BM_BM_APPS_PACKET_PIPE_H_
#define BM_BM_APPS_PACKET_PIPE_H_

#include <string>
#include <thread>
#include <mutex>
#include <memory>

namespace bm_apps {

class PacketInjectImp;

class PacketInject {
 public:
  /* the library owns the memory, make a copy if you need before returning */
  typedef std::function<void(int port_num, const char *buffer, int len,
                             void *cookie)> PacketReceiveCb;

  explicit PacketInject(const std::string &addr);

  ~PacketInject();

  void start();

  void set_packet_receiver(const PacketReceiveCb &cb, void *cookie);

  void send(int port_num, const char *buffer, int len);

  // these 4 port_* functions are optional, depending on receiver configuration
  void port_add(int port_num);

  void port_remove(int port_num);

  void port_bring_up(int port_num);

  void port_bring_down(int port_num);

 private:
  // cannot use {nullptr} with pimpl
  std::unique_ptr<PacketInjectImp> pimp;
};

}  // namespace bm_apps

#endif  // BM_BM_APPS_PACKET_PIPE_H_
