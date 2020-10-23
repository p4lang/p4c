#include "dpdkVarCollector.h"

namespace DPDK {
cstring DpdkVariableCollector::get_next_tmp() {
  std::ostringstream out;
  out << "backend_tmp" << next_tmp_id;
  cstring tmp = out.str();
  next_tmp_id++;
  return tmp;
}

void DpdkVariableCollector::push_variable(const IR::DpdkDeclaration *d) {
  variables.push_back(d);
}
}  // namespace DPDK
