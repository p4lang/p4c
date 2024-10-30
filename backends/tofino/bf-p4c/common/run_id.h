/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _BACKENDS_TOFINO_BF_P4C_COMMON_RUN_ID_H_
#define _BACKENDS_TOFINO_BF_P4C_COMMON_RUN_ID_H_

#include <string>

class RunId {
 public:
    static const std::string &getId() {
        static RunId instance;
        return instance._runId;
    }

 private:
    std::string _runId;
    RunId();

 public:
    // disable any other constructors
    RunId(RunId const &) = delete;
    void operator=(RunId const &) = delete;
};

#endif /* _BACKENDS_TOFINO_BF_P4C_COMMON_RUN_ID_H_ */
