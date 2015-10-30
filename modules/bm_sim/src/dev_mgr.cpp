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
#include <stdexcept>
#include <iostream>
#include <thread>

#define UNUSED(x) (void)(x)

#include "bm_sim/dev_mgr.h"

/////////////////////////////////////////////////////////////////////////////////////////////////

// These are private implementations

// Implementation that uses the BMI to send/receive packets
// from true interfaces
class BmiDevMgrImplementation
    : public DevMgrInterface
{
public:
  BmiDevMgrImplementation()
  {
     assert(!bmi_port_create_mgr(&port_mgr));
  }

  ~BmiDevMgrImplementation()
  {
     bmi_port_destroy_mgr(port_mgr);
  } 
    
  ReturnCode port_add(const std::string &iface_name, port_t port_num,
		      const char *in_pcap, const char*out_pcap)
  {
     assert(!bmi_port_interface_add(port_mgr, iface_name.c_str(), port_num, in_pcap, out_pcap));
     return ReturnCode::SUCCESS;
  }
    
  ReturnCode port_remove(port_t port_num)
  {
     assert(!bmi_port_interface_remove(port_mgr, port_num));
     return ReturnCode::SUCCESS;
  }

  void transmit_fn(int port_num, const char *buffer, int len)
  {
     bmi_port_send(port_mgr, port_num, buffer, len);
  }

  void start()
  {
     assert(port_mgr);
     int exitCode = bmi_start_mgr(port_mgr);
     if (exitCode != 0)
        bm_fatal_error("Could not start thread receiving packets");
  }

  ReturnCode set_packet_handler(PacketHandler handler, void* cookie)
  {
      typedef void function_t(int, const char *, int, void *);
      function_t** ptr_fun = handler.target<function_t *>();
      assert(ptr_fun);
      assert(*ptr_fun);
      assert(!bmi_set_packet_handler(port_mgr, *ptr_fun, cookie));
      return ReturnCode::SUCCESS;
  }
      
private:
  bmi_port_mgr_t *port_mgr{nullptr};
};

/////////////////////////////////////////////////////////////////////////////////////////////////

// Implementation which uses Pcap files to read/write packets
class FilesDevMgrImplementation
    : public DevMgrInterface
{
public:
  FilesDevMgrImplementation(bool respectTiming, unsigned wait_time_in_seconds)
      : reader(respectTiming, wait_time_in_seconds)
  {}

  ReturnCode port_add(const std::string &iface_name, port_t port_num,
		      const char *in_pcap, const char*out_pcap)
  {
     UNUSED(iface_name);
     reader.addFile(port_num, std::string(in_pcap));
     writer.addFile(port_num, std::string(out_pcap));
     return ReturnCode::SUCCESS;
  }
    
  ReturnCode port_remove(port_t port_num)
  {
     UNUSED(port_num);
     bm_fatal_error("Removing ports not implemented when reading pcap files");
     return ReturnCode::ERROR; // unreachable
  }
    
  void transmit_fn(int port_num, const char *buffer, int len)
  {
    writer.send_packet(port_num, buffer, len);
  }

  void start()
  {
    reader_thread = std::thread(&PcapFilesReader::start, &reader);
    reader_thread.detach();
  }
    
  ReturnCode set_packet_handler(PacketHandler handler, void* cookie)
  {
    reader.set_packet_handler(handler, cookie);
    return ReturnCode::SUCCESS;
  }
      
private:
  PcapFilesReader reader;
  PcapFilesWriter writer;
  std::thread reader_thread;
};

/////////////////////////////////////////////////////////////////////////////////////////////////

DevMgr::DevMgr()
    : impl(nullptr)
{}

void
DevMgr::start()
{
  assert(impl);
  impl->start();
}

void
DevMgr::setUseFiles(bool useFiles, unsigned wait_time_in_seconds)
{
  assert(!impl);
  if (useFiles)
  {
    impl = std::unique_ptr<DevMgrInterface>(
            new FilesDevMgrImplementation(
                                          false /* no real-time packet replay */,
                                          wait_time_in_seconds));
  }
  else
    impl = std::unique_ptr<DevMgrInterface>(new BmiDevMgrImplementation());
}

PacketDispatcherInterface::ReturnCode
DevMgr::port_add(const std::string &iface_name, port_t port_num,
		 const char *pcap_in, const char* pcap_out)
{
  // TODO: check if port is taken...
  assert(impl);
  return impl->port_add(iface_name, port_num, pcap_in, pcap_out);
}

void
DevMgr::transmit_fn(int port_num, const char *buffer, int len)
{
  assert(impl);
  impl->transmit_fn(port_num, buffer, len);
}

PacketDispatcherInterface::ReturnCode
DevMgr::port_remove(port_t port_num)
{
  assert(impl);
  return impl->port_remove(port_num);
}

PacketDispatcherInterface::ReturnCode
DevMgr::set_packet_handler(PacketHandler handler, void *cookie)
{
  assert(impl);
  return impl->set_packet_handler(handler, cookie);
}
