//:: # Generate the handler routines for the interfaces

//:: pd_prefix = "p4_pd_" + p4_prefix + "_"
//:: pd_static_prefix = "p4_pd_"
//:: api_prefix = p4_prefix + "_"
#ifdef P4THRIFT
#include <p4thrift/processor/TMultiplexedProcessor.h>

namespace thrift_provider = p4::thrift;
#else
#include <thrift/processor/TMultiplexedProcessor.h>

namespace thrift_provider = apache::thrift;
#endif

using namespace ::thrift_provider;

using boost::shared_ptr;

#include "p4_pd_rpc_server.ipp"

extern "C" {

// processor needs to be of type TMultiplexedProcessor,
// I am keeping a void * pointer for 
// now, in case this function is called from C code
int add_to_rpc_server(void *processor) {
  std::cerr << "Adding Thrift service for P4 program ${p4_prefix} to server\n";

  shared_ptr<${p4_prefix}Handler> ${p4_prefix}_handler(new ${p4_prefix}Handler());

  TMultiplexedProcessor *processor_ = (TMultiplexedProcessor *) processor;
  processor_->registerProcessor(
				"${p4_prefix}",
				shared_ptr<TProcessor>(new ${p4_prefix}Processor(${p4_prefix}_handler))
				);
  
  return 0;
}

}
