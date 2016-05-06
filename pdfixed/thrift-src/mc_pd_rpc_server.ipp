#include "mc.h"

#include <iostream>

#include <bm/pdfixed/pd_static.h>
#include <bm/pdfixed/pd_pre.h>

using namespace  ::mc_pd_rpc;
using namespace  ::res_pd_rpc;

class mcHandler : virtual public mcIf {
public:
  mcHandler() {

  }

  int32_t mc_init(){
    std::cerr << "In mc_init\n";

    return 0;
  }

  SessionHandle_t mc_create_session(){
    std::cerr << "In mc_create_session\n";
    SessionHandle_t sess_hdl;
    p4_pd_mc_create_session((uint32_t*)&sess_hdl);
    return sess_hdl;
  }

  int32_t mc_destroy_session(const SessionHandle_t sess_hdl){
    std::cerr << "In mc_destroy_session\n";

    return p4_pd_mc_delete_session(sess_hdl);
  }

  int32_t mc_complete_operations(const SessionHandle_t sess_hdl){
    std::cerr << "In mc_complete_operations\n";

    return p4_pd_mc_complete_operations(sess_hdl);
  }

  McHandle_t mc_mgrp_create(const SessionHandle_t sess_hdl, const int dev, const int16_t mgid){
    std::cerr << "In mc_mgrp_create\n";

    p4_pd_entry_hdl_t grp_hdl;
    p4_pd_mc_mgrp_create(sess_hdl, dev, mgid, &grp_hdl);
    return grp_hdl;
  }

  int32_t mc_mgrp_destroy(const SessionHandle_t sess_hdl, const int dev, const McHandle_t grp_hdl){
    std::cerr << "In mc_mgrp_destroy\n";

    return p4_pd_mc_mgrp_destroy(sess_hdl, dev, grp_hdl);
  }

  McHandle_t mc_node_create(const SessionHandle_t sess_hdl, const int dev, const int16_t rid, const std::string &port_map, const std::string &lag_map){
    std::cerr << "In mc_node_create\n";

    p4_pd_entry_hdl_t l1_hdl;
    p4_pd_mc_node_create(sess_hdl, dev, rid, (uint8_t*)port_map.c_str(), (uint8_t*)lag_map.c_str(), &l1_hdl);
    return l1_hdl;
  }

  McHandle_t mc_node_update(const SessionHandle_t sess_hdl, const int dev, const McHandle_t node_hdl, const std::string &port_map, const std::string &lag_map){
    std::cerr << "In mc_node_update\n";

    return p4_pd_mc_node_update(sess_hdl, dev, node_hdl, (uint8_t*)port_map.c_str(), (uint8_t*)lag_map.c_str());
  }

  int32_t mc_node_destroy(const SessionHandle_t sess_hdl, const int dev, const McHandle_t node_hdl){
    std::cerr << "In mc_node_destroy\n";

    return p4_pd_mc_node_destroy(sess_hdl, dev, node_hdl);
  }

  int32_t mc_associate_node(const SessionHandle_t sess_hdl, const int dev, const McHandle_t grp_hdl, const McHandle_t l1_hdl){
    std::cerr << "In mc_associate_node\n";

    return p4_pd_mc_associate_node(sess_hdl, dev, grp_hdl, l1_hdl, 0, 0);
  }

  int32_t mc_dissociate_node(const SessionHandle_t sess_hdl, const int dev, const McHandle_t grp_hdl, const McHandle_t l1_hdl){
    std::cerr << "In mc_dissociate_node\n";

    return p4_pd_mc_dissociate_node(sess_hdl, dev, grp_hdl, l1_hdl);
  }

  int32_t mc_set_lag_membership(const SessionHandle_t sess_hdl, const int dev, const int8_t lag, const std::string &port_map){
    std::cerr << "In mc_set_lag_membership\n";

    int32_t y = p4_pd_mc_set_lag_membership(sess_hdl, dev, lag, (uint8_t*)port_map.c_str());
    return y;
  }

};
