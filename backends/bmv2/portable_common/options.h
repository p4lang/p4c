/*
Copyright 2024 Marvell Technology, Inc.

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

#ifndef BACKENDS_BMV2_PORTABLE_COMMON_OPTIONS_H_
#define BACKENDS_BMV2_PORTABLE_COMMON_OPTIONS_H_

#include "backends/bmv2/common/options.h"
#include "backends/bmv2/portable_common/midend.h"

namespace BMV2 {

class PortableOptions : public BMV2Options {
 public:
    /// Process the command line arguments and set options accordingly.
    std::vector<const char *> *process(int argc, char *const argv[]) override;
};

}  // namespace BMV2

#endif /* BACKENDS_BMV2_PORTABLE_COMMON_OPTIONS_H_ */
