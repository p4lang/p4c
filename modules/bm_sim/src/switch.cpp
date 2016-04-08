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

#include <cassert>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <streambuf>

#include "bm_sim/switch.h"
#include "bm_sim/P4Objects.h"
#include "bm_sim/options_parse.h"
#include "bm_sim/logger.h"
#include "bm_sim/debugger.h"
#include "md5.h"

namespace bm {

static void
packet_handler(int port_num, const char *buffer, int len, void *cookie) {
  // static_cast<Switch *> if okay here because cookie was obtained by casting a
  // Switch * to void *
  static_cast<Switch *>(cookie)->receive(port_num, buffer, len);
}

// TODO(antonin): maybe a factory method would be more appropriate for Switch
SwitchWContexts::SwitchWContexts(size_t nb_cxts, bool enable_swap)
  : DevMgr(),
    nb_cxts(nb_cxts), contexts(nb_cxts), enable_swap(enable_swap),
    phv_source(std::move(PHVSourceIface::make_phv_source(nb_cxts))) {
  for (size_t i = 0; i < nb_cxts; i++) {
    contexts.at(i).set_cxt_id(i);
  }
}

LookupStructureFactory SwitchWContexts::default_lookup_factory {};

void
SwitchWContexts::add_required_field(const std::string &header_name,
                                  const std::string &field_name) {
  required_fields.insert(std::make_pair(header_name, field_name));
}

void
SwitchWContexts::force_arith_field(const std::string &header_name,
                                 const std::string &field_name) {
  arith_fields.insert(std::make_pair(header_name, field_name));
}

int
SwitchWContexts::init_objects(const std::string &json_path, int dev_id,
                              std::shared_ptr<TransportIface> transport) {
  std::ifstream fs(json_path, std::ios::in);
  if (!fs) {
    std::cout << "JSON input file " << json_path << " cannot be opened\n";
    return 1;
  }

  device_id = dev_id;

  if (!transport) {
    notifications_transport = std::shared_ptr<TransportIface>(
        TransportIface::make_dummy());
  } else {
    notifications_transport = std::move(transport);
  }

  for (size_t cxt_id = 0; cxt_id < nb_cxts; cxt_id++) {
    auto &cxt = contexts.at(cxt_id);
    cxt.set_device_id(device_id);
    cxt.set_notifications_transport(notifications_transport);
    int status = cxt.init_objects(&fs, get_lookup_factory(),
                                  required_fields, arith_fields);
    fs.clear();
    fs.seekg(0, std::ios::beg);
    if (status != 0) return status;
    phv_source->set_phv_factory(cxt_id, &cxt.get_phv_factory());
  }

  {
    std::unique_lock<std::mutex> config_lock(config_mutex);
    current_config = std::string((std::istreambuf_iterator<char>(fs)),
                                 std::istreambuf_iterator<char>());
  }

  return 0;
}

int
SwitchWContexts::init_from_command_line_options(int argc, char *argv[]) {
  OptionsParser parser;
  parser.parse(argc, argv);

  notifications_addr = parser.notifications_addr;
  auto transport = std::shared_ptr<TransportIface>(
      TransportIface::make_nanomsg(notifications_addr));
  transport->open();

#ifdef BMDEBUG_ON
  // has to be before init_objects because forces arith
  if (parser.debugger) {
    for (Context &c : contexts)
      c.set_force_arith(true);
    Debugger::init_debugger(parser.debugger_addr);
  }
#endif

  int status = init_objects(parser.config_file_path, parser.device_id,
                            transport);
  if (status != 0) return status;

  if (parser.console_logging)
    Logger::set_logger_console();

  if (parser.file_logger != "")
    Logger::set_logger_file(parser.file_logger);

  Logger::set_log_level(parser.log_level);

  if (parser.use_files)
    set_dev_mgr_files(parser.wait_time);
  else if (parser.packet_in)
    set_dev_mgr_packet_in(device_id, parser.packet_in_addr, transport);
  else
    set_dev_mgr_bmi(device_id, transport);

  for (const auto &iface : parser.ifaces) {
    std::cout << "Adding interface " << iface.second
              << " as port " << iface.first
              << (parser.use_files ? " (files)" : "")
              << std::endl;

    const char* inFileName = NULL;
    const char* outFileName = NULL;

    std::string inFile;
    std::string outFile;

    if (parser.use_files) {
      inFile = iface.second + "_in.pcap";
      inFileName = inFile.c_str();
      outFile = iface.second + "_out.pcap";
      outFileName = outFile.c_str();
    } else if (parser.pcap) {
      inFile = iface.second + ".pcap";
      inFileName = inFile.c_str();
      outFileName = inFileName;
    }

    port_add(iface.second, iface.first, inFileName, outFileName);
  }
  thrift_port = parser.thrift_port;

  // TODO(unknown): is this the right place to do this?
  set_packet_handler(packet_handler, static_cast<void *>(this));
  start();

  return status;
}

// TODO(antonin)
// I wonder if the correct thing to do would be to lock all contexts' mutex
// simultaneously for a swap. My first intuition is that it is not necessary. If
// a second load_new_config (for e.g.) happens before the first call terminates,
// we will return a ONGOING_SWAP error, since we always call
// Context::load_new_config in the same order. Still, need to think about it
// some more.

// for now, swap as to be done switch-wide, cannot be done on a per context
// basis, but this could easily be changed
RuntimeInterface::ErrorCode
SwitchWContexts::load_new_config(const std::string &new_config) {
  if (!enable_swap) return ErrorCode::CONFIG_SWAP_DISABLED;
  std::istringstream ss(new_config);
  for (auto &cxt : contexts) {
    ErrorCode rc = cxt.load_new_config(&ss, get_lookup_factory(),
                                       required_fields, arith_fields);
    if (rc != ErrorCode::SUCCESS) return rc;
    ss.clear();
    ss.seekg(0, std::ios::beg);
  }
  {
    std::unique_lock<std::mutex> config_lock(config_mutex);
    current_config = new_config;
  }
  return ErrorCode::SUCCESS;
}

RuntimeInterface::ErrorCode
SwitchWContexts::swap_configs() {
  if (!enable_swap) return ErrorCode::CONFIG_SWAP_DISABLED;
  for (auto &cxt : contexts) {
    ErrorCode rc = cxt.swap_configs();
    if (rc != ErrorCode::SUCCESS) return rc;
  }
  return ErrorCode::SUCCESS;
}

RuntimeInterface::ErrorCode
SwitchWContexts::reset_state() {
  for (auto &cxt : contexts) {
    ErrorCode rc = cxt.reset_state();
    if (rc != ErrorCode::SUCCESS) return rc;
  }
  return ErrorCode::SUCCESS;
}

int
SwitchWContexts::swap_requested() {
  for (auto &cxt : contexts) {
    if (cxt.swap_requested()) return true;
  }
  return false;
}

int
SwitchWContexts::do_swap() {
  int rc = 1;
  if (!enable_swap || !swap_requested()) return rc;
  boost::unique_lock<boost::shared_mutex> lock(ongoing_swap_mutex);
  for (size_t cxt_id = 0; cxt_id < nb_cxts; cxt_id++) {
    auto &cxt = contexts[cxt_id];
    if (!cxt.swap_requested()) continue;
    // TODO(antonin): we spin until no more packets exist for this context, is
    // there a better way of doing this?
    while (phv_source->phvs_in_use(cxt_id) > 0) { }
    int swap_done = cxt.do_swap();
    if (swap_done == 0)
      phv_source->set_phv_factory(cxt_id, &cxt.get_phv_factory());
    rc &= swap_done;
  }
  return rc;
}

std::string
SwitchWContexts::get_config() {
  std::unique_lock<std::mutex> config_lock(config_mutex);
  return current_config;
}

std::string
SwitchWContexts::get_config_md5() {
  std::unique_lock<std::mutex> config_lock(config_mutex);
  MD5_CTX cxt;
  MD5_Init(&cxt);
  MD5_Update(&cxt, current_config.data(), current_config.size());
  unsigned char md5[16];
  MD5_Final(md5, &cxt);
  return std::string(reinterpret_cast<char *>(md5), sizeof(md5));
}

// Switch convenience class

Switch::Switch(bool enable_swap)
    : SwitchWContexts(1u, enable_swap) { }

}  // namespace bm
