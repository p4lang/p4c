#ifndef _BM_RUNTIME_BM_RUNTIME_H_
#define _BM_RUNTIME_BM_RUNTIME_H_

#ifdef P4THRIFT
#include <p4thrift/processor/TMultiplexedProcessor.h>

namespace thrift_provider = p4::thrift;
#else
#include <thrift/processor/TMultiplexedProcessor.h>

namespace thrift_provider = apache::thrift;
#endif

#include <bm_sim/switch.h>

using namespace thrift_provider;
using namespace thrift_provider::transport;
using namespace thrift_provider::protocol;

namespace bm_runtime {

using boost::shared_ptr;

extern TMultiplexedProcessor *processor_;
extern bm::SwitchWContexts *switch_;

template <typename Handler, typename Processor, typename S>
int add_service(const std::string &service_name) {
  // TODO(antonin): static_cast too error prone here?
  shared_ptr<Handler> handler(new Handler(static_cast<S *>(switch_)));
  processor_->registerProcessor(service_name,
				shared_ptr<TProcessor>(new Processor(handler)));
  return 0;
}

int start_server(bm::SwitchWContexts *sw, int port);

}

#endif
