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

#ifndef BACKENDS_DPDK_PSA_SWITCH_MIDEND_H_
#define BACKENDS_DPDK_PSA_SWITCH_MIDEND_H_

#include "backends/bmv2/common/midend.h"
#include "backends/bmv2/common/options.h"
#include "frontends/common/options.h"
#include "ir/ir.h"
#include "midend/convertEnums.h"

namespace DPDK {

class PsaSwitchMidEnd : public BMV2::MidEnd {
public:
  // If p4c is run with option '--listMidendPasses', outStream is used for
  // printing passes names
  explicit PsaSwitchMidEnd(CompilerOptions &options,
                           std::ostream *outStream = nullptr);
};

} // namespace DPDK

#endif /* BACKENDS_DPDK_PSA_SWITCH_MIDEND_H_ */
