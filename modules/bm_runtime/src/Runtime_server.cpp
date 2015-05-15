#include <thread>

#include "Runtime.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include <bm_sim/switch.h>
#include <bm_sim/pre.h>

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

  BmEntryHandle bm_table_add_entry(const std::string& table_name, const BmMatchParams& match_key, const std::string& action_name, const BmActionData& action_data, const BmAddEntryOptions& options) {
    printf("bm_table_add_entry\n");
    entry_handle_t entry_handle;
    std::vector<MatchKeyParam> params;
    params.reserve(match_key.size()); // the number of elements will be the same
    for(const BmMatchParam &bm_param : match_key) {
      switch(bm_param.type) {
      case BmMatchParamType::type::EXACT:
	params.emplace_back(MatchKeyParam::Type::EXACT,
			    bm_param.exact.key);
	break;
      case BmMatchParamType::type::LPM:
	params.emplace_back(MatchKeyParam::Type::LPM,
			    bm_param.lpm.key, bm_param.lpm.prefix_length);
	break;
      case BmMatchParamType::type::TERNARY:
	params.emplace_back(MatchKeyParam::Type::TERNARY,
			    bm_param.ternary.key, bm_param.ternary.mask);
	break;
      case BmMatchParamType::type::VALID:
	params.emplace_back(MatchKeyParam::Type::VALID,
			    bm_param.valid.key ? "\x01" : "\x00");
	break;
      default:
	assert(0 && "wrong type");
      }
    }
    ActionData data;
    for(const std::string &d : action_data) {
      data.push_back_action_data(d.data(), d.size());
    }
    MatchTable::ErrorCode error_code = switch_->table_add_entry(
        table_name,
	params,
	action_name,
	std::move(data),
	&entry_handle,
	options.priority
    );
    if(error_code != MatchTable::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = (TableOperationErrorCode::type) error_code;
      throw ito;
    }
    return entry_handle;
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

  void bm_table_modify_entry(const std::string& table_name, const BmEntryHandle entry_handle, const std::string &action_name, const BmActionData& action_data) {
    printf("bm_table_modify_entry\n");
    ActionData data;
    for(const std::string &d : action_data) {
      data.push_back_action_data(d.data(), d.size());
    }
    MatchTable::ErrorCode error_code = switch_->table_modify_entry(
        table_name,
	entry_handle,
	action_name,
	data
    );
    if(error_code != MatchTable::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = (TableOperationErrorCode::type) error_code;
      throw ito;
    }
  }

  void bm_table_read_counter(BmCounterValue& _return, const std::string& table_name, const BmEntryHandle entry_handle) {
    printf("bm_table_read_counter\n");
    MatchTable::counter_value_t bytes; // unsigned
    MatchTable::counter_value_t packets;
    MatchTable::ErrorCode error_code = switch_->table_read_counters(
        table_name,
	entry_handle,
	&bytes, &packets
    );
    if(error_code != MatchTable::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = (TableOperationErrorCode::type) error_code;
      throw ito;
    }
    _return.bytes = (int64_t) bytes;
    _return.packets = (int64_t) packets;
  }

  void bm_table_reset_counters(const std::string& table_name) {
    printf("bm_table_reset_counters\n");
    MatchTable::ErrorCode error_code = switch_->table_reset_counters(
        table_name
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

  BmMcMgrpHandle bm_mc_mgrp_create(const BmMcMgrp mgrp) {
    printf("bm_mc_mgrp_create\n");
    McPre::mgrp_hdl_t mgrp_hdl;
    McPre::McReturnCode error_code =
      switch_->get_pre()->mc_mgrp_create(mgrp, &mgrp_hdl);
    if(error_code != McPre::SUCCESS) {
      InvalidMcOperation imo;
      imo.what = (McOperationErrorCode::type) error_code;
      throw imo;
    }
    return mgrp_hdl;
  }

  void bm_mc_mgrp_destroy(const BmMcMgrpHandle mgrp_handle) {
    printf("bm_mc_mgrp_destroy\n");
    McPre::McReturnCode error_code =
      switch_->get_pre()->mc_mgrp_destroy(mgrp_handle);
    if(error_code != McPre::SUCCESS) {
      InvalidMcOperation imo;
      imo.what = (McOperationErrorCode::type) error_code;
      throw imo;
    }
  }

  BmMcL1Handle bm_mc_l1_node_create(const BmMcRid rid) {
    printf("bm_mc_l1_node_create\n");
    McPre::l1_hdl_t l1_hdl;
    McPre::McReturnCode error_code =
      switch_->get_pre()->mc_l1_node_create(rid, &l1_hdl);
    if(error_code != McPre::SUCCESS) {
      InvalidMcOperation imo;
      imo.what = (McOperationErrorCode::type) error_code;
      throw imo;
    }
    return l1_hdl;
  }

  void bm_mc_l1_node_associate(const BmMcMgrpHandle mgrp_handle, const BmMcL1Handle l1_handle) {
    printf("bm_mc_l1_node_associate\n");
    McPre::McReturnCode error_code =
      switch_->get_pre()->mc_l1_node_associate(mgrp_handle, l1_handle);
    if(error_code != McPre::SUCCESS) {
      InvalidMcOperation imo;
      imo.what = (McOperationErrorCode::type) error_code;
      throw imo;
    }
  }

  void bm_mc_l1_node_destroy(const BmMcL1Handle l1_handle) {
    printf("bm_mc_l1_node_destroy\n");
    McPre::McReturnCode error_code =
      switch_->get_pre()->mc_l1_node_destroy(l1_handle);
    if(error_code != McPre::SUCCESS) {
      InvalidMcOperation imo;
      imo.what = (McOperationErrorCode::type) error_code;
      throw imo;
    }
  }

  BmMcL2Handle bm_mc_l2_node_create(const BmMcL1Handle l1_handle, const BmMcPortMap& port_map) {
    printf("bm_mc_l2_node_create\n");
    McPre::l2_hdl_t l2_hdl;
    McPre::McReturnCode error_code = switch_->get_pre()->mc_l2_node_create(
        l1_handle, &l2_hdl, McPre::PortMap(port_map)
    );
    if(error_code != McPre::SUCCESS) {
      InvalidMcOperation imo;
      imo.what = (McOperationErrorCode::type) error_code;
      throw imo;
    }
    return l2_hdl;
  }

  void bm_mc_l2_node_update(const BmMcL2Handle l2_handle, const BmMcPortMap& port_map) {
    printf("bm_mc_l2_node_update\n");
    McPre::McReturnCode error_code = switch_->get_pre()->mc_l2_node_update(
        l2_handle, McPre::PortMap(port_map)
    );
    if(error_code != McPre::SUCCESS) {
      InvalidMcOperation imo;
      imo.what = (McOperationErrorCode::type) error_code;
      throw imo;
    }
  }

  void bm_mc_l2_node_destroy(const BmMcL2Handle l2_handle) {
    printf("bm_mc_l2_node_destroy\n");
    McPre::McReturnCode error_code =
      switch_->get_pre()->mc_l2_node_destroy(l2_handle);
    if(error_code != McPre::SUCCESS) {
      InvalidMcOperation imo;
      imo.what = (McOperationErrorCode::type) error_code;
      throw imo;
    }
  }

};

namespace bm_runtime {

static int serve(int port) {
  shared_ptr<RuntimeHandler> handler(new RuntimeHandler());
  shared_ptr<TProcessor> processor(new RuntimeProcessor(handler));
  shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

  TThreadedServer server(processor, serverTransport, transportFactory, protocolFactory);
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
