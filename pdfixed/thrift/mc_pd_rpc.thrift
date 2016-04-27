include "res.thrift"

namespace py mc_pd_rpc
namespace cpp mc_pd_rpc
namespace c_glib mc_pd_rpc

typedef i32 McHandle_t

service mc {
    # Multicast APIs.
    i32             mc_init ();
    res.SessionHandle_t mc_create_session    ();
    i32             mc_destroy_session   (1: res.SessionHandle_t sess_hdl);
    i32             mc_complete_operations(1: res.SessionHandle_t sess_hdl);
    McHandle_t      mc_mgrp_create    (1: res.SessionHandle_t sess_hdl, 2: i32 dev_id, 3: i16 mgid);
    i32             mc_mgrp_destroy   (1: res.SessionHandle_t sess_hdl, 2: i32 dev_id, 3: McHandle_t grp_hdl);
    McHandle_t      mc_node_create    (1: res.SessionHandle_t sess_hdl, 2: i32 dev_id, 3: i16 rid, 4: binary port_map, 5: binary lag_map);
    i32             mc_node_update    (1: res.SessionHandle_t sess_hdl, 2: i32 dev_id, 3: McHandle_t l1_hdl, 4: binary port_map, 5: binary lag_map);
    i32             mc_node_destroy   (1: res.SessionHandle_t sess_hdl, 2: i32 dev_id, 3: McHandle_t l1_hdl);
    i32             mc_associate_node (1: res.SessionHandle_t sess_hdl, 2: i32 dev_id, 3: McHandle_t grp_hdl, 4: McHandle_t l1_hdl);
    i32             mc_dissociate_node(1: res.SessionHandle_t sess_hdl, 2: i32 dev_id, 3: McHandle_t grp_hdl, 4: McHandle_t l1_hdl);
    i32             mc_set_lag_membership(1: res.SessionHandle_t sess_hdl, 2: i32 dev_id, 3: byte lag_index, 4: binary port_map);
}
