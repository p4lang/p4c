/* Copyright 2018-present Barefoot Networks, Inc.
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

#include <bm/bm_sim/_assert.h>
#include <bm/bm_sim/logger.h>

#ifdef WITH_PI
#include <bm/PI/pi.h>
#endif

#include <bm/simple_switch/runner.h>

#ifdef WITH_PI
#include <PI/pi.h>
#include <PI/target/pi_imp.h>
#endif

#include "simple_switch.h"

namespace bm {

namespace sswitch {

/* static */
constexpr uint32_t SimpleSwitchRunner::default_drop_port;

SimpleSwitchRunner::SimpleSwitchRunner(uint32_t cpu_port, uint32_t drop_port)
    : cpu_port(cpu_port),
      simple_switch(new SimpleSwitch(true /* enable_swap */, drop_port)) { }

SimpleSwitchRunner::~SimpleSwitchRunner() = default;

int SimpleSwitchRunner::init_and_start(const bm::OptionsParser &parser) {
  int status = simple_switch->init_from_options_parser(
      parser, nullptr, nullptr);
  if (status != 0) return status;

#ifdef WITH_PI
  auto transmit_fn = [this](bm::DevMgrIface::port_t port_num,
                            packet_id_t pkt_id, const char *buf, int len) {
    (void)pkt_id;
    if (cpu_port > 0 && port_num == cpu_port) {
      BMLOG_DEBUG("Transmitting packet-in");
      auto status = pi_packetin_receive(simple_switch->get_device_id(),
                                        buf, static_cast<size_t>(len));
      if (status != PI_STATUS_SUCCESS)
        bm::Logger::get()->error("Error when transmitting packet-in");
    } else {
      simple_switch->transmit_fn(port_num, buf, len);
    }
  };
  simple_switch->set_transmit_fn(transmit_fn);

  bm::pi::register_switch(simple_switch.get(), cpu_port);
#endif

  simple_switch->start_and_return();

  return 0;
}

device_id_t SimpleSwitchRunner::get_device_id() const {
  return simple_switch->get_device_id();
}

DevMgr *SimpleSwitchRunner::get_dev_mgr() {
  return simple_switch.get();
}

}  // namespace sswitch

}  // namespace bm
