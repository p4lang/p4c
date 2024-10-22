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

#include "bf-p4c/mau/dynhash.h"

#include "bf-p4c/common/asm_output.h"
#include "bf-p4c/common/utils.h"
#include "bf-p4c/mau/ixbar_expr.h"
#include "bf-p4c/mau/resource.h"
#include "bf-p4c/mau/tofino/input_xbar.h"
#include "lib/stringify.h"

/**
 * Guarantees that each dynamic hash function is the same across all calls of this dynamic
 * hash function.  This way the JSON node will be consistent
 */
bool VerifyUniqueDynamicHash::preorder(const IR::MAU::HashGenExpression *hge) {
    if (!hge->dynamic) return false;
    auto key = hge->id.name;
    if (verify_hash_gen.count(key) == 0) {
        verify_hash_gen[key] = hge;
        return false;
    }

    auto hge_comp = verify_hash_gen.at(key);
    if (!hge->equiv(*hge_comp)) {
        ::fatal_error(
            "%1% : Hash.get over dynamic hash %2% differ: %3%.  Dynamic hashes must "
            "have the same field list and sets of algorithm for each get call, as these must "
            "change simultaneously at runtime",
            hge, key, hge_comp);
    }
    return false;
}

/**
 * Gather all tables that have allocated this particular dynamic hash object
 */
bool GatherDynamicHashAlloc::preorder(const IR::MAU::Table *tbl) {
    if (!tbl->is_placed()) return true;
    for (auto &hd_use : tbl->resources->hash_dists) {
        for (auto &ir_alloc : hd_use.ir_allocations) {
            if (!ir_alloc.is_dynamic()) continue;
            auto key = ir_alloc.dyn_hash_name;
            BUG_CHECK(verify_hash_gen.count(key),
                      "Cannot associate the hash allocation for "
                      "dyn hash key %s",
                      key);
            HashFuncLoc hfl(tbl->stage(), hd_use.hash_group());
            auto value = std::make_pair(hfl, &ir_alloc);
            hash_gen_alloc[key].emplace_back(value);
        }
    }
    return true;
}

namespace BFN {

unsigned fieldListHandle = 0x0;
unsigned dynHashHandle = 0x0;
unsigned algoHandle = 0x0;

bool GenerateDynamicHashJson::preorder(const IR::MAU::Table *tbl) {
    all_placed &= tbl->is_placed();
    if (tbl->conditional_gateway_only()) return true;
    if (tbl->is_always_run_action()) return true;
    if (auto res = tbl->resources) {
        Util::JsonObject *_dynHashCalc = new Util::JsonObject();
        if (auto match_table = tbl->match_table->to<IR::P4Table>()) {
            cstring fieldListCalcName = ""_cs;
            cstring fieldListName = ""_cs;
            IR::NameList algorithms;
            int hash_bit_width = -1;
            LOG5("Annotations : " << match_table->annotations);
            for (auto annot : match_table->annotations->annotations) {
                if (annot->name == "action_selector_hash_field_calc_name")
                    fieldListCalcName = annot->expr[0]->to<IR::StringLiteral>()->value;
                else if (annot->name == "action_selector_hash_field_list_name")
                    fieldListName = annot->expr[0]->to<IR::StringLiteral>()->value;
                else if (annot->name == "action_selector_hash_field_calc_output_width")
                    hash_bit_width = annot->expr[0]->to<IR::Constant>()->asInt();
                else if (annot->name == "algorithm")
                    algorithms.names.push_back(annot->expr[0]->to<IR::StringLiteral>()->value);
            }
            // If none of the above values are populated dont proceed.
            LOG5("fieldListCalcName: " << fieldListCalcName << " fieldListName: " << fieldListName
                                       << " hash_bit_width: " << hash_bit_width
                                       << "Algo size: " << algorithms.names.size());
            if (!fieldListCalcName.isNullOrEmpty() && !fieldListName.isNullOrEmpty() &&
                (hash_bit_width > 0) && (algorithms.names.size() > 0)) {
                _dynHashCalc->emplace("name"_cs, fieldListCalcName);
                _dynHashCalc->emplace("handle"_cs, dynHashHandleBase + dynHashHandle++);
                LOG4("Generating dynamic hash schema for " << fieldListCalcName
                                                           << " and field list " << fieldListName);
                gen_ixbar_json(res->selector_ixbar.get(), _dynHashCalc, tbl->stage(), fieldListName,
                               &algorithms, hash_bit_width);
                _dynHashNode->append(_dynHashCalc);
            }
        }
    }
    return true;
}

void GenerateDynamicHashJson::gen_hash_dist_json(cstring dyn_hash_name) {
    auto hge = verify_hash_gen.at(dyn_hash_name);

    auto pos = hash_gen_alloc.find(dyn_hash_name);
    if (pos == hash_gen_alloc.end()) {
#if 0
        // FIXME -- HashGenExpressions used directly in SaluActions have no hash_dist
        // associated with them, so we currently can't generate json for them.  For
        // now just leave them out -- runtime will not be able to change the hash function
        BUG_CHECK(!all_placed, "No allocation for dyn hash object %s to coordinate against",
                dyn_hash_name);
#endif
        return;
    }

    auto &hga_value = hash_gen_alloc.at(dyn_hash_name);
    AllocToHashUse alloc_to_use;
    for (auto &entry : hga_value) {
        alloc_to_use[entry.first].push_back(entry.second);
    }

    Util::JsonObject *_dynHashCalc = new Util::JsonObject();
    Util::JsonObject *_fieldList = new Util::JsonObject();
    Util::JsonArray *_xbar_cfgs = new Util::JsonArray();
    Util::JsonArray *_hash_cfgs = new Util::JsonArray();

    _dynHashCalc->emplace("name"_cs, dyn_hash_name);
    _dynHashCalc->emplace("handle"_cs, dynHashHandleBase + dynHashHandle++);

    gen_algo_json(_dynHashCalc, hge);

    int hash_bit_width = hge->hash_output_width;
    std::map<cstring, cstring> fieldNames;
    gen_field_list_json(_fieldList, hge, fieldNames);

    for (auto func_to_alloc : alloc_to_use) {
        Tofino::IXBar::Use combined_use;
        const HashFuncLoc &hfl = func_to_alloc.first;
        for (auto &alloc : func_to_alloc.second) {
            BUG_CHECK(alloc->use, "no IXBar::Use");
            for (auto &b : alloc->use->use) {
                bool repeat_byte = false;
                for (auto &c_b : combined_use.use) {
                    if (b.loc == c_b.loc) {
                        repeat_byte = true;
                        for (auto fi : b.field_bytes) c_b.add_info(fi);
                        break;
                    }
                }
                if (!repeat_byte) combined_use.use.push_back(b);
            }
            auto &hdh = combined_use.hash_dist_hash;
            auto &local_hdh = dynamic_cast<Tofino::IXBar::Use &>(*alloc->use).hash_dist_hash;
            hdh.allocated = true;
            hdh.group = hfl.hash_group;
            hdh.galois_start_bit_to_p4_hash.insert(local_hdh.galois_start_bit_to_p4_hash.begin(),
                                                   local_hdh.galois_start_bit_to_p4_hash.end());
        }
        gen_ixbar_bytes_json(_xbar_cfgs, hfl.stage, fieldNames, combined_use);
        gen_hash_json(_hash_cfgs, hfl.stage, combined_use, hash_bit_width);
        combined_use.clear();
    }

    _fieldList->emplace("crossbar_configuration"_cs, _xbar_cfgs);
    Util::JsonArray *_field_lists = new Util::JsonArray();
    _field_lists->append(_fieldList);
    _dynHashCalc->emplace("field_lists"_cs, _field_lists);
    _dynHashCalc->emplace("hash_configuration"_cs, _hash_cfgs);
    _dynHashCalc->emplace("hash_bit_width"_cs, hash_bit_width);
    _dynHashNode->append(_dynHashCalc);
}

void GenerateDynamicHashJson::end_apply() {
    for (auto &entry : verify_hash_gen) {
        gen_hash_dist_json(entry.first);
    }
}

/**
 * FIXME: All of this code needs to be changed between now and when the dynamic hashing
 * infrastructure is correctly brought to the backend.  But essentially for every
 * single dynamic hash object, there needs to be at least one dynamic hash node per stage
 * per hash function of each of these allocations.  The XBar and the Hash is on this granularity
 *
 * Because in the current structure, within a single stage a IR::MAU::HashDist object can
 * itself have multiple allocations, this breaks down each allocation on a per hash function
 * allocation.  Also, one IR object does not coordinate with one IXBar::HashDistIRUse object
 */
/*
void GenerateDynamicHashJson::gen_hash_dist_json(const IR::MAU::Table *tbl) {
    return;
    auto &hash_dists = tbl->resources->hash_dists;
    HashDistToAlloc ir_to_alloc;
    // Gather up all IXBar::HashDistIRUse per IR::MAU::HashDist allocation
    for (auto &hash_dist_use : hash_dists) {
        for (auto &ir_alloc : hash_dist_use.ir_allocations) {
            BUG_CHECK(ir_alloc.original_hd != nullptr, "Cannot coordinate HashDist "
                "in the dynamic hash JSON");
            ir_to_alloc[ir_alloc.original_hd].push_back(&ir_alloc);
        }
    }

    for (auto entry : ir_to_alloc) {
        auto orig_hd = entry.first;
        auto field_list = orig_hd->field_list->to<IR::HashListExpression>();
        if (field_list == nullptr) continue;
        if (field_list->fieldListCalcName.isNullOrEmpty()) continue;
        auto field_list_names = field_list->fieldListNames;
        if (field_list_names->names.empty()) continue;
        cstring fieldListName = field_list->fieldListNames->names[0];
        HashFuncToAlloc hash_func_to_alloc;
        // Gather a per hash function per IR allocation
        for (auto *ir_alloc : entry.second) {
            int key = ir_alloc->use.hash_dist_hash.group;
            hash_func_to_alloc[key].push_back(ir_alloc);
        }
        // FIXME: This is handled so oddly, and doesn't need to really exist.  It should be
        // the width of the HashDist object (but that doesn't work for dyn_hash.p4)
        int hash_bit_width = field_list->outputWidth;

        std::map<cstring, cstring> fieldNames;
        Util::JsonObject *_dynHashCalc = new Util::JsonObject();
        Util::JsonObject *_fieldList = new Util::JsonObject();
        Util::JsonArray *_xbar_cfgs = new Util::JsonArray();
        Util::JsonArray *_hash_cfgs = new Util::JsonArray();
        _dynHashCalc->emplace("name", field_list->fieldListCalcName);
        _dynHashCalc->emplace("handle", dynHashHandleBase + dynHashHandle++);

        // Algorithms are a global node to the hash calculation
        gen_algo_json(_dynHashCalc, field_list->algorithms);
        bool first_run = true;
        safe_vector<const IR::Expression *> field_list_order;
        // Field list is not on a per hash function/stage basis, but a global node
        for (auto func_to_alloc : hash_func_to_alloc) {
            for (auto &alloc : func_to_alloc.second) {
                if (first_run) {
                    first_run = false;
                    BUG_CHECK(!alloc->use.field_list_order.empty(), "Dynamic hash calculation "
                        "has an empty field list?");
                    field_list_order = alloc->use.field_list_order;
                    gen_field_list_json(_fieldList, fieldListName, field_list_order, fieldNames);
                } else {
                    BUG_CHECK(field_list_order == alloc->use.field_list_order, "Hash Dist "
                        "has different field list orders");
                }
            }
        }


        // For each hash function, generate the necessary xbar and hash cfgs
        for (auto func_to_alloc : hash_func_to_alloc) {
            IXBar::Use combined_use;
            for (auto &alloc : func_to_alloc.second) {
                for (auto &b : alloc->use.use) {
                    bool repeat_byte = false;
                    for (auto &c_b : combined_use.use) {
                        if (b.loc == c_b.loc) {
                            repeat_byte = true;
                            for (auto fi : b.field_bytes)
                                c_b.add_info(fi);
                            break;
                        }
                    }
                    if (!repeat_byte)
                        combined_use.use.push_back(b);
                }
                auto &hdh = combined_use.hash_dist_hash;
                auto &local_hdh = alloc->use.hash_dist_hash;
                hdh.allocated = true;
                hdh.group = func_to_alloc.first;
                hdh.galois_start_bit_to_p4_hash.insert(
                    local_hdh.galois_start_bit_to_p4_hash.begin(),
                    local_hdh.galois_start_bit_to_p4_hash.end());
            }
            gen_ixbar_bytes_json(_xbar_cfgs, tbl->stage(), fieldNames, combined_use);
            gen_hash_json(_hash_cfgs, tbl->stage(), combined_use, hash_bit_width);
            combined_use.clear();
        }
        _fieldList->emplace("crossbar_configuration", _xbar_cfgs);
        Util::JsonArray *_field_lists = new Util::JsonArray();
        _field_lists->append(_fieldList);
        _dynHashCalc->emplace("field_lists", _field_lists);
        _dynHashCalc->emplace("hash_configuration", _hash_cfgs);
        _dynHashCalc->emplace("hash_bit_width", hash_bit_width);
        _dynHashNode->append(_dynHashCalc);
    }
}
*/

void GenerateDynamicHashJson::gen_single_algo_json(Util::JsonArray *_algos,
                                                   const IR::MAU::HashFunction *algorithm,
                                                   cstring alg_name, bool &is_default) {
    Util::JsonObject *_algo = new Util::JsonObject();
    _algo->emplace("name"_cs, alg_name);  // p4 algo name
    _algo->emplace("type"_cs, algorithm->algo_type());
    unsigned aHandle = algoHandleBase + algoHandle;
    if (algoHandles.count(alg_name) > 0) {
        aHandle = algoHandles[alg_name];
    } else {
        algoHandles[alg_name] = aHandle;
        algoHandle++;
    }
    _algo->emplace("handle"_cs, aHandle);
    _algo->emplace("is_default"_cs, is_default);
    _algo->emplace("msb"_cs, algorithm->msb);
    _algo->emplace("extend"_cs, algorithm->extend);
    _algo->emplace("reverse"_cs, algorithm->reverse);
    // Convert poly in koopman notation to actual value
    big_int poly, init, final_xor;
    poly = algorithm->poly;
    poly = (poly << 1) + 1;
    _algo->emplace("poly"_cs, Util::toString(poly, 0, 0, 16));
    init = algorithm->init;
    _algo->emplace("init"_cs, Util::toString(init, 0, 0, 16));
    final_xor = algorithm->final_xor;
    _algo->emplace("final_xor"_cs, Util::toString(final_xor, 0, 0, 16));
    _algos->append(_algo);
    is_default = false;  // only set 1st algo to default
}

void GenerateDynamicHashJson::gen_algo_json(Util::JsonObject *_dhc,
                                            const IR::MAU::HashGenExpression *hge) {
    _dhc->emplace("any_hash_algorithm_allowed"_cs, hge->any_alg_allowed);
    Util::JsonArray *_algos = new Util::JsonArray();
    bool is_default = true;
    if (hge->alg_names) {
        for (auto a : hge->alg_names->names) {
            // Call Dyn Hash Library and generate a hash function object for
            // given algorithm
            auto algoExpr = IR::MAU::HashFunction::convertHashAlgorithmBFN(hge->srcInfo, a.name);
            auto algorithm = new IR::MAU::HashFunction();
            if (algorithm->setup(algoExpr)) {
                gen_single_algo_json(_algos, algorithm, a.name, is_default);
            }
        }
    } else {
        gen_single_algo_json(_algos, &hge->algorithm, hge->algorithm.name(), is_default);
    }
    _dhc->emplace("algorithms"_cs, _algos);
}

void GenerateDynamicHashJson::gen_field_list_json(Util::JsonObject *_field_list,
                                                  const IR::MAU::HashGenExpression *hge,
                                                  std::map<cstring, cstring> &fieldNames) {
    Util::JsonArray *_fields = new Util::JsonArray();
    auto fle = hge->expr->to<IR::MAU::FieldListExpression>();

    cstring field_list_name = fle->id.name;
    _field_list->emplace("name"_cs, field_list_name);
    _field_list->emplace("handle"_cs, fieldListHandleBase + fieldListHandle++);
    // Multiple field lists not supported,
    // is_default is always true, remove field?
    _field_list->emplace("is_default"_cs, true);
    // can_permute & can_rotate need to be passed in (as annotations?) to be set
    // here. these fields are optional in schema
    _field_list->emplace("can_permute"_cs, fle->permutable);
    _field_list->emplace("can_rotate"_cs, fle->rotateable);

    BuildP4HashFunction builder(phv);
    hge->apply(builder);
    P4HashFunction *func = builder.func();

    int numConstants = 0;
    for (auto *e : func->inputs) {
        auto field = PHV::AbstractField::create(phv, e);
        Util::JsonObject *_field = new Util::JsonObject();
        cstring name = ""_cs;
        auto range = field->range();
        if (field->is<PHV::Constant>()) {
            // Constant fields have unique field names with the format
            // constant<id>_<size>_<value>
            // id increments by 1 for every constant field.
            name = "constant" + std::to_string(numConstants++);
            name = name + "_" + std::to_string(range.size());
            name = name + "_" + field->to<PHV::Constant>()->value->toString();
        } else if (field->field()) {
            name = canon_name(field->field()->externalName());
            fieldNames[field->field()->name] = name;
        } else {
            continue;  // error out here?
        }
        _field->emplace("name"_cs, name);
        _field->emplace("start_bit"_cs, range.lo);
        _field->emplace("bit_width"_cs, range.size());
        // All fields optional by default, remove field?
        _field->emplace("optional"_cs, true);
        _field->emplace("is_constant"_cs, field->is<PHV::Constant>());
        _fields->append(_field);
    }
    _field_list->emplace("fields"_cs, _fields);
}

void GenerateDynamicHashJson::gen_ixbar_bytes_json(Util::JsonArray *_xbar_cfgs, int stage,
                                                   const std::map<cstring, cstring> &fieldNames,
                                                   const IXBar::Use &ixbar_use) {
    Util::JsonObject *_xbar_cfg = new Util::JsonObject();
    _xbar_cfg->emplace("stage_number"_cs, stage);
    Util::JsonArray *_xbar = new Util::JsonArray();
    for (auto &byte : ixbar_use.use) {
        for (auto &fieldinfo : byte.field_bytes) {
            for (int i = fieldinfo.lo; i <= fieldinfo.hi; i++) {
                Util::JsonObject *_xbar_byte = new Util::JsonObject();
                _xbar_byte->emplace("byte_number"_cs, (byte.loc.group * 16 + byte.loc.byte));
                _xbar_byte->emplace("bit_in_byte"_cs, (i - fieldinfo.lo + fieldinfo.cont_lo));
                _xbar_byte->emplace("name"_cs, fieldNames.at(fieldinfo.field));
                _xbar_byte->emplace("field_bit"_cs, i);
                _xbar->append(_xbar_byte);
            }
        }
    }
    _xbar_cfg->emplace("crossbar"_cs, _xbar);
    // TODO: Add wide hash support to populate 'crossbar_mod'
    _xbar_cfg->emplace("crossbar_mod"_cs, new Util::JsonArray());
    _xbar_cfgs->append(_xbar_cfg);
}

void GenerateDynamicHashJson::gen_hash_json(Util::JsonArray *_hash_cfgs, int stage,
                                            Tofino::IXBar::Use &ixbar_use, int &hash_bit_width) {
    int hashGroup = -1;
    Util::JsonArray *_hash_bits = new Util::JsonArray();
    int num_hash_bits = 0;
    if (ixbar_use.hash_dist_hash.allocated) {
        auto hdh = ixbar_use.hash_dist_hash;
        hashGroup = hdh.group;
        hash_bit_width = hash_bit_width >= 0 ? hash_bit_width : hdh.algorithm.size;
        // calculate the output hash bits from a hash table
        for (auto &b : hdh.galois_start_bit_to_p4_hash) {
            auto hash_bit = b.first;
            num_hash_bits += b.second.size();
            for (auto bit = b.second.lo; bit <= b.second.hi; bit++) {
                Util::JsonObject *_hash_bit = new Util::JsonObject();
                _hash_bit->emplace("gfm_hash_bit"_cs, hash_bit++);
                _hash_bit->emplace("p4_hash_bit"_cs, bit);
                _hash_bits->append(_hash_bit);
            }
        }
    } else if (ixbar_use.meter_alu_hash.allocated) {
        auto mah = ixbar_use.meter_alu_hash;
        hashGroup = mah.group;
        num_hash_bits = mah.bit_mask.is_contiguous() && (mah.bit_mask.min().index() == 0)
                            ? mah.bit_mask.max().index() + 1
                            : 0;
        if (!num_hash_bits) return;  // invalid bit mask
        for (auto hash_bit = 0; hash_bit < num_hash_bits; hash_bit++) {
            Util::JsonObject *_hash_bit = new Util::JsonObject();
            _hash_bit->emplace("gfm_hash_bit"_cs, hash_bit);
            _hash_bit->emplace("p4_hash_bit"_cs, hash_bit);
            _hash_bits->append(_hash_bit);
        }
    } else {
        BUG("  Cannot correctly generate a correct hash for dynamic hash");
    }
    Util::JsonObject *_hash_cfg = new Util::JsonObject();
    _hash_cfg->emplace("stage_number"_cs, stage);
    Util::JsonObject *_hash = new Util::JsonObject();
    _hash->emplace("hash_id"_cs, hashGroup);
    _hash->emplace("num_hash_bits"_cs, num_hash_bits);
    _hash->emplace("hash_bits"_cs, _hash_bits);
    _hash_cfg->emplace("hash"_cs, _hash);
    // TODO: Add wide hash support to populate 'hash_mod'
    Util::JsonObject *_hash_mod = new Util::JsonObject();
    _hash_mod->emplace("hash_id"_cs, 0);
    _hash_mod->emplace("num_hash_bits"_cs, 0);
    _hash_mod->emplace("hash_bits"_cs, new Util::JsonArray());
    _hash_cfg->emplace("hash_mod"_cs, _hash_mod);
    _hash_cfgs->append(_hash_cfg);
}

void GenerateDynamicHashJson::gen_ixbar_json(const IXBar::Use *ixbar_use_, Util::JsonObject *_dhc,
                                             int stage, const cstring field_list_name,
                                             const IR::NameList *algorithms, int hash_width) {
    auto *ixbar_use = dynamic_cast<const Tofino::IXBar::Use *>(ixbar_use_);
    Util::JsonArray *_field_lists = new Util::JsonArray();
    Util::JsonObject *_field_list = new Util::JsonObject();
    Util::JsonArray *_fields = new Util::JsonArray();
    _field_list->emplace("name"_cs, field_list_name);
    _field_list->emplace("handle"_cs, fieldListHandleBase + fieldListHandle++);
    // Multiple field lists not supported,
    // is_default is always true, remove field?
    _field_list->emplace("is_default"_cs, true);
    // can_permute & can_rotate need to be passed in (as annotations?) to be set
    // here. these fields are optional in schema
    _field_list->emplace("can_permute"_cs, false);
    _field_list->emplace("can_rotate"_cs, false);
    int numConstants = 0;
    std::map<cstring, cstring> fieldNames;
    for (auto *e : ixbar_use->field_list_order) {
        auto field = PHV::AbstractField::create(phv, e);
        Util::JsonObject *_field = new Util::JsonObject();
        cstring name = ""_cs;
        auto range = field->range();
        if (field->is<PHV::Constant>()) {
            // Constant fields have unique field names with the format
            // constant<id>_<size>_<value>
            // id increments by 1 for every constant field.
            name = "constant" + std::to_string(numConstants++);
            name = name + "_" + std::to_string(range.size());
            name = name + "_" + field->to<PHV::Constant>()->value->toString();
        } else if (field->field()) {
            name = canon_name(field->field()->externalName());
            fieldNames[field->field()->name] = name;
        } else {
            continue;  // error out here?
        }
        _field->emplace("name"_cs, name);
        _field->emplace("start_bit"_cs, range.lo);
        _field->emplace("bit_width"_cs, range.size());
        // All fields optional by default, remove field?
        _field->emplace("optional"_cs, true);
        _field->emplace("is_constant"_cs, field->is<PHV::Constant>());
        _fields->append(_field);
    }
    _field_list->emplace("fields"_cs, _fields);
    Util::JsonArray *_xbar_cfgs = new Util::JsonArray();
    Util::JsonObject *_xbar_cfg = new Util::JsonObject();
    _xbar_cfg->emplace("stage_number"_cs, stage);
    Util::JsonArray *_xbar = new Util::JsonArray();
    for (auto &byte : ixbar_use->use) {
        for (auto &fieldinfo : byte.field_bytes) {
            for (int i = fieldinfo.lo; i <= fieldinfo.hi; i++) {
                Util::JsonObject *_xbar_byte = new Util::JsonObject();
                _xbar_byte->emplace("byte_number"_cs, (byte.loc.group * 16 + byte.loc.byte));
                _xbar_byte->emplace("bit_in_byte"_cs, (i - fieldinfo.lo));
                _xbar_byte->emplace("name"_cs, fieldNames[fieldinfo.field]);
                _xbar_byte->emplace("field_bit"_cs, i);
                _xbar->append(_xbar_byte);
            }
        }
    }
    _xbar_cfg->emplace("crossbar"_cs, _xbar);
    // TODO: Add wide hash support to populate 'crossbar_mod'
    _xbar_cfg->emplace("crossbar_mod"_cs, new Util::JsonArray());
    _xbar_cfgs->append(_xbar_cfg);
    _field_list->emplace("crossbar_configuration"_cs, _xbar_cfgs);
    _field_lists->append(_field_list);
    _dhc->emplace("field_lists"_cs, _field_lists);
    int hashGroup = -1;
    Util::JsonArray *_hash_bits = new Util::JsonArray();
    int num_hash_bits = 0;
    int hash_bit_width = -1;
    if (ixbar_use->meter_alu_hash.allocated) {
        auto &mah = ixbar_use->meter_alu_hash;
        const IR::MAU::HashFunction *hashAlgo = &mah.algorithm;
        hashGroup = mah.group;
        hash_bit_width = hash_width <= 0 ? hashAlgo->size : hash_width;
        num_hash_bits = mah.bit_mask.is_contiguous() && (mah.bit_mask.min().index() == 0)
                            ? mah.bit_mask.max().index() + 1
                            : 0;
        if (!num_hash_bits) return;  // invalid bit mask
        for (auto hash_bit = 0; hash_bit < num_hash_bits; hash_bit++) {
            Util::JsonObject *_hash_bit = new Util::JsonObject();
            _hash_bit->emplace("gfm_hash_bit"_cs, hash_bit);
            _hash_bit->emplace("p4_hash_bit"_cs, hash_bit);
            _hash_bits->append(_hash_bit);
        }
    }
    // any_hash_algorithm_allowed is an optional field
    _dhc->emplace("any_hash_algorithm_allowed"_cs, false);
    Util::JsonArray *_algos = new Util::JsonArray();
    if (algorithms) {
        bool is_default = true;
        for (auto a : algorithms->names) {
            // Call Dyn Hash Library and generate a hash function object for
            // given algorithm
            auto srcInfo = ixbar_use->meter_alu_hash.allocated
                               ? &ixbar_use->meter_alu_hash.algorithm.srcInfo
                               : new Util::SourceInfo();
            auto algoExpr = IR::MAU::HashFunction::convertHashAlgorithmBFN(*srcInfo, a.name);
            auto algorithm = new IR::MAU::HashFunction();
            if (algorithm->setup(algoExpr)) {
                Util::JsonObject *_algo = new Util::JsonObject();
                _algo->emplace("name"_cs, a);  // p4 algo name
                _algo->emplace("type"_cs, algorithm->algo_type());
                unsigned aHandle = algoHandleBase + algoHandle;
                if (algoHandles.count(a) > 0) {
                    aHandle = algoHandles[a];
                } else {
                    algoHandles[a] = aHandle;
                    algoHandle++;
                }
                _algo->emplace("handle"_cs, aHandle);
                _algo->emplace("is_default"_cs, is_default);
                _algo->emplace("msb"_cs, algorithm->msb);
                _algo->emplace("extend"_cs, algorithm->extend);
                _algo->emplace("reverse"_cs, algorithm->reverse);
                // Convert poly in koopman notation to actual value
                big_int poly, init, final_xor;
                poly = algorithm->poly;
                poly = (poly << 1) + 1;
                _algo->emplace("poly"_cs, Util::toString(poly, 0, 0, 16));
                init = algorithm->init;
                _algo->emplace("init"_cs, Util::toString(init, 0, 0, 16));
                final_xor = algorithm->final_xor;
                _algo->emplace("final_xor"_cs, Util::toString(final_xor, 0, 0, 16));
                _algos->append(_algo);
                is_default = false;  // only set 1st algo to default
            }
        }
    }
    _dhc->emplace("algorithms"_cs, _algos);
    Util::JsonArray *_hash_cfgs = new Util::JsonArray();
    Util::JsonObject *_hash_cfg = new Util::JsonObject();
    _hash_cfg->emplace("stage_number"_cs, stage);
    Util::JsonObject *_hash = new Util::JsonObject();
    _hash->emplace("hash_id"_cs, hashGroup);
    _dhc->emplace("hash_bit_width"_cs, hash_bit_width);
    _hash->emplace("num_hash_bits"_cs, num_hash_bits);
    _hash->emplace("hash_bits"_cs, _hash_bits);
    _hash_cfg->emplace("hash"_cs, _hash);
    // TODO: Add wide hash support to populate 'hash_mod'
    Util::JsonObject *_hash_mod = new Util::JsonObject();
    _hash_mod->emplace("hash_id"_cs, 0);
    _hash_mod->emplace("num_hash_bits"_cs, 0);
    _hash_mod->emplace("hash_bits"_cs, new Util::JsonArray());
    _hash_cfg->emplace("hash_mod"_cs, _hash_mod);
    _hash_cfgs->append(_hash_cfg);
    _dhc->emplace("hash_configuration"_cs, _hash_cfgs);
}

std::ostream &operator<<(std::ostream &out, const DynamicHashJson &dyn) {
    auto dyn_json = new Util::JsonObject();
    if (dyn._dynHashNode)
        dyn_json->emplace("dynamic_hash_calculations"_cs, dyn._dynHashNode);
    else
        dyn_json->emplace("dynamic_hash_calculations"_cs, new Util::JsonObject());
    dyn_json->serialize(out);
    return out;
}

}  // namespace BFN
