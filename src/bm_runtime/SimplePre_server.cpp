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

#include <bm/SimplePre.h>

#include <bm/bm_sim/switch.h>
#include <bm/bm_sim/simple_pre.h>
#include <bm/bm_sim/logger.h>

namespace bm_runtime { namespace simple_pre {

using namespace bm;

class SimplePreHandler : virtual public SimplePreIf {
public:
  SimplePreHandler(SwitchWContexts *sw) {
    for (size_t cxt_id = 0; cxt_id < sw->get_nb_cxts(); cxt_id++) {
      auto pre = sw->get_cxt_component<McSimplePre>(cxt_id);
      assert(pre != nullptr);
      pres.push_back(pre);
    }
  }

  BmMcMgrpHandle bm_mc_mgrp_create(const int32_t cxt_id, const BmMcMgrp mgrp) {
    Logger::get()->trace("bm_mc_mgrp_create");
    McSimplePre::mgrp_hdl_t mgrp_hdl;
    McSimplePre::McReturnCode error_code = pres.at(cxt_id)->mc_mgrp_create(
        mgrp, &mgrp_hdl);
    if(error_code != McSimplePre::SUCCESS) {
      InvalidMcOperation imo;
      imo.code = (McOperationErrorCode::type) error_code;
      throw imo;
    }
    return mgrp_hdl;
  }

  void bm_mc_mgrp_destroy(const int32_t cxt_id, const BmMcMgrpHandle mgrp_handle) {
    Logger::get()->trace("bm_mc_mgrp_destroy");
    McSimplePre::McReturnCode error_code = pres.at(cxt_id)->mc_mgrp_destroy(
        mgrp_handle);
    if(error_code != McSimplePre::SUCCESS) {
      InvalidMcOperation imo;
      imo.code = (McOperationErrorCode::type) error_code;
      throw imo;
    }
  }

  BmMcL1Handle bm_mc_node_create(const int32_t cxt_id, const BmMcRid rid, const BmMcPortMap& port_map) {
    Logger::get()->trace("bm_mc_node_create");
    McSimplePre::l1_hdl_t l1_hdl;
    McSimplePre::McReturnCode error_code = pres.at(cxt_id)->mc_node_create(
        rid, McSimplePre::PortMap(port_map), &l1_hdl);
    if(error_code != McSimplePre::SUCCESS) {
      InvalidMcOperation imo;
      imo.code = (McOperationErrorCode::type) error_code;
      throw imo;
    }
    return l1_hdl;
  }

  void bm_mc_node_associate(const int32_t cxt_id, const BmMcMgrpHandle mgrp_handle, const BmMcL1Handle l1_handle) {
    Logger::get()->trace("bm_mc_node_associate");
    McSimplePre::McReturnCode error_code = pres.at(cxt_id)->mc_node_associate(
        mgrp_handle, l1_handle);
    if(error_code != McSimplePre::SUCCESS) {
      InvalidMcOperation imo;
      imo.code = (McOperationErrorCode::type) error_code;
      throw imo;
    }
  }

  void bm_mc_node_dissociate(const int32_t cxt_id, const BmMcMgrpHandle mgrp_handle, const BmMcL1Handle l1_handle) {
    Logger::get()->trace("bm_mc_node_dissociate");
    McSimplePre::McReturnCode error_code = pres.at(cxt_id)->mc_node_dissociate(
        mgrp_handle, l1_handle);
    if(error_code != McSimplePre::SUCCESS) {
      InvalidMcOperation imo;
      imo.code = (McOperationErrorCode::type) error_code;
      throw imo;
    }
  }

  void bm_mc_node_destroy(const int32_t cxt_id, const BmMcL1Handle l1_handle) {
    Logger::get()->trace("bm_mc_node_destroy");
    McSimplePre::McReturnCode error_code = pres.at(cxt_id)->mc_node_destroy(
        l1_handle);
    if(error_code != McSimplePre::SUCCESS) {
      InvalidMcOperation imo;
      imo.code = (McOperationErrorCode::type) error_code;
      throw imo;
    }
  }

  void bm_mc_node_update(const int32_t cxt_id, const BmMcL1Handle l1_handle, const BmMcPortMap& port_map) {
    Logger::get()->trace("bm_mc_node_update");
    McSimplePre::McReturnCode error_code = pres.at(cxt_id)->mc_node_update(
        l1_handle, McSimplePre::PortMap(port_map));
    if(error_code != McSimplePre::SUCCESS) {
      InvalidMcOperation imo;
      imo.code = (McOperationErrorCode::type) error_code;
      throw imo;
    }
  }

  void bm_mc_get_entries(std::string& _return, const int32_t cxt_id) {
    Logger::get()->trace("bm_mc_get_entries");
    _return = pres.at(cxt_id)->mc_get_entries();
  }

private:
  std::vector<std::shared_ptr<McSimplePre> > pres{};
};

boost::shared_ptr<SimplePreIf> get_handler(SwitchWContexts *switch_) {
  return boost::shared_ptr<SimplePreHandler>(new SimplePreHandler(switch_));
}

}  // namespace simple_pre
}  // namespace bm_runtime
