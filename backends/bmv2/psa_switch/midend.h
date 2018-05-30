/*
Copyright 2013-present Barefoot Networks, Inc.

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

#ifndef BACKENDS_BMV2_PSA_SWITCH_MIDEND_H_
#define BACKENDS_BMV2_PSA_SWITCH_MIDEND_H_

#include "ir/ir.h"
#include "frontends/common/options.h"
#include "midend/convertEnums.h"
#include "backends/bmv2/common/midend.h"

namespace BMV2 {

class PsaSwitchMidEnd : public MidEnd {
 public:
    explicit PsaSwitchMidEnd(CompilerOptions& options);
};

}  // namespace BMV2

#endif /* BACKENDS_BMV2_PSA_SWITCH_MIDEND_H_ */
