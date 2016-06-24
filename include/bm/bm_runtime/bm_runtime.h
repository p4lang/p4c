#ifndef _BM_RUNTIME_BM_RUNTIME_H_
#define _BM_RUNTIME_BM_RUNTIME_H_

#ifdef P4THRIFT
#include <p4thrift/processor/TMultiplexedProcessor.h>

namespace thrift_provider = p4::thrift;
#else
#include <thrift/processor/TMultiplexedProcessor.h>

namespace thrift_provider = apache::thrift;
#endif

#include <bm/bm_sim/switch.h>

using namespace thrift_provider;
using namespace thrift_provider::transport;
using namespace thrift_provider::protocol;

namespace bm_runtime {

extern TMultiplexedProcessor *processor_;

template <typename Iface, typename Processor>
int add_service(const std::string &service_name,
                boost::shared_ptr<Iface> handler) {
  processor_->registerProcessor(service_name,
				shared_ptr<TProcessor>(new Processor(handler)));
  return 0;
}

int start_server(bm::SwitchWContexts *sw, int port);

}

#endif
