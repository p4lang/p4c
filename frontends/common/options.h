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

/* -*-C++-*- */

#ifndef FRONTENDS_COMMON_OPTIONS_H_
#define FRONTENDS_COMMON_OPTIONS_H_

#include "parser_options.h"
// for p4::P4RuntimeFormat definition
#include "control-plane/p4RuntimeSerializer.h"


class CompilerOptions : public ParserOptions {
 public:
    CompilerOptions();

    // if true, skip frontend passes whose names are contained in passesToExcludeFrontend vector
    bool excludeFrontendPasses = false;
    bool listFrontendPasses = false;
    // strings matched against pass names that should be excluded from Frontend passes
    std::vector<cstring> passesToExcludeFrontend;
    // Target
    cstring target = nullptr;
    // Architecture
    cstring arch = nullptr;
    // Write P4Runtime control plane API description to the specified files.
    cstring p4RuntimeFiles = nullptr;
    // Write static table entries as a P4Runtime WriteRequest message to the specified files.
    cstring p4RuntimeEntriesFiles = nullptr;
    // Choose format for P4Runtime API description.
    P4::P4RuntimeFormat p4RuntimeFormat = P4::P4RuntimeFormat::BINARY;
};
#endif  /* FRONTENDS_COMMON_OPTIONS_H_ */
