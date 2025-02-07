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

#include "backends/tofino/bf-asm/json.h"
#include "bfas.h"
#include "lib/log.h"
#include "sections.h"

class Primitives : public Section {
    int lineno = -1;
    std::unique_ptr<json::obj> _primitives = nullptr;
    std::string _primitivesFileName;

    Primitives() : Section("primitives") {}

    void input(VECTOR(value_t) args, value_t data) {
        lineno = data.lineno;
        if (!CHECKTYPE(data, tSTR)) return;
        _primitivesFileName = data.s;
    }

    void process() {
        if (_primitivesFileName.empty()) return;
        std::ifstream inputFile(_primitivesFileName);
        if (!inputFile && _primitivesFileName[0] != '/')
            inputFile.open(asmfile_dir + "/" + _primitivesFileName);
        if (!inputFile) {
            warning(lineno, "%s: can't read file", _primitivesFileName.c_str());
        } else {
            inputFile >> _primitives;
            if (!inputFile) {
                warning(lineno, "%s: not valid primitives json representation",
                        _primitivesFileName.c_str());
                _primitives.reset(new json::map());
            }
        }
    }

    bool merge_actions(json::vector &_prim_actions, json::vector &ctxt_actions) {
        bool merged = false;
        for (auto &_prim_action : _prim_actions) {
            for (auto &ctxt_action : ctxt_actions) {
                if (*ctxt_action->to<json::map>()["name"] ==
                    *_prim_action->to<json::map>()["name"]) {
                    ctxt_action->to<json::map>().merge(_prim_action->to<json::map>());
                    merged = true;
                    auto aname = ctxt_action->to<json::map>()["name"]->to<json::string>();
                    LOG3("Merged primitive action : " << aname);
                    break;
                }
            }
        }
        return merged;
    }

    // If primitives json is present this function will merge the primitives
    // nodes in the correct table->actions->action node The 'primitives' section
    // is run last so we have already populated the context json tables at this
    // stage. We check for the following tree structures to merge the action
    // nodes
    // Structure 1 (Match Tables)
    // tables
    //  |
    //  |--> table0
    //        |--> name
    //        |--> actions
    //              |
    //              |--> action0
    //                      |
    //                      |--> name
    //                      |--> primitives (merge here)
    // Structure 2 (ALPM Tables)
    // tables
    //  |
    //  |--> table0
    //        |--> name
    //        |--> match_attributes
    //              |
    //              |--> pre_classifier
    //                      |
    //                      |--> actions
    //                             |
    //                             |--> action0
    //                                   |
    //                                   |--> name
    //                                   |--> primitives (merge here)
    // We can have multiple tables with the same name but one without
    // and other with actions node e.g. stateful & its associated match table.
    // In this case we want to merge with match table since it has the actions
    // node
    void output(json::map &ctxtJson) {
        if (_primitives) {
            json::vector &prim_tables = _primitives->to<json::map>()["tables"];
            json::vector &ctxt_tables = ctxtJson["tables"];
            for (auto &prim_table : prim_tables) {
                json::string prim_table_name =
                    prim_table->to<json::map>()["name"]->to<json::string>();
                bool is_merged = false;
                json::string ctxt_table_name;
                for (auto &ctxt_table : ctxt_tables) {
                    ctxt_table_name = ctxt_table->to<json::map>()["name"]->to<json::string>();
                    if (prim_table_name == ctxt_table_name) {
                        if ((ctxt_table->to<json::map>().count("actions") > 0) &&
                            (prim_table->to<json::map>().count("actions") > 0)) {
                            json::vector &prim_table_actions =
                                prim_table->to<json::map>()["actions"];
                            json::vector &ctxt_table_actions =
                                ctxt_table->to<json::map>()["actions"];
                            is_merged = merge_actions(prim_table_actions, ctxt_table_actions);
                            break;
                        } else if ((ctxt_table->to<json::map>().count("match_attributes") > 0) &&
                                   (prim_table->to<json::map>().count("match_attributes") > 0)) {
                            json::map &prim_table_ma =
                                prim_table->to<json::map>()["match_attributes"];
                            json::map &ctxt_table_ma =
                                ctxt_table->to<json::map>()["match_attributes"];
                            if ((ctxt_table_ma.to<json::map>().count("pre_classifier") > 0) &&
                                (prim_table_ma.to<json::map>().count("pre_classifier") > 0)) {
                                json::map &prim_table_pc =
                                    prim_table_ma.to<json::map>()["pre_classifier"];
                                json::map &ctxt_table_pc =
                                    ctxt_table_ma.to<json::map>()["pre_classifier"];
                                if ((ctxt_table_pc.to<json::map>().count("actions") > 0) &&
                                    (prim_table_pc.to<json::map>().count("actions") > 0)) {
                                    json::vector &prim_table_actions =
                                        prim_table_pc.to<json::map>()["actions"];
                                    json::vector &ctxt_table_actions =
                                        ctxt_table_pc.to<json::map>()["actions"];
                                    LOG3("Merging primitive actions on table: " << prim_table_name);
                                    is_merged =
                                        merge_actions(prim_table_actions, ctxt_table_actions);
                                    break;
                                }
                            }
                        }
                    }
                }
                if (!is_merged) {
                    warning(lineno, "No table named %s found to merge primitive info",
                            prim_table_name.c_str());
                }
            }
        }
    }

    static Primitives singleton_primitives;
} Primitives::singleton_primitives;
