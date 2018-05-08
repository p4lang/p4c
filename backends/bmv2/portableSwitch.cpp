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

#include "frontends/common/model.h"
#include "portableSwitch.h"

namespace P4 {

const IR::P4Program* PsaProgramStructure::create(const IR::P4Program* program) {
    createTypes();
    createHeaders();
    createExterns();
    createParsers();
    createActions();
    createControls();
    createDeparsers();
    return program;
}

Util::IJson* PsaProgramStructure::convertParserStatement(const IR::StatOrDecl* stat) {
    auto result = new Util::JsonObject();
    cstring scalarsName = refMap->newName("scalars");
    auto conv = new BMV2::PsaExpressionConverter(refMap,typeMap,scalarsName,&scalarMetadataFields);
    auto params = BMV2::mkArrayField(result, "parameters");
    if (stat->is<IR::AssignmentStatement>()) {
        auto assign = stat->to<IR::AssignmentStatement>();
        auto type = typeMap->getType(assign->left, true);
        cstring operation = BMV2::Backend::jsonAssignment(type, true);
        result->emplace("op", operation);
        auto l = conv->convertLeftValue(assign->left);
        bool convertBool = type->is<IR::Type_Boolean>();
        auto r = conv->convert(assign->right, true, true, convertBool);
        params->append(l);
        params->append(r);

        if (operation != "set") {
            // must wrap into another outer object
            auto wrap = new Util::JsonObject();
            wrap->emplace("op", "primitive");
            auto params = BMV2::mkParameters(wrap);
            params->append(result);
            result = wrap;
        }

        return result;
    } else if (stat->is<IR::MethodCallStatement>()) {
        auto mce = stat->to<IR::MethodCallStatement>()->methodCall;
        auto minst = P4::MethodInstance::resolve(mce, refMap, typeMap);
        if (minst->is<P4::ExternMethod>()) {
            auto extmeth = minst->to<P4::ExternMethod>();
            if (extmeth->method->name.name == corelib.packetIn.extract.name) {
                int argCount = mce->arguments->size();
                if (argCount == 1 || argCount == 2) {
                    cstring ename = argCount == 1 ? "extract" : "extract_VL";
                    result->emplace("op", ename);
                    auto arg = mce->arguments->at(0);
                    auto argtype = typeMap->getType(arg->expression, true);
                    if (!argtype->is<IR::Type_Header>()) {
                        ::error("%1%: extract only accepts arguments with header types, not %2%",
                                arg, argtype);
                        return result;
                    }
                    auto param = new Util::JsonObject();
                    params->append(param);
                    cstring type;
                    Util::IJson* j = nullptr;

                    if (arg->expression->is<IR::Member>()) {
                        auto mem = arg->expression->to<IR::Member>();
                        auto baseType = typeMap->getType(mem->expr, true);
                        if (baseType->is<IR::Type_Stack>()) {
                            if (mem->member == IR::Type_Stack::next) {
                                type = "stack";
                                j = conv->convert(mem->expr);
                            } else {
                                BUG("%1%: unsupported", mem);
                            }
                        }
                    }
                    if (j == nullptr) {
                        type = "regular";
                        j = conv->convert(arg->expression);
                    }
                    auto value = j->to<Util::JsonObject>()->get("value");
                    param->emplace("type", type);
                    param->emplace("value", value);

                    if (argCount == 2) {
                        auto arg2 = mce->arguments->at(1);
                        auto jexpr = conv->convert(arg2->expression, true, false);
                        auto rwrap = new Util::JsonObject();
                        // The spec says that this must always be wrapped in an expression
                        rwrap->emplace("type", "expression");
                        rwrap->emplace("value", jexpr);
                        params->append(rwrap);
                    }
                    return result;
                }
            }
        } else if (minst->is<P4::ExternFunction>()) {
            auto extfn = minst->to<P4::ExternFunction>();
            if (extfn->method->name.name == IR::ParserState::verify) {
                result->emplace("op", "verify");
                BUG_CHECK(mce->arguments->size() == 2, "%1%: Expected 2 arguments", mce);
                {
                    auto cond = mce->arguments->at(0);
                    // false means don't wrap in an outer expression object, which is not needed
                    // here
                    auto jexpr = conv->convert(cond->expression, true, false);
                    params->append(jexpr);
                }
                {
                    auto error = mce->arguments->at(1);
                    // false means don't wrap in an outer expression object, which is not needed
                    // here
                    auto jexpr = conv->convert(error->expression, true, false);
                    params->append(jexpr);
                }
                return result;
            }
        } else if (minst->is<P4::BuiltInMethod>()) {
            /* example result:
             {
                "parameters" : [
                {
                  "op" : "add_header",
                  "parameters" : [{"type" : "header", "value" : "ipv4"}]
                }
              ],
              "op" : "primitive"
            } */
            result->emplace("op", "primitive");

            auto bi = minst->to<P4::BuiltInMethod>();
            cstring primitive;
            auto paramsValue = new Util::JsonObject();
            params->append(paramsValue);

            auto pp = BMV2::mkArrayField(paramsValue, "parameters");
            auto obj = conv->convert(bi->appliedTo);
            pp->append(obj);

            if (bi->name == IR::Type_Header::setValid) {
                primitive = "add_header";
            } else if (bi->name == IR::Type_Header::setInvalid) {
                primitive = "remove_header";
            } else if (bi->name == IR::Type_Stack::push_front ||
                       bi->name == IR::Type_Stack::pop_front) {
                if (bi->name == IR::Type_Stack::push_front)
                    primitive = "push";
                else
                    primitive = "pop";

                BUG_CHECK(mce->arguments->size() == 1, "Expected 1 argument for %1%", mce);
                auto arg = conv->convert(mce->arguments->at(0)->expression);
                pp->append(arg);
            } else {
                BUG("%1%: Unexpected built-in method", bi->name);
            }

            paramsValue->emplace("op", primitive);
            return result;
        }
    }
    ::error("%1%: not supported in parser on this target", stat);
    return result;
}


void PsaProgramStructure::convertSimpleKey(const IR::Expression* keySet,
                                       mpz_class& value, mpz_class& mask) const {
    if (keySet->is<IR::Mask>()) {
        auto mk = keySet->to<IR::Mask>();
        if (!mk->left->is<IR::Constant>()) {
            ::error("%1% must evaluate to a compile-time constant", mk->left);
            return;
        }
        if (!mk->right->is<IR::Constant>()) {
            ::error("%1% must evaluate to a compile-time constant", mk->right);
            return;
        }
        value = mk->left->to<IR::Constant>()->value;
        mask = mk->right->to<IR::Constant>()->value;
    } else if (keySet->is<IR::Constant>()) {
        value = keySet->to<IR::Constant>()->value;
        mask = -1;
    } else if (keySet->is<IR::BoolLiteral>()) {
        value = keySet->to<IR::BoolLiteral>()->value ? 1 : 0;
        mask = -1;
    } else if (keySet->is<IR::DefaultExpression>()) {
        value = 0;
        mask = 0;
    } else {
        ::error("%1% must evaluate to a compile-time constant", keySet);
        value = 0;
        mask = 0;
    }
}

unsigned PsaProgramStructure::combine(const IR::Expression* keySet,
                                const IR::ListExpression* select,
                                mpz_class& value, mpz_class& mask,
                                bool& is_vset, cstring& vset_name) const {
    // From the BMv2 spec: For values and masks, make sure that you
    // use the correct format. They need to be the concatenation (in
    // the right order) of all byte padded fields (padded with 0
    // bits). For example, if the transition key consists of a 12-bit
    // field and a 2-bit field, each value will need to have 3 bytes
    // (2 for the first field, 1 for the second one). If the
    // transition value is 0xaba, 0x3, the value attribute will be set
    // to 0x0aba03.
    // Return width in bytes
    value = 0;
    mask = 0;
    is_vset = false;
    unsigned totalWidth = 0;
    if (keySet->is<IR::DefaultExpression>()) {
        return totalWidth;
    } else if (keySet->is<IR::ListExpression>()) {
        auto le = keySet->to<IR::ListExpression>();
        BUG_CHECK(le->components.size() == select->components.size(),
                  "%1%: mismatched select", select);
        unsigned index = 0;

        bool noMask = true;
        for (auto it = select->components.begin();
             it != select->components.end(); ++it) {
            auto e = *it;
            auto keyElement = le->components.at(index);

            auto type = typeMap->getType(e, true);
            int width = type->width_bits();
            BUG_CHECK(width > 0, "%1%: unknown width", e);

            mpz_class key_value, mask_value;
            convertSimpleKey(keyElement, key_value, mask_value);
            unsigned w = 8 * ROUNDUP(width, 8);
            totalWidth += ROUNDUP(width, 8);
            value = Util::shift_left(value, w) + key_value;
            if (mask_value != -1) {
                noMask = false;
            } else {
                // mask_value == -1 is a special value used to
                // indicate an exact match on all bit positions.  When
                // there is more than one keyElement, we must
                // represent such an exact match with 'width' 1 bits,
                // because it may be combined into a mask for other
                // keyElements that have their own independent masks.
                mask_value = Util::mask(width);
            }
            mask = Util::shift_left(mask, w) + mask_value;
            LOG3("Shifting " << " into key " << key_value << " &&& " << mask_value <<
                             " result is " << value << " &&& " << mask);
            index++;
        }

        if (noMask)
            mask = -1;
        return totalWidth;
    } else if (keySet->is<IR::PathExpression>()) {
        auto pe = keySet->to<IR::PathExpression>();
        auto decl = refMap->getDeclaration(pe->path, true);
        vset_name = decl->controlPlaneName();
        is_vset = true;
        return totalWidth;
    } else {
        BUG_CHECK(select->components.size() == 1, "%1%: mismatched select/label", select);
        convertSimpleKey(keySet, value, mask);
        auto type = typeMap->getType(select->components.at(0), true);
        return ROUNDUP(type->width_bits(), 8);
    }
}

Util::IJson* PsaProgramStructure::stateName(IR::ID state) {
    if (state.name == IR::ParserState::accept) {
        return Util::JsonValue::null;
    } else if (state.name == IR::ParserState::reject) {
        ::warning("Explicit transition to %1% not supported on this target", state);
        return Util::JsonValue::null;
    } else {
        return new Util::JsonValue(state.name);
    }
}

std::vector<Util::IJson*>
PsaProgramStructure::convertSelectExpression(const IR::SelectExpression* expr) {
    std::vector<Util::IJson*> result;
    auto se = expr->to<IR::SelectExpression>();
    for (auto sc : se->selectCases) {
        auto trans = new Util::JsonObject();
        mpz_class value, mask;
        bool is_vset;
        cstring vset_name;
        unsigned bytes = combine(sc->keyset, se->select, value, mask, is_vset, vset_name);
        if (is_vset) {
            trans->emplace("type", "parse_vset");
            trans->emplace("value", vset_name);
            trans->emplace("mask", mask);
            trans->emplace("next_state", stateName(sc->state->path->name));
        } else {
            if (mask == 0) {
                trans->emplace("value", "default");
                trans->emplace("mask", Util::JsonValue::null);
                trans->emplace("next_state", stateName(sc->state->path->name));
            } else {
                trans->emplace("type", "hexstr");
                trans->emplace("value", BMV2::stringRepr(value, bytes));
                if (mask == -1)
                    trans->emplace("mask", Util::JsonValue::null);
                else
                    trans->emplace("mask", BMV2::stringRepr(mask, bytes));
                trans->emplace("next_state", stateName(sc->state->path->name));
            }
        }
        result.push_back(trans);
    }
    return result;
}

Util::IJson* PsaProgramStructure::convertSelectKey(const IR::SelectExpression* expr) {
    auto se = expr->to<IR::SelectExpression>();
    CHECK_NULL(se);
    auto key = conv->convert(se->select, false);
    return key;
}

Util::IJson*
PsaProgramStructure::convertPathExpression(const IR::PathExpression* pe) {
    auto trans = new Util::JsonObject();
    trans->emplace("value", "default");
    trans->emplace("mask", Util::JsonValue::null);
    trans->emplace("next_state", stateName(pe->path->name));
    return trans;
}

Util::IJson*
PsaProgramStructure::createDefaultTransition() {
    auto trans = new Util::JsonObject();
    trans->emplace("value", "default");
    trans->emplace("mask", Util::JsonValue::null);
    trans->emplace("next_state", Util::JsonValue::null);
    return trans;
}

void PsaProgramStructure::createStructLike(const IR::Type_StructLike* st) {
    CHECK_NULL(st);
    cstring name = st->controlPlaneName();
    unsigned max_length = 0;  // for variable-sized headers
    bool varbitFound = false;
    auto fields = new Util::JsonArray();
    for (auto f : st->fields) {
        auto field = new Util::JsonArray();
        auto ftype = typeMap->getType(f, true);
        if (ftype->to<IR::Type_StructLike>()) {
            BUG("%1%: nested structure", st);
        } else if (ftype->is<IR::Type_Boolean>()) {
            field->append(f->name.name);
            field->append(1);
            field->append(0);
            max_length += 1;
        } else if (auto type = ftype->to<IR::Type_Bits>()) {
            field->append(f->name.name);
            field->append(type->size);
            field->append(type->isSigned);
            max_length += type->size;
        } else if (auto type = ftype->to<IR::Type_Varbits>()) {
            field->append(f->name.name);
            max_length += type->size;
            field->append("*");
            if (varbitFound)
                ::error("%1%: headers with multiple varbit fields not supported", st);
            varbitFound = true;
        } else if (auto type = ftype->to<IR::Type_Error>()) {
            field->append(f->name.name);
            field->append(error_width);
            field->append(0);
            max_length += error_width;
        } else if (ftype->to<IR::Type_Stack>()) {
            BUG("%1%: nested stack", st);
        } else {
            BUG("%1%: unexpected type for %2%.%3%", ftype, st, f->name);
        }
        fields->append(field);
    }
    // must add padding
    unsigned padding = max_length % 8;
    if (padding != 0) {
        cstring name = refMap->newName("_padding");
        auto field = new Util::JsonArray();
        field->append(name);
        field->append(8 - padding);
        field->append(false);
        fields->append(field);
    }

    unsigned max_length_bytes = (max_length + padding) / 8;
    if (!varbitFound) {
        // ignore
        max_length = 0;
        max_length_bytes = 0;
    }
    json->add_header_type(name, fields, max_length_bytes);
}

void PsaProgramStructure::createTypes() {
    for (auto kv : header_types)
        createStructLike(kv.second);
    for (auto kv : metadata_types)
        createStructLike(kv.second);
    for (auto kv : header_union_types) {
        auto st = kv.second;
        auto fields = new Util::JsonArray();
        for (auto f : st->fields) {
            auto field = new Util::JsonArray();
            auto ftype = typeMap->getType(f, true);
            auto ht = ftype->to<IR::Type_Header>();
            CHECK_NULL(ht);
            field->append(f->name.name);
            field->append(ht->name.name);
        }
        json->add_union_type(st->name, fields);
    }
    // add errors to json
    // add enums to json
}

void PsaProgramStructure::createHeaders() {
    for (auto kv : headers) {
        auto type = kv.second->type->to<IR::Type_StructLike>();
        json->add_header(type->controlPlaneName(), kv.second->name);
    }
    for (auto kv : metadata) {
        auto type = kv.second->type->to<IR::Type_StructLike>();
        json->add_header(type->controlPlaneName(), kv.second->name);
    }
    for (auto kv : header_stacks) {

        // json->add_header_stack(stack_type, stack_name, stack_size, ids);
    }
    for (auto kv : header_unions) {
        auto header_name = kv.first;
        auto header_type = kv.second->to<IR::Type_StructLike>()->controlPlaneName();
        // We have to add separately a header instance for all
        // headers in the union.  Each instance will be named with
        // a prefix including the union name, e.g., "u.h"
        Util::JsonArray* fields = new Util::JsonArray();
        for (auto uf : kv.second->to<IR::Type_HeaderUnion>()->fields) {
            auto uft = typeMap->getType(uf, true);
            auto h_name = header_name + "." + uf->controlPlaneName();
            auto h_type = uft->to<IR::Type_StructLike>()->controlPlaneName();
            unsigned id = json->add_header(h_type, h_name);
            fields->append(id);
        }
        json->add_union(header_type, fields, header_name);
    }
}

void PsaProgramStructure::createParsers() {
    // add parsers to json
    for (auto kv : parsers) {
        LOG1("parser" << kv.first << kv.second);
        auto parser_id = json->add_parser(kv.first);
        for (auto s : kv.second->parserLocals) {
            if (auto inst = s->to<IR::P4ValueSet>()) {
                auto bitwidth = inst->elementType->width_bits();
                auto name = inst->controlPlaneName();
                json->add_parse_vset(name, bitwidth);
            }
        }

        // convert parse state
        for (auto state : kv.second->states) {
            if (state->name == IR::ParserState::reject || state->name == IR::ParserState::accept)
                continue;
            auto state_id = json->add_parser_state(parser_id, state->controlPlaneName());
            // convert statements
            for (auto s : state->components) {
                auto op = convertParserStatement(s);
                json->add_parser_op(state_id, op);
            }

            // convert transitions
            if (state->selectExpression != nullptr) {
                if (state->selectExpression->is<IR::SelectExpression>()) {
                    auto expr = state->selectExpression->to<IR::SelectExpression>();
                    auto transitions = convertSelectExpression(expr);
                    for (auto transition : transitions) {
                        json->add_parser_transition(state_id, transition);
                    }
                    auto key = convertSelectKey(expr);
                    json->add_parser_transition_key(state_id, key);
                } else if (state->selectExpression->is<IR::PathExpression>()) {
                    auto expr = state->selectExpression->to<IR::PathExpression>();
                    auto transition = convertPathExpression(expr);
                    json->add_parser_transition(state_id, transition);
                } else {
                    BUG("%1%: unexpected selectExpression", state->selectExpression);
                }
            } else {
                auto transition = createDefaultTransition();
                json->add_parser_transition(state_id, transition);
            }

        }

    }

}


void PsaProgramStructure::createExterns() {
    // add parse_vsets to json
    // add meter_arrays to json
    // add counter_arrays to json
    // add register_arrays to json
    // add checksums to json
    // add learn_list to json
    // add calculations to json
    // add extern_instances to json
}

void PsaProgramStructure::createActions() {
    // add actions to json
}

void PsaProgramStructure::createControls() {
    // add pipelines to json
}

void PsaProgramStructure::createDeparsers() {
    // add deparsers to json
}

bool ParsePsaArchitecture::preorder(const IR::ToplevelBlock* block) {
    return false;
}

void ParsePsaArchitecture::parse_pipeline(const IR::PackageBlock* block, gress_t gress) {
    if (gress == INGRESS) {
        auto parser = block->getParameterValue("ip")->to<IR::ParserBlock>();
        auto pipeline = block->getParameterValue("ig")->to<IR::ControlBlock>();
        auto deparser = block->getParameterValue("id")->to<IR::ControlBlock>();
        structure->block_type.emplace(parser->container, std::make_pair(gress, PARSER));
        structure->block_type.emplace(pipeline->container, std::make_pair(gress, PIPELINE));
        structure->block_type.emplace(deparser->container, std::make_pair(gress, DEPARSER));
    } else if (gress == EGRESS) {
        auto parser = block->getParameterValue("ep")->to<IR::ParserBlock>();
        auto pipeline = block->getParameterValue("eg")->to<IR::ControlBlock>();
        auto deparser = block->getParameterValue("ed")->to<IR::ControlBlock>();
        structure->block_type.emplace(parser->container, std::make_pair(gress, PARSER));
        structure->block_type.emplace(pipeline->container, std::make_pair(gress, PIPELINE));
        structure->block_type.emplace(deparser->container, std::make_pair(gress, DEPARSER));
    }
}

bool ParsePsaArchitecture::preorder(const IR::PackageBlock* block) {
    auto pkg = block->getParameterValue("ingress");
    if (auto ingress = pkg->to<IR::PackageBlock>()) {
        parse_pipeline(ingress, INGRESS);
    }
    pkg = block->getParameterValue("egress");
    if (auto egress = pkg->to<IR::PackageBlock>()) {
        parse_pipeline(egress, EGRESS);
    }
    return false;
}

void InspectPsaProgram::postorder(const IR::P4Parser* p) {
    // populate structure->parsers
}

void InspectPsaProgram::postorder(const IR::P4Control* c) {

}

void InspectPsaProgram::postorder(const IR::Type_Header* h) {
    // inspect IR::Type_Header
    // populate structure->header_types;
}

void InspectPsaProgram::postorder(const IR::Type_HeaderUnion* hu) {
    // inspect IR::Type_HeaderUnion
    // populate structure->header_union_types;
}

void InspectPsaProgram::postorder(const IR::Declaration_Variable* var) {
    // inspect IR::Declaration_Variable
    // populate structure->headers or
    //          structure->header_stacks or
    //          structure->header_unions
    // based on the type of the variable
}

void InspectPsaProgram::postorder(const IR::Declaration_Instance* di) {
    // inspect IR::Declaration_Instance,
    // populate structure->meter_arrays or
    //          structure->counter_arrays or
    //          structure->register_arrays or
    //          structure->extern_instances or
    //          structure->checksums
    // based on the type of the instance
}

void InspectPsaProgram::postorder(const IR::P4Action* act) {
    // inspect IR::P4Action,
    // populate structure->actions
}

void InspectPsaProgram::postorder(const IR::Type_Error* err) {
    // inspect IR::Type_Error
    // populate structure->errors.
}

bool InspectPsaProgram::isHeaders(const IR::Type_StructLike* st) {
    bool result = false;
    for (auto f : st->fields) {
        if (f->type->is<IR::Type_Header>() || f->type->is<IR::Type_Stack>()) {
            result = true;
        }
    }
    return result;
}

void InspectPsaProgram::addHeaderType(const IR::Type_StructLike *st) {
    if (st->is<IR::Type_HeaderUnion>()) {
        for (auto f : st->fields) {
            auto ftype = typeMap->getType(f, true);
            auto ht = ftype->to<IR::Type_Header>();
            CHECK_NULL(ht);
            addHeaderType(ht);
        }
        pinfo->header_union_types.emplace(st->getName(), st->to<IR::Type_HeaderUnion>());
        return;
    } else if (st->is<IR::Type_Header>()) {
        pinfo->header_types.emplace(st->getName(), st->to<IR::Type_Header>());
    } else if (st->is<IR::Type_Struct>()) {
        pinfo->metadata_types.emplace(st->getName(), st->to<IR::Type_Struct>());
    }
}

void InspectPsaProgram::addHeaderInstance(const IR::Type_StructLike *st, cstring name) {
    auto inst = new IR::Declaration_Variable(name, st);
    if (st->is<IR::Type_Header>())
        pinfo->headers.emplace(name, inst);
    else if (st->is<IR::Type_Struct>())
        pinfo->metadata.emplace(name, inst);
    else if (st->is<IR::Type_HeaderUnion>())
        pinfo->header_unions.emplace(name, inst);
}

void InspectPsaProgram::addTypesAndInstances(const IR::Type_StructLike* type, bool isHeader) {
    addHeaderType(type);
    addHeaderInstance(type, type->controlPlaneName());
    for (auto f : type->fields) {
        auto ft = typeMap->getType(f, true);
        if (ft->is<IR::Type_StructLike>()) {
            // The headers struct can not contain nested structures.
            if (isHeader && ft->is<IR::Type_Struct>()) {
                ::error("Type %1% should only contain headers, header stacks, or header unions",
                        type);
                return;
            }
            if (auto hft = ft->to<IR::Type_Header>()) {
                addHeaderType(hft);
                addHeaderInstance(hft, hft->controlPlaneName());
            } else if (ft->is<IR::Type_HeaderUnion>()) {
                for (auto uf : ft->to<IR::Type_HeaderUnion>()->fields) {
                    auto uft = typeMap->getType(uf, true);
                    if (auto h_type = uft->to<IR::Type_Header>()) {
                        addHeaderType(h_type);
                        addHeaderInstance(h_type, h_type->controlPlaneName());
                    } else {
                        ::error("Type %1% cannot contain type %2%", ft, uft);
                        return;
                    }
                }
                pinfo->header_union_types.emplace(type->getName(), type->to<IR::Type_HeaderUnion>());
                addHeaderInstance(type, type->controlPlaneName());
            } else {
                LOG1("add struct type " << type);
                pinfo->metadata_types.emplace(type->getName(), type->to<IR::Type_Struct>());
                addHeaderInstance(type, type->controlPlaneName());
            }
        } else if (ft->is<IR::Type_Stack>()) {
            auto stack = ft->to<IR::Type_Stack>();
            auto stack_name = f->controlPlaneName();
            auto stack_size = stack->getSize();
            auto type = typeMap->getTypeType(stack->elementType, true);
            BUG_CHECK(type->is<IR::Type_Header>(), "%1% not a header type", stack->elementType);
            auto ht = type->to<IR::Type_Header>();
            addHeaderType(ht);
            auto stack_type = stack->elementType->to<IR::Type_Header>();
            std::vector<unsigned> ids;
            for (unsigned i = 0; i < stack_size; i++) {
                cstring hdrName = f->controlPlaneName() + "[" + Util::toString(i) + "]";
                // FIXME:
                // auto id = json->add_header(stack_type, hdrName);
                addHeaderInstance(stack_type, hdrName);
                // ids.push_back(id);
            }
            // addHeaderStackInstance();
        } else {
            // Treat this field like a scalar local variable
            cstring newName = refMap->newName(type->getName() + "." + f->name);
            if (ft->is<IR::Type_Bits>()) {
                auto tb = ft->to<IR::Type_Bits>();
                pinfo->scalars_width += tb->size;
                pinfo->scalarMetadataFields.emplace(f, newName);
            } else if (ft->is<IR::Type_Boolean>()) {
                pinfo->scalars_width += 1;
                pinfo->scalarMetadataFields.emplace(f, newName);
            } else if (ft->is<IR::Type_Error>()) {
                pinfo->scalars_width += 32;
                pinfo->scalarMetadataFields.emplace(f, newName);
            } else {
                BUG("%1%: Unhandled type for %2%", ft, f);
            }
        }
    }
}

bool InspectPsaProgram::preorder(const IR::Parameter* param) {
    auto ft = typeMap->getType(param->getNode(), true);
    // only convert parameters that are IR::Type_StructLike
    if (!ft->is<IR::Type_StructLike>())
        return false;
    auto st = ft->to<IR::Type_StructLike>();
    // parameter must be a type that we have not seen before
    if (pinfo->hasVisited(st))
        return false;
    LOG1("type struct " << ft);
    auto isHeader = isHeaders(st);
    addTypesAndInstances(st, isHeader);
    return false;
}

bool InspectPsaProgram::preorder(const IR::P4Parser* p) {
    if (pinfo->block_type.count(p)) {
        auto info = pinfo->block_type.at(p);
        if (info.first == INGRESS && info.second == PARSER)
            pinfo->parsers.emplace("ingress", p);
        else if (info.first == EGRESS && info.second == PARSER)
            pinfo->parsers.emplace("egress", p);
    }
    return false;
}

bool InspectPsaProgram::preorder(const IR::P4Control *c) {
    if (pinfo->block_type.count(c)) {
        auto info = pinfo->block_type.at(c);
        if (info.first == INGRESS && info.second == PIPELINE)
            pinfo->pipelines.emplace("ingress", c);
        else if (info.first == EGRESS && info.second == PIPELINE)
            pinfo->pipelines.emplace("egress", c);
        else if (info.first == INGRESS && info.second == DEPARSER)
            pinfo->deparsers.emplace("ingress", c);
        else if (info.first == EGRESS && info.second == DEPARSER)
            pinfo->deparsers.emplace("egress", c);
    }
    return false;
}



}  // namespace P4
