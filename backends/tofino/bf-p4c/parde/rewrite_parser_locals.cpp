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

#include "bf-p4c/parde/rewrite_parser_locals.h"

bool GetSelectFieldInfo::preorder(const IR::BFN::Select *select) {
    auto state = findContext<IR::BFN::ParserState>();
    auto save = select->source->to<IR::BFN::SavedRVal>();
    if (save) {
        auto field = phv.field(save->source);
        select_field_to_state[field].insert(state);
    }
    return true;
}

bool AddParserMatchDefs::preorder(IR::BFN::Extract *extract) {
    auto parser = findOrigCtxt<IR::BFN::Parser>();
    auto state = findOrigCtxt<IR::BFN::ParserState>();
    auto graph = parserInfo.graph(parser);
    auto dest_field = phv.field(extract->dest->field);
    if (getSelectField.select_field_to_state.count(dest_field) &&
        extract->dest->field->is<IR::TempVar>() && !uses.is_used_mau(dest_field)) {
        for (auto select_state : getSelectField.select_field_to_state.at(dest_field)) {
            // since select has to be at the end of a state it is safe to use a field defined in
            // the same state as it was extracted in
            if (state == select_state || graph.is_ancestor(state, select_state)) {
                auto match = new IR::BFN::MatchLVal(extract->dest->field);
                LOG3("Match Value added in " << extract << " in state " << state->name);
                extract->dest = match;
                break;
            }
        }
    }
    return true;
}

RewriteParserMatchDefs::RewriteParserMatchDefs(const PhvInfo &phv) {
    auto *parserInfo = new CollectParserInfo;
    auto *selectFieldInfo = new GetSelectFieldInfo(phv);
    auto *uses = new PhvUse(phv);
    addPasses({uses, new ResolveExtractSaves(phv), parserInfo, selectFieldInfo,
               new AddParserMatchDefs(phv, *parserInfo, *selectFieldInfo, *uses)});
}
