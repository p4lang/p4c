/* Copyright 2023 Intel Corporation

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

#ifndef BACKENDS_DPDK_TDICONF_H_
#define BACKENDS_DPDK_TDICONF_H_

#include <string>

#include "backends/dpdk/options.h"
namespace DPDK {

class TdiBfrtConf {
 public:
    /// Generate the TDI configuration file required by the tdi-pipeline-builder.
    static void generate(DPDK::DpdkOptions &options);
};

}  // namespace DPDK

#endif /* BACKENDS_DPDK_TDICONF_H_ */
