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

typedef RuntimeInterface::mbr_hdl_t mbr_hdl_t;
typedef RuntimeInterface::grp_hdl_t grp_hdl_t;

Switch *switch_;

class RuntimeHandler : virtual public RuntimeIf {
 public:
  RuntimeHandler() {
    // Your initialization goes here
  }

  static TableOperationErrorCode::type get_exception_code(MatchErrorCode bm_code) {
    switch(bm_code) {
    case MatchErrorCode::TABLE_FULL:
      return TableOperationErrorCode::TABLE_FULL;
    case MatchErrorCode::INVALID_HANDLE:
      return TableOperationErrorCode::INVALID_HANDLE;
    case MatchErrorCode::COUNTERS_DISABLED:
      return TableOperationErrorCode::COUNTERS_DISABLED;
    case MatchErrorCode::INVALID_TABLE_NAME:
      return TableOperationErrorCode::INVALID_TABLE_NAME;
    case MatchErrorCode::INVALID_ACTION_NAME:
      return TableOperationErrorCode::INVALID_ACTION_NAME;
    case MatchErrorCode::WRONG_TABLE_TYPE:
      return TableOperationErrorCode::WRONG_TABLE_TYPE;
    case MatchErrorCode::INVALID_MBR_HANDLE:
      return TableOperationErrorCode::INVALID_MBR_HANDLE;
    case MatchErrorCode::MBR_STILL_USED:
      return TableOperationErrorCode::MBR_STILL_USED;
    case MatchErrorCode::MBR_ALREADY_IN_GRP:
      return TableOperationErrorCode::MBR_ALREADY_IN_GRP;
    case MatchErrorCode::MBR_NOT_IN_GRP:
      return TableOperationErrorCode::MBR_NOT_IN_GRP;
    case MatchErrorCode::INVALID_GRP_HANDLE:
      return TableOperationErrorCode::INVALID_GRP_HANDLE;
    case MatchErrorCode::GRP_STILL_USED:
      return TableOperationErrorCode::GRP_STILL_USED;
    case MatchErrorCode::EMPTY_GRP:
      return TableOperationErrorCode::EMPTY_GRP;
    case MatchErrorCode::ERROR:
      return TableOperationErrorCode::ERROR;
    default:
      assert(0 && "invalid error code");
    }
  }

  static void build_match_key(std::vector<MatchKeyParam> &params,
			      const BmMatchParams& match_key) {
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
  }

  BmEntryHandle bm_mt_add_entry(const std::string& table_name, const BmMatchParams& match_key, const std::string& action_name, const BmActionData& action_data, const BmAddEntryOptions& options) {
    printf("bm_table_add_entry\n");
    entry_handle_t entry_handle;
    std::vector<MatchKeyParam> params;
    build_match_key(params, match_key);
    ActionData data;
    for(const std::string &d : action_data) {
      data.push_back_action_data(d.data(), d.size());
    }
    MatchErrorCode error_code = switch_->mt_add_entry(
        table_name,
	params,
	action_name,
	std::move(data),
	&entry_handle,
	options.priority
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = get_exception_code(error_code);
      throw ito;
    }
    return entry_handle;
  }

  void bm_mt_set_default_action(const std::string& table_name, const std::string& action_name, const BmActionData& action_data) {
    printf("bm_set_default_action\n");
    ActionData data;
    for(const std::string &d : action_data) {
      data.push_back_action_data(d.data(), d.size());
    }
    MatchErrorCode error_code = switch_->mt_set_default_action(
        table_name,
	action_name,
	std::move(data)
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_delete_entry(const std::string& table_name, const BmEntryHandle entry_handle) {
    printf("bm_table_delete_entry\n");
    MatchErrorCode error_code = switch_->mt_delete_entry(
        table_name,
	entry_handle
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_modify_entry(const std::string& table_name, const BmEntryHandle entry_handle, const std::string &action_name, const BmActionData& action_data) {
    printf("bm_table_modify_entry\n");
    ActionData data;
    for(const std::string &d : action_data) {
      data.push_back_action_data(d.data(), d.size());
    }
    MatchErrorCode error_code = switch_->mt_modify_entry(
        table_name,
	entry_handle,
	action_name,
	data
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = get_exception_code(error_code);
      throw ito;
    }
  }

  BmMemberHandle bm_mt_indirect_add_member(const std::string& table_name, const std::string& action_name, const BmActionData& action_data) {
    printf("bm_mt_indirect_add_member\n");
    mbr_hdl_t mbr_handle;
    ActionData data;
    for(const std::string &d : action_data) {
      data.push_back_action_data(d.data(), d.size());
    }
    MatchErrorCode error_code = switch_->mt_indirect_add_member(
        table_name, action_name,
	std::move(data), &mbr_handle
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = get_exception_code(error_code);
      throw ito;
    }
    return mbr_handle;
  }

  void bm_mt_indirect_delete_member(const std::string& table_name, const BmMemberHandle mbr_handle) {
    printf("bm_mt_indirect_delete_member\n");
    MatchErrorCode error_code = switch_->mt_indirect_delete_member(
        table_name, mbr_handle
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_indirect_modify_member(const std::string& table_name, const BmMemberHandle mbr_handle, const std::string& action_name, const BmActionData& action_data) {
    printf("bm_mt_indirect_modify_member\n");
    ActionData data;
    for(const std::string &d : action_data) {
      data.push_back_action_data(d.data(), d.size());
    }
    MatchErrorCode error_code = switch_->mt_indirect_modify_member(
      table_name, mbr_handle, action_name, std::move(data)
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = get_exception_code(error_code);
      throw ito;
    }
  }

  BmEntryHandle bm_mt_indirect_add_entry(const std::string& table_name, const BmMatchParams& match_key, const BmMemberHandle mbr_handle, const BmAddEntryOptions& options) {
    printf("bm_mt_indirect_add_entry\n");
    entry_handle_t entry_handle;
    std::vector<MatchKeyParam> params;
    build_match_key(params, match_key);
    MatchErrorCode error_code = switch_->mt_indirect_add_entry(
      table_name, params, mbr_handle, &entry_handle, options.priority
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = get_exception_code(error_code);
      throw ito;
    }
    return entry_handle;
  }

  void bm_mt_indirect_modify_entry(const std::string& table_name, const BmEntryHandle entry_handle, const BmMemberHandle mbr_handle) {
    printf("bm_mt_indirect_modify_entry\n");
    MatchErrorCode error_code = switch_->mt_indirect_modify_entry(
      table_name, entry_handle, mbr_handle
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_indirect_delete_entry(const std::string& table_name, const BmEntryHandle entry_handle) {
    printf("bm_mt_indirect_delete_entry\n");
    MatchErrorCode error_code = switch_->mt_indirect_delete_entry(
      table_name, entry_handle
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_indirect_set_default_member(const std::string& table_name, const BmMemberHandle mbr_handle) {
    printf("bm_mt_indirect_set_default_member\n");
    MatchErrorCode error_code = switch_->mt_indirect_set_default_member(
      table_name, mbr_handle
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = get_exception_code(error_code);
      throw ito;
    }
  }

  BmGroupHandle bm_mt_indirect_ws_create_group(const std::string& table_name) {
    printf("bm_mt_indirect_ws_create_group\n");
    grp_hdl_t grp_handle;
    MatchErrorCode error_code = switch_->mt_indirect_ws_create_group(
      table_name, &grp_handle
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = get_exception_code(error_code);
      throw ito;
    }
    return grp_handle;
  }

  void bm_mt_indirect_ws_delete_group(const std::string& table_name, const BmGroupHandle grp_handle) {
    printf("bm_mt_indirect_ws_delete_group\n");
    MatchErrorCode error_code = switch_->mt_indirect_ws_delete_group(
      table_name, grp_handle
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_indirect_ws_add_member_to_group(const std::string& table_name, const BmMemberHandle mbr_handle, const BmGroupHandle grp_handle) {
    printf("bm_mt_indirect_ws_add_member_to_group\n");
    MatchErrorCode error_code = switch_->mt_indirect_ws_add_member_to_group(
      table_name, mbr_handle, grp_handle
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_indirect_ws_remove_member_from_group(const std::string& table_name, const BmMemberHandle mbr_handle, const BmGroupHandle grp_handle) {
    printf("bm_mt_indirect_ws_remove_member_from_group\n");
    MatchErrorCode error_code = switch_->mt_indirect_ws_remove_member_from_group(
      table_name, mbr_handle, grp_handle
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = get_exception_code(error_code);
      throw ito;
    }
  }

  BmEntryHandle bm_mt_indirect_ws_add_entry(const std::string& table_name, const BmMatchParams& match_key, const BmGroupHandle grp_handle, const BmAddEntryOptions& options) {
    printf("bm_mt_indirect_ws_add_entry\n");
    entry_handle_t entry_handle;
    std::vector<MatchKeyParam> params;
    build_match_key(params, match_key);
    MatchErrorCode error_code = switch_->mt_indirect_add_entry(
      table_name, params, grp_handle, &entry_handle, options.priority
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = get_exception_code(error_code);
      throw ito;
    }
    return entry_handle;
  }

  void bm_mt_indirect_ws_modify_entry(const std::string& table_name, const BmEntryHandle entry_handle, const BmGroupHandle grp_handle) {
    printf("bm_mt_indirect_ws_modify_entry\n");
    MatchErrorCode error_code = switch_->mt_indirect_ws_modify_entry(
      table_name, entry_handle, grp_handle
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_mt_indirect_ws_set_default_group(const std::string& table_name, const BmGroupHandle grp_handle) {
    printf("bm_mt_indirect_ws_set_default_group\n");
    MatchErrorCode error_code = switch_->mt_indirect_ws_set_default_group(
      table_name, grp_handle
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = get_exception_code(error_code);
      throw ito;
    }
  }

  void bm_table_read_counter(BmCounterValue& _return, const std::string& table_name, const BmEntryHandle entry_handle) {
    printf("bm_table_read_counter\n");
    MatchTable::counter_value_t bytes; // unsigned
    MatchTable::counter_value_t packets;
    MatchErrorCode error_code = switch_->table_read_counters(
        table_name,
	entry_handle,
	&bytes, &packets
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = get_exception_code(error_code);
      throw ito;
    }
    _return.bytes = (int64_t) bytes;
    _return.packets = (int64_t) packets;
  }

  void bm_table_reset_counters(const std::string& table_name) {
    printf("bm_table_reset_counters\n");
    MatchErrorCode error_code = switch_->table_reset_counters(
        table_name
    );
    if(error_code != MatchErrorCode::SUCCESS) {
      InvalidTableOperation ito;
      ito.what = get_exception_code(error_code);
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

  void bm_load_new_config(const std::string& config_str) {
    printf("bm_load_new_config\n");
    RuntimeInterface::ErrorCode error_code =
      switch_->load_new_config(config_str);
    if(error_code != RuntimeInterface::SUCCESS) {
      InvalidSwapOperation iso;
      iso.what = (SwapOperationErrorCode::type) error_code;
      throw iso;
    }
  }

  void bm_swap_configs() {
    printf("bm_swap_configs\n");
    RuntimeInterface::ErrorCode error_code = switch_->swap_configs();
    if(error_code != RuntimeInterface::SUCCESS) {
      InvalidSwapOperation iso;
      iso.what = (SwapOperationErrorCode::type) error_code;
      throw iso;
    }
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
