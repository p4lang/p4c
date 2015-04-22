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

  BmEntryHandle bm_table_add_exact_match_entry(const std::string& table_name, const std::string& action_name, const BmMatchKey& match_key, const BmActionData& action_data) {
    printf("bm_table_add_exact_match_entry\n");
    entry_handle_t entry_handle;
    ByteContainer key;
    key.reserve(64);
    for(const std::string &d : match_key) {
      key.append(d.data(), d.size());
    }
    ActionData data;
    for(const std::string &d : action_data) {
      data.push_back_action_data(d.data(), d.size());
    }
    MatchTable::ErrorCode error_code = switch_->table_add_exact_match_entry(
        table_name, action_name,
	std::move(key),
	std::move(data),
	&entry_handle
    );
    if(error_code != MatchTable::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = (TableOperationErrorCode::type) error_code;
      throw ito;
    }
    return (BmEntryHandle) entry_handle;
  }

  BmEntryHandle bm_table_add_lpm_entry(const std::string& table_name, const std::string& action_name, const BmMatchKey& match_key, const int32_t prefix_length, const BmActionData& action_data) {
    printf("bm_table_add_lpm_entry\n");
    entry_handle_t entry_handle;
    ByteContainer key;
    key.reserve(64);
    for(const std::string &d : match_key) {
      key.append(d.data(), d.size());
    }
    ActionData data;
    for(const std::string &d : action_data) {
      data.push_back_action_data(d.data(), d.size());
    }
    MatchTable::ErrorCode error_code = switch_->table_add_lpm_entry(
        table_name, action_name,
	std::move(key),
	prefix_length,
	std::move(data),
	&entry_handle
    );
    if(error_code != MatchTable::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = (TableOperationErrorCode::type) error_code;
      throw ito;
    }
    return (BmEntryHandle) entry_handle;
  }

  BmEntryHandle bm_table_add_ternary_match_entry(const std::string& table_name, const std::string& action_name, const BmMatchKey& match_key, const BmMatchKey& match_mask, const int32_t priority, const BmActionData& action_data) {
    printf("bm_table_add_ternary_match_entry\n");
    entry_handle_t entry_handle;
    ByteContainer key;
    key.reserve(64);
    for(const std::string &d : match_key) {
      key.append(d.data(), d.size());
    }
    ByteContainer mask;
    mask.reserve(64);
    for(const std::string &d : match_mask) {
      mask.append(d.data(), d.size());
    }
    ActionData data;
    for(const std::string &d : action_data) {
      data.push_back_action_data(d.data(), d.size());
    }
    MatchTable::ErrorCode error_code = switch_->table_add_ternary_match_entry(
        table_name, action_name,
	std::move(key),
	std::move(mask),
	priority,
	std::move(data),
	&entry_handle
    );
    if(error_code != MatchTable::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = (TableOperationErrorCode::type) error_code;
      throw ito;
    }
    return (BmEntryHandle) entry_handle;
  }

  void bm_set_default_action(const std::string& table_name, const std::string& action_name, const BmActionData& action_data) {
    printf("bm_set_default_action\n");
    ActionData data;
    for(const std::string &d : action_data) {
      data.push_back_action_data(d.data(), d.size());
    }
    MatchTable::ErrorCode error_code = switch_->table_set_default_action(
        table_name,
	action_name,
	std::move(data)
    );
    if(error_code != MatchTable::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = (TableOperationErrorCode::type) error_code;
      throw ito;
    }
  }

  void bm_table_delete_entry(const std::string& table_name, const BmEntryHandle entry_handle) {
    printf("bm_table_delete_entry\n");
    MatchTable::ErrorCode error_code = switch_->table_delete_entry(
        table_name,
	entry_handle
    );
    if(error_code != MatchTable::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = (TableOperationErrorCode::type) error_code;
      throw ito;
    }
  }

  void bm_learning_ack(const BmLearningListId list_id, const BmLearningBufferId buffer_id, const std::vector<BmLearningSampleId> & sample_ids) {
    printf("bm_learning_ack\n");
    switch_->get_learn_engine()->ack(list_id, buffer_id, sample_ids);
  }

  void bm_learning_ack_buffer(const BmLearningListId list_id, const BmLearningBufferId buffer_id) {
    printf("bm_learning_ack_buffer\n");
    switch_->get_learn_engine()->ack_buffer(list_id, buffer_id);
  }

};

namespace bm_runtime {

static int serve(int port) {
  shared_ptr<RuntimeHandler> handler(new RuntimeHandler());
  shared_ptr<TProcessor> processor(new RuntimeProcessor(handler));
  shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

  TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
  server.serve();
  return 0;  
}

  int start_server(Switch *sw, int port) {
  switch_ = sw;
  std::thread server_thread(serve, port);
  printf("Thrift server was started\n");
  server_thread.detach();
  return 0;
}

}
