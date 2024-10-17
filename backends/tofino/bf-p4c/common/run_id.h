/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#ifndef _EXTENSIONS_BF_P4C_COMMON_RUN_ID_H_
#define _EXTENSIONS_BF_P4C_COMMON_RUN_ID_H_

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
    RunId(RunId const&)           = delete;
    void operator=(RunId const&)  = delete;
};

#endif  /* _EXTENSIONS_BF_P4C_COMMON_RUN_ID_H_ */
