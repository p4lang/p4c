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

#include <bm/config.h>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/processor/TMultiplexedProcessor.h>

namespace thrift_provider = apache::thrift;

#include <iostream>
#include <mutex>
#include <thread>

#include <bm/bm_sim/logger.h>
#include <bm/bm_sim/switch.h>
#include <bm/bm_sim/simple_pre.h>
#include <bm/bm_sim/simple_pre_lag.h>
#include <bm/thrift/stdcxx.h>
#include <bm/Standard.h>
#include <bm/SimplePre.h>
#include <bm/SimplePreLAG.h>

using namespace ::thrift_provider;
using namespace ::thrift_provider::protocol;
using namespace ::thrift_provider::transport;
using namespace ::thrift_provider::server;

using ::bm_runtime::standard::StandardProcessor;
using ::bm_runtime::simple_pre::SimplePreProcessor;
using ::bm_runtime::simple_pre_lag::SimplePreLAGProcessor;

using ::stdcxx::shared_ptr;

namespace bm_runtime {

namespace standard {
shared_ptr<StandardIf> get_handler(bm::SwitchWContexts *switch_);
}  // namespace standard

namespace simple_pre {
shared_ptr<SimplePreIf> get_handler(bm::SwitchWContexts *switch_);
}  // namespace simple_pre

namespace simple_pre_lag {
shared_ptr<SimplePreLAGIf> get_handler(bm::SwitchWContexts *switch_);
}  // namespace simple_pre_lag

bm::SwitchWContexts *switch_;
TMultiplexedProcessor *processor_;

namespace {

std::mutex m_ready;
std::condition_variable cv_ready;
bool ready = false;

template<typename T>
bool switch_has_component() {
  // TODO(antonin): do something more resilient (i.e. per context)
  return (switch_->get_cxt_component<T>(0) != nullptr);
}

int serve(int port) {
  shared_ptr<TMultiplexedProcessor> processor(new TMultiplexedProcessor());
  processor_ = processor.get();

  processor->registerProcessor(
      "standard",
      shared_ptr<TProcessor>(new StandardProcessor(
          standard::get_handler(switch_)))
  );

  if(switch_has_component<bm::McSimplePre>()) {
    processor->registerProcessor(
        "simple_pre",
        shared_ptr<TProcessor>(new SimplePreProcessor(
            simple_pre::get_handler(switch_)))
    );
  }

  if(switch_has_component<bm::McSimplePreLAG>()) {
    processor->registerProcessor(
        "simple_pre_lag",
        shared_ptr<TProcessor>(new SimplePreLAGProcessor(
            simple_pre_lag::get_handler(switch_)))
    );
  }

  shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

  {
    std::unique_lock<std::mutex> lock(m_ready);
    ready = true;
    cv_ready.notify_one();
  }

  TThreadedServer server(processor, serverTransport, transportFactory,
                         protocolFactory);
  try {
    server.serve();
  } catch (const transport::TTransportException &e) {
    std::cerr << "Thrift returned an exception when trying to bind to port "
              << port << "\n"
              << "The exception is: " << e.what() << "\n"
              << "You may have another process already using this port, maybe "
              << "another instance of bmv2.\n";
    std::exit(1);
  }
  return 0;  
}

}  // namespace

int start_server(bm::SwitchWContexts *sw, int port) {
  switch_ = sw;
  bm::Logger::get()->info("Starting Thrift server on port {}", port);
  std::thread server_thread(serve, port);
  std::unique_lock<std::mutex> lock(m_ready);
  while(!ready) cv_ready.wait(lock);
  server_thread.detach();
  bm::Logger::get()->info("Thrift server was started");
  return 0;
}

}  // namespace bm_runtime
