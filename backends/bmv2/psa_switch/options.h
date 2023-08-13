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

#ifndef BACKENDS_BMV2_PSA_SWITCH_OPTIONS_H_
#define BACKENDS_BMV2_PSA_SWITCH_OPTIONS_H_

#include <stdlib.h>

#include <vector>

#include "backends/bmv2/common/options.h"
#include "backends/bmv2/psa_switch/midend.h"
#include "frontends/common/parser_options.h"

namespace BMV2 {

class PsaSwitchOptions : public BMV2Options {
 public:
    PsaSwitchOptions() {
        registerOption(
            "--listMidendPasses", nullptr,
            [this](const char *) {
                listMidendPasses = true;
                loadIRFromJson = false;
                PsaSwitchMidEnd midEnd(*this, outStream);
                exit(0);
                return false;
            },
            "[PsaSwitch back-end] Lists exact name of all midend passes.\n");
    }

    /// Process the command line arguments and set options accordingly.
    std::vector<const char *> *process(int argc, char *const argv[]) override;
};

using PsaSwitchContext = P4CContextWithOptions<PsaSwitchOptions>;

}  // namespace BMV2

#endif /* BACKENDS_BMV2_PSA_SWITCH_OPTIONS_H_ */
