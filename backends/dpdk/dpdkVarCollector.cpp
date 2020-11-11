/*
Copyright 2020 Intel Corp.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

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
} // namespace DPDK
