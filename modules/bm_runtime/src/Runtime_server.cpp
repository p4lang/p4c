#include <thread>

#include "Runtime.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include <bm_sim/switch.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

using namespace  ::bm_runtime;

Switch *switch_;

class RuntimeHandler : virtual public RuntimeIf {
 public:
  RuntimeHandler() {
    // Your initialization goes here
  }

  BmEntryHandle bm_table_add_exact_match_entry(const std::string& table_name, const std::string& action_name, const std::string& key, const BmActionData& action_data) {
    // Your implementation goes here
    printf("bm_table_add_exact_match_entry\n");
    return 0;
  }

  BmEntryHandle bm_table_add_lpm_entry(const std::string& table_name, const std::string& action_name, const std::string& key, const int32_t prefix_length, const BmActionData& action_data) {
    // Your implementation goes here
    printf("bm_table_add_lpm_entry\n");
    return 0;
  }

  BmEntryHandle bm_table_add_ternary_match_entry(const std::string& table_name, const std::string& action_name, const std::string& key, const std::string& mask, const int32_t priority, const BmActionData& action_data) {
    // Your implementation goes here
    printf("bm_table_add_ternary_match_entry\n");
    return 0;
  }

  void bm_set_default_action(const std::string& table_name, const std::string& action_name, const BmActionData& action_data) {
    // Your implementation goes here
    printf("bm_set_default_action\n");
  }

  void bm_table_delete_entry(const std::string& table_name, const BmEntryHandle entry_handle) {
    // Your implementation goes here
    printf("bm_table_delete_entry\n");
  }

};

namespace bm_runtime {

static int serve() {
  int port = 9090;
  shared_ptr<RuntimeHandler> handler(new RuntimeHandler());
  shared_ptr<TProcessor> processor(new RuntimeProcessor(handler));
  shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

  TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
  server.serve();
  return 0;  
}

int start_server(Switch *sw) {
  switch_ = sw;
  std::thread server_thread(serve);
  printf("Thrift server was started\n");
  server_thread.detach();
  return 0;
}

}
