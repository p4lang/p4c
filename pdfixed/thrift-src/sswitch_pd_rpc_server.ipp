#include "sswitch.h"

#include <iostream>

#include <bm/pdfixed/pd_mirroring.h>

using namespace  ::sswitch_pd_rpc;
using namespace  ::res_pd_rpc;

class sswitchHandler : virtual public sswitchIf {
public:
  sswitchHandler() {

  }

  // mirroring api

  int32_t mirroring_mapping_add(const int32_t mirror_id, const int32_t egress_port) {
      std::cerr << "In mirroring_mapping_add\n";
      return p4_pd_mirroring_mapping_add(mirror_id, egress_port);
  }

  int32_t mirroring_mapping_delete(const int32_t mirror_id) {
      std::cerr << "In mirroring_mapping_delete\n";
      return p4_pd_mirroring_mapping_delete(mirror_id);
  }

  int32_t mirroring_mapping_get_egress_port(int32_t mirror_id) {
      std::cerr << "In mirroring_mapping_get_egress_port\n";
      return p4_pd_mirroring_mapping_get_egress_port(mirror_id);
  }

};
