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

#include "field_list.h"

#include <typeindex>
#include <typeinfo>

P4V1::FieldListConverter::FieldListConverter() {
    ExpressionConverter::addConverter(cstring(std::type_index(typeid(IR::FieldList)).name()),
                                      convertFieldList);
}

const IR::Node *P4V1::FieldListConverter::convertFieldList(const IR::Node *node) {
    auto fl = node->to<IR::FieldList>();
    BUG_CHECK(fl != nullptr, "Invalid node type %1%", node);

    cstring pragma_string = "field_list_field_slice"_cs;

    std::set<cstring> sliced_fields;
    std::map<cstring, std::pair<int, int>> field_slices;
    for (auto anno : fl->annotations->annotations) {
        if (anno->name == pragma_string) {
            if (anno->expr.size() != 3) error("Invalid pragma specification -- ", pragma_string);

            if (!anno->expr[0]->is<IR::StringLiteral>())
                error("Invalid field in pragma specification -- ", anno->expr[0]);

            auto field = anno->expr[0]->to<IR::StringLiteral>()->value;
            if (!anno->expr[1]->is<IR::Constant>() || !anno->expr[2]->is<IR::Constant>())
                error("Invalid slice bit position(s) in pragma specification -- ", pragma_string);

            if (sliced_fields.count(field)) error("Duplicate slice definition for field ", field);
            sliced_fields.insert(field);

            auto msb = anno->expr[1]->to<IR::Constant>()->asInt();
            auto lsb = anno->expr[2]->to<IR::Constant>()->asInt();

            field_slices.emplace(field, std::make_pair(msb, lsb));
        }
    }

    auto components = new IR::Vector<IR::Expression>();

    for (auto f : fl->fields) {
        if (!f->is<IR::Member>()) {
            components->push_back(f);  // slice can only be applied to IR::Member.
            continue;
        }

        BFN::PathLinearizer path;
        f->apply(path);
        cstring field_string = path.linearPath->to_cstring();

        IR::Slice *f_slice = nullptr;
        for (auto slice : field_slices) {
            if (field_string.endsWith(slice.first.string())) {
                auto hi = slice.second.first;
                auto lo = slice.second.second;

                if (lo > hi || hi > f->type->width_bits() || lo < 0)
                    error("Invalid field slice %1%[%2%:%3%]", f, hi, lo);

                f_slice = new IR::Slice(f, hi, lo);
                break;
            }
        }
        if (f_slice)
            components->push_back(f_slice);
        else
            components->push_back(f);
    }

    return new IR::ListExpression(fl->srcInfo, *components);
}

P4V1::FieldListConverter P4V1::FieldListConverter::singleton;
