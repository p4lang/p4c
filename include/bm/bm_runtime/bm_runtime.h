#ifndef _BM_RUNTIME_BM_RUNTIME_H_
#define _BM_RUNTIME_BM_RUNTIME_H_

#include <thrift/processor/TMultiplexedProcessor.h>

#include <bm/config.h>
#include <bm/bm_sim/switch.h>
#include <bm/thrift/stdcxx.h>

using namespace thrift_provider;
using namespace thrift_provider::transport;
using namespace thrift_provider::protocol;

using ::stdcxx::shared_ptr;

namespace bm_runtime {

extern TMultiplexedProcessor *processor_;

template <typename Iface, typename Processor>
int add_service(const std::string &service_name,
                shared_ptr<Iface> handler) {
  processor_->registerProcessor(service_name,
				shared_ptr<TProcessor>(new Processor(handler)));
  return 0;
}

int start_server(bm::SwitchWContexts *sw, int port);

}

#endif  // _BM_RUNTIME_BM_RUNTIME_H_
