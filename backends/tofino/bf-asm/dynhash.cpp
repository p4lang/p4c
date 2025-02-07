/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fstream>
#include <iostream>
#include <string>

#include "bfas.h"
#include "backends/tofino/bf-asm/json.h"
#include "sections.h"

class DynHash : public Section {
    int lineno = -1;
    std::unique_ptr<json::obj> _dynhash = nullptr;
    std::string _dynhashFileName;

    DynHash() : Section("dynhash") {}

    void input(VECTOR(value_t) args, value_t data) {
        lineno = data.lineno;
        if (!CHECKTYPE(data, tSTR)) return;
        _dynhashFileName = data.s;
    }

    void process() {
        if (_dynhashFileName.empty()) return;
        std::ifstream inputFile(_dynhashFileName);
        if (!inputFile && _dynhashFileName[0] != '/')
            inputFile.open(asmfile_dir + "/" + _dynhashFileName);
        if (!inputFile) {
            warning(lineno, "%s: can't read file", _dynhashFileName.c_str());
        } else {
            inputFile >> _dynhash;
            if (!inputFile) {
                warning(lineno, "%s: not valid dynhash json representation",
                        _dynhashFileName.c_str());
                _dynhash.reset(new json::map());
            }
        }
    }

    void output(json::map &ctxtJson) {
        ctxtJson["dynamic_hash_calculations"] = json::vector();  // this key required by schema
        if (_dynhash) {
            ctxtJson.merge(_dynhash->to<json::map>());
        }
    }

    static DynHash singleton_dynhash;
} DynHash::singleton_dynhash;
