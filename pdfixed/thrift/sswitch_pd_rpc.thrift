include "res.thrift"

namespace py sswitch_pd_rpc
namespace cpp sswitch_pd_rpc
namespace c_glib sswitch_pd_rpc

service sswitch {
    # mirroring api

    i32 mirroring_mapping_add(1:i32 mirror_id, 2:i32 egress_port);
    i32 mirroring_mapping_delete(1:i32 mirror_id);
    i32 mirroring_mapping_get_egress_port(1:i32 mirror_id);
}
