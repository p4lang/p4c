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

#include "bf-p4c/arch/arch.h"

#include <utility>

#include "bf-p4c/arch/psa/psa.h"
#include "bf-p4c/arch/t2na.h"
#include "bf-p4c/arch/tna.h"
#include "bf-p4c/arch/v1model.h"
#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/common/pragma/collect_global_pragma.h"
#include "bf-p4c/device.h"
#include "frontends/p4/methodInstance.h"
#include "ir/declaration.h"
#include "ir/id.h"
#include "ir/ir-generated.h"
#include "lib/cstring.h"

namespace BFN {

Pipeline::Pipeline(cstring name, const IR::BFN::P4Thread *ingress, const IR::BFN::P4Thread *egress,
                   const IR::BFN::P4Thread *ghost)
    : names({name}) {
    threads[INGRESS] = ingress;
    threads[EGRESS] = egress;
    if (ghost) threads[GHOST] = ghost;

    CollectGlobalPragma cgp;
    for (const auto &[gress, thread] : threads) {
        thread->apply(cgp);
    }
    insertPragmas(cgp.global_pragmas());
}

bool Pipeline::equiv(const Pipeline &other) const {
    if (pragmas != other.pragmas) {
        return false;
    }
    if (threads.size() != other.threads.size()) return false;
    for (const auto &[gress, thread] : threads) {
        if (other.threads.count(gress) == 0) return false;
        if (!thread->equiv(*other.threads.at(gress))) {
            return false;
        }
    }
    return true;
}

void Pipeline::insertPragmas(const std::vector<const IR::Annotation *> &all_pragmas) {
    auto applies = [this](const IR::Annotation *p) {
        if (p->expr.empty() || p->expr.at(0) == nullptr) {
            return true;
        }

        auto arg0 = p->expr.at(0)->to<IR::StringLiteral>();

        // Determine whether the name of a pipe is present as the first argument.
        // If the pipe name doesn't match, the pragma is to be applied for a different pipe.
        if (BFNContext::get().isPipeName(arg0->value)) {
            return arg0->value == names.front();
        }
        return true;
    };
    for (const auto *pragma : all_pragmas) {
        if (applies(pragma)) {
            pragmas.push_back(pragma);
        }
    }
}

ArchTranslation::ArchTranslation(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                                 BFN_Options &options) {
    if (Architecture::currentArchitecture() == Architecture::V1MODEL) {
        passes.push_back(new BFN::SimpleSwitchTranslation(refMap, typeMap, options /*map*/));
    } else if (Architecture::currentArchitecture() == Architecture::TNA) {
        if (Device::currentDevice() == Device::TOFINO) {
            passes.push_back(new BFN::TnaArchTranslation(refMap, typeMap, options));
        }
        if (Device::currentDevice() == Device::JBAY) {
            passes.push_back(new BFN::T2naArchTranslation(refMap, typeMap, options));
        }
    } else if (Architecture::currentArchitecture() == Architecture::T2NA) {
        if (Device::currentDevice() == Device::JBAY) {
            passes.push_back(new BFN::T2naArchTranslation(refMap, typeMap, options));
        }
    } else if (Architecture::currentArchitecture() == Architecture::PSA) {
        passes.push_back(new BFN::PortableSwitchTranslation(refMap, typeMap, options /*map*/));
    } else {
        P4C_UNIMPLEMENTED("Unknown architecture");
    }
}

bool Architecture::preorder(const IR::PackageBlock *pkg) {
    if (pkg->type->name == "V1Switch") {
        architecture = V1MODEL;
        version = "1.0.0"_cs;  // V1model arch does not have a version
        found = true;
    } else if (pkg->type->name == "PSA_Switch") {
        architecture = PSA;
        version = "1.0.0"_cs;  // PSA arch does not have versioning
        found = true;
    } else if (pkg->type->name == "Switch" || pkg->type->name == "MultiParserSwitch") {
        if (auto annot = pkg->type->getAnnotation(IR::Annotation::pkginfoAnnotation)) {
            for (auto kv : annot->kv) {
                if (kv->name == "arch") {
                    architecture = toArchEnum(kv->expression->to<IR::StringLiteral>()->value);
                } else if (kv->name == "version") {
                    version = kv->expression->to<IR::StringLiteral>()->value;
                }
            }
            found = true;
        }
    }
    return false;
}

// parse tna pipeline with single parser.
void ParseTna::parseSingleParserPipeline(const IR::PackageBlock *block, unsigned index) {
    auto thread_i = new IR::BFN::P4Thread();
    bool isMultiParserProgram = (block->type->name == "MultiParserPipeline");
    cstring pipeName;
    auto decl = block->node->to<IR::Declaration_Instance>();
    // If no declaration found (anonymous instantiation) get the pipe name from arch definition
    if (decl) {
        pipeName = decl->Name();
    } else {
        auto cparams = mainBlock->getConstructorParameters();
        auto idxParam = cparams->getParameter(index);
        pipeName = idxParam->name;
    }
    BUG_CHECK(!pipeName.isNullOrEmpty(), "Cannot determine pipe name for pipe block at index %d",
              index);

    if (isMultiParserProgram) {
        auto ingress_parsers = block->getParameterValue("ig_prsr"_cs);
        BUG_CHECK(ingress_parsers->is<IR::PackageBlock>(), "Expected PackageBlock");
        parseMultipleParserInstances(ingress_parsers->to<IR::PackageBlock>(), pipeName, thread_i,
                                     INGRESS);
    } else {
        auto ingress_parser = block->getParameterValue("ingress_parser"_cs);
        BlockInfo ingress_parser_block_info(index, pipeName, INGRESS, PARSER);
        BUG_CHECK(ingress_parser->is<IR::ParserBlock>(), "Expected ParserBlock");
        thread_i->parsers.push_back(ingress_parser->to<IR::ParserBlock>()->container);
        toBlockInfo.emplace(ingress_parser->to<IR::ParserBlock>()->container,
                            ingress_parser_block_info);
    }

    auto ingress = block->getParameterValue("ingress"_cs);
    BlockInfo ingress_mau_block_info(index, pipeName, INGRESS, MAU);
    thread_i->mau = ingress->to<IR::ControlBlock>()->container;
    toBlockInfo.emplace(ingress->to<IR::ControlBlock>()->container, ingress_mau_block_info);

    if (auto ingress_deparser = block->findParameterValue("ingress_deparser"_cs)) {
        BlockInfo ingress_deparser_block_info(index, pipeName, INGRESS, DEPARSER);
        thread_i->deparser = ingress_deparser->to<IR::ControlBlock>()->container;
        toBlockInfo.emplace(ingress_deparser->to<IR::ControlBlock>()->container,
                            ingress_deparser_block_info);
    }

    auto thread_e = new IR::BFN::P4Thread();
    if (isMultiParserProgram) {
        if (auto egress_parser = block->findParameterValue("eg_prsr"_cs)) {
            auto parsers = egress_parser->to<IR::PackageBlock>();
            parseMultipleParserInstances(parsers, pipeName, thread_e, EGRESS);
        }
    } else {
        if (auto egress_parser = block->findParameterValue("egress_parser"_cs)) {
            BlockInfo egress_parser_block_info(index, pipeName, EGRESS, PARSER);
            thread_e->parsers.push_back(egress_parser->to<IR::ParserBlock>()->container);
            toBlockInfo.emplace(egress_parser->to<IR::ParserBlock>()->container,
                                egress_parser_block_info);
        }
    }

    auto egress = block->getParameterValue("egress"_cs);
    thread_e->mau = egress->to<IR::ControlBlock>()->container;
    BlockInfo egess_mau_block_info(index, pipeName, EGRESS, MAU);
    toBlockInfo.emplace(egress->to<IR::ControlBlock>()->container, egess_mau_block_info);

    auto egress_deparser = block->getParameterValue("egress_deparser"_cs);
    thread_e->deparser = egress_deparser->to<IR::ControlBlock>()->container;
    BlockInfo egress_deparser_block_info(index, pipeName, EGRESS, DEPARSER);
    toBlockInfo.emplace(egress_deparser->to<IR::ControlBlock>()->container,
                        egress_deparser_block_info);

    IR::BFN::P4Thread *thread_g = nullptr;
    if (auto ghost = block->findParameterValue("ghost"_cs)) {
        auto ghost_cb = ghost->to<IR::ControlBlock>()->container;
        thread_g = new IR::BFN::P4Thread();
        thread_g->mau = ghost_cb;
        toBlockInfo.emplace(ghost_cb, BlockInfo(index, pipeName, GHOST, MAU));
    }

    Pipeline pipeline(pipeName, thread_i, thread_e, thread_g);
    pipelines.addPipeline(index, pipeline, pipeName);
}

void ParseTna::parsePortMapAnnotation(const IR::PackageBlock *block, DefaultPortMap &map) {
    if (auto anno = block->node->getAnnotation("default_portmap"_cs)) {
        int index = 0;
        for (auto expr : anno->expr) {
            std::vector<int> ports;
            if (auto cst = expr->to<IR::Constant>()) {
                ports.push_back(cst->asInt());
            } else if (auto list = expr->to<IR::ListExpression>()) {
                for (auto expr : list->components) {
                    ports.push_back(expr->to<IR::Constant>()->asInt());
                }
            } else if (auto list = expr->to<IR::StructExpression>()) {
                for (auto expr : list->components) {
                    ports.push_back(expr->expression->to<IR::Constant>()->asInt());
                }
            }
            map[index] = ports;
            index++;
        }
    }
}

void ParseTna::parseMultipleParserInstances(const IR::PackageBlock *block, cstring pipe,
                                            IR::BFN::P4Thread *thread, gress_t gress) {
    int index = 0;
    DefaultPortMap map;
    parsePortMapAnnotation(block, map);
    for (auto param : block->constantValue) {
        if (!param.second) continue;
        if (!param.second->is<IR::ParserBlock>()) continue;
        auto p = param.first->to<IR::Parameter>();
        cstring archName = gress == INGRESS ? "ig_prsr"_cs : "eg_prsr"_cs;
        auto decl = block->node->to<IR::Declaration_Instance>();
        if (decl) archName = decl->controlPlaneName();
        archName = p ? archName + "." + p->name : "";

        BlockInfo block_info(index, pipe, gress, PARSER, archName);
        if (map.count(index) != 0)
            block_info.portmap.insert(block_info.portmap.end(), map[index].begin(),
                                      map[index].end());
        thread->parsers.push_back(param.second->to<IR::ParserBlock>()->container);
        toBlockInfo.emplace(param.second->to<IR::ParserBlock>()->container, block_info);
        index++;
    }
    hasMultipleParsers = true;
}

bool ParseTna::preorder(const IR::PackageBlock *block) {
    mainBlock = block;

    auto pos = 0;
    for (auto param : block->constantValue) {
        if (!param.second) continue;
        if (!param.second->is<IR::PackageBlock>()) continue;
        parseSingleParserPipeline(param.second->to<IR::PackageBlock>(), pos++);
        if (pos > 1) hasMultiplePipes = true;
    }
    return false;
}

const IR::Node *DoRewriteControlAndParserBlocks::postorder(IR::P4Parser *node) {
    auto orig = getOriginal();
    if (!block_info->count(orig)) {
        error(ErrorType::ERR_INVALID,
              "%1% parser. You are compiling for the %2% "
              "P4 architecture.\n"
              "Please verify that you included the correct architecture file.",
              node, BackendOptions().arch);
        return node;
    }

    auto *tnaBlocks = new IR::Vector<IR::Node>();
    // If the same IR::P4Parser block is used in multiple places, we need to
    // generate a new IR::BFN::Parser block for each use. E.g.
    // Pipeline(Parser(), Ingress(), Deparser(), Parser(), Egress(), Deparser());
    // The unique name for each IR::BFN::Parser block is determined by the gress.
    // IR::P4Parser used in different gress must have different names.

    // We first collect whether IR::P4Parser block is used in each gress, and
    // if the IR::P4Parser block is used in multiple gresses, we need to generate
    // a new IR::BFN::Parser block for each gress.
    std::set<gress_t> block_used_in_gress;
    for (auto &[_, blockinfo] : block_info->equal_range(orig))
        block_used_in_gress.insert(blockinfo.gress);

    // Once a block is generated for the gress, we need to keep track of the
    // generated block so that we can reuse it if the same IR::P4Parser block
    // is used in the same gress.
    std::map<gress_t, cstring> block_used_in_gress_generated;
    for (auto &[_, blockinfo] : block_info->equal_range(orig)) {
        cstring name = node->name;
        if (block_used_in_gress.size() > 1) name = refMap->newName(node->name.name.string());
        if (block_used_in_gress_generated.count(blockinfo.gress) != 0) {
            block_name_map.emplace(std::make_pair(blockinfo.pipe_name, blockinfo.block_index),
                                   block_used_in_gress_generated.at(blockinfo.gress));
            continue;
        }
        block_used_in_gress_generated.emplace(blockinfo.gress, name);
        const auto *parser = new IR::BFN::TnaParser(
            node->srcInfo, name, node->type, node->constructorParams, node->parserLocals,
            node->states, {}, blockinfo.gress, blockinfo.portmap);
        block_name_map.emplace(std::make_pair(blockinfo.pipe_name, blockinfo.block_index), name);
        tnaBlocks->push_back(parser);
    }
    return tnaBlocks;
}

const IR::Node *DoRewriteControlAndParserBlocks::postorder(IR::P4Control *node) {
    auto orig = getOriginal();
    if (!block_info->count(orig)) {
        error(ErrorType::ERR_INVALID,
              "%1% control. You are compiling for the %2% "
              "P4 architecture.\n"
              "Please verify that you included the correct architecture file.",
              node, BackendOptions().arch);
        return node;
    }

    auto *tnaBlocks = new IR::Vector<IR::Node>();
    // See comments in the above IR::P4Parser visitor.
    std::set<gress_t> block_used_in_gress;
    for (auto &[_, blockinfo] : block_info->equal_range(orig))
        block_used_in_gress.insert(blockinfo.gress);
    std::map<gress_t, cstring> block_used_in_gress_generated;
    for (auto &[_, blockinfo] : block_info->equal_range(orig)) {
        cstring name = node->name;
        if (block_used_in_gress.size() > 1) name = refMap->newName(node->name.name.string());
        if (block_used_in_gress_generated.count(blockinfo.gress) != 0) {
            block_name_map.emplace(std::make_pair(blockinfo.pipe_name, blockinfo.block_index),
                                   block_used_in_gress_generated.at(blockinfo.gress));
            continue;
        }
        block_used_in_gress_generated.emplace(blockinfo.gress, name);

        const auto type = node->type;
        const auto *nType = new IR::Type_Control(type->srcInfo, name, type->annotations,
                                                 type->typeParameters, type->applyParams);
        const IR::Node *tnaBlock = nullptr;
        if (blockinfo.block_type == ArchBlock_t::MAU) {
            tnaBlock = new IR::BFN::TnaControl(node->srcInfo, name, nType, node->constructorParams,
                                               node->controlLocals, node->body, {}, blockinfo.gress,
                                               blockinfo.pipe_name);
        } else if (blockinfo.block_type == ArchBlock_t::DEPARSER) {
            tnaBlock = new IR::BFN::TnaDeparser(node->srcInfo, name, nType, node->constructorParams,
                                                node->controlLocals, node->body, {},
                                                blockinfo.gress, blockinfo.pipe_name);
        } else {
            BUG("Unexpected block type %1%, should be MAU or DEPARSER", blockinfo.block_type);
        }
        block_name_map.emplace(std::make_pair(blockinfo.pipe_name, blockinfo.block_index), name);
        tnaBlocks->push_back(tnaBlock);
    }
    return tnaBlocks;
}

const IR::Node *DoRewriteControlAndParserBlocks::postorder(IR::Declaration_Instance *decl) {
    auto inst = P4::Instantiation::resolve(decl, refMap, typeMap);
    if (!inst->is<P4::PackageInstantiation>()) return decl;
    auto pkg = inst->to<P4::PackageInstantiation>()->package;
    if (pkg->name != "Pipeline") return decl;

    auto newArgs = new IR::Vector<IR::Argument>();
    int index = 0;
    for (auto arg : *decl->arguments) {
        const auto newName = ::get(block_name_map, std::make_pair(decl->name, index));
        CHECK_NULL(newName);
        const auto *typeRef = new IR::Type_Name(IR::ID(newName, nullptr));
        const auto *cc = arg->expression->to<IR::ConstructorCallExpression>();
        CHECK_NULL(cc);
        const auto *expr = new IR::ConstructorCallExpression(typeRef, cc->arguments);
        newArgs->push_back(new IR::Argument(expr));
        index++;
    }
    return new IR::Declaration_Instance(decl->srcInfo, decl->name, decl->annotations, decl->type,
                                        newArgs);
}

void add_param(ordered_map<cstring, cstring> &tnaParams, const IR::ParameterList *params,
               IR::ParameterList *newParams, cstring hdr, size_t index, cstring hdr_type = ""_cs,
               IR::Direction dir = IR::Direction::None) {
    if (params->parameters.size() > index) {
        auto *param = params->parameters.at(index);
        tnaParams.emplace(hdr, param->name);
    } else {
        // add optional parameter to parser and control type
        auto *annotations = new IR::Annotations();
        annotations->annotations.push_back(new IR::Annotation("optional"_cs, {}));
        newParams->push_back(
            new IR::Parameter(IR::ID(hdr), annotations, dir, new IR::Type_Name(IR::ID(hdr_type))));
        tnaParams.emplace(hdr, hdr);
    }
}

const IR::Node *RestoreParams::postorder(IR::BFN::TnaControl *control) {
    auto *params = control->type->getApplyParameters();
    ordered_map<cstring, cstring> tnaParams;
    auto *newParams = new IR::ParameterList();
    for (auto p : params->parameters) {
        newParams->push_back(p);
    }
    if (control->thread == INGRESS) {
        add_param(tnaParams, params, newParams, "hdr"_cs, 0);
        add_param(tnaParams, params, newParams, "ig_md"_cs, 1);
        add_param(tnaParams, params, newParams, "ig_intr_md"_cs, 2);
        add_param(tnaParams, params, newParams, "ig_intr_md_from_prsr"_cs, 3,
                  "ingress_intrinsic_metadata_from_parser_t"_cs, IR::Direction::In);
        add_param(tnaParams, params, newParams, "ig_intr_md_for_dprsr"_cs, 4,
                  "ingress_intrinsic_metadata_for_deparser_t"_cs, IR::Direction::InOut);
        add_param(tnaParams, params, newParams, "ig_intr_md_for_tm"_cs, 5,
                  "ingress_intrinsic_metadata_for_tm_t"_cs, IR::Direction::InOut);
        // Check for optional ghost_intrinsic_metadata_t for t2na arch
        // Note that we use the pkginfo annotation on the package to
        // determine if the ghost intrinsic metadata is present, because the
        // tna arch is designated to be the 'portable' architecture for
        // tofino, and programmer could compile a tna program for tofino2
        // with flags such as --target tofino2 --arch t2na.  In this case,
        // the ghost intrinsic metadata should not be present because the
        // program includes 'tna.p4', instead of 't2na.p4'
        if (Architecture::currentArchitecture() == Architecture::T2NA) {
            add_param(tnaParams, params, newParams, "gh_intr_md"_cs, 6,
                      "ghost_intrinsic_metadata_t"_cs, IR::Direction::In);
        }
    } else if (control->thread == EGRESS) {
        add_param(tnaParams, params, newParams, "hdr"_cs, 0);
        add_param(tnaParams, params, newParams, "eg_md"_cs, 1);
        add_param(tnaParams, params, newParams, "eg_intr_md"_cs, 2);
        add_param(tnaParams, params, newParams, "eg_intr_md_from_prsr"_cs, 3,
                  "egress_intrinsic_metadata_from_parser_t"_cs, IR::Direction::In);
        add_param(tnaParams, params, newParams, "eg_intr_md_for_dprsr"_cs, 4,
                  "egress_intrinsic_metadata_for_deparser_t"_cs, IR::Direction::InOut);
        add_param(tnaParams, params, newParams, "eg_intr_md_for_oport"_cs, 5,
                  "egress_intrinsic_metadata_for_output_port_t"_cs, IR::Direction::InOut);
    }

    auto newType = new IR::Type_Control(control->srcInfo, control->name, control->type->annotations,
                                        control->type->typeParameters, newParams);
    return new IR::BFN::TnaControl(control->srcInfo, control->name, newType,
                                   control->constructorParams, control->controlLocals,
                                   control->body, tnaParams, control->thread, control->pipeName);
}

const IR::Node *RestoreParams::postorder(IR::BFN::TnaParser *parser) {
    auto *params = parser->type->getApplyParameters();
    ordered_map<cstring, cstring> tnaParams;

    auto *newParams = new IR::ParameterList();
    for (auto p : params->parameters) {
        newParams->push_back(p);
    }
    if (parser->thread == INGRESS) {
        add_param(tnaParams, params, newParams, "pkt"_cs, 0);
        add_param(tnaParams, params, newParams, "hdr"_cs, 1);
        add_param(tnaParams, params, newParams, "ig_md"_cs, 2);
        add_param(tnaParams, params, newParams, "ig_intr_md"_cs, 3,
                  "ingress_intrinsic_metadata_t"_cs, IR::Direction::Out);
        add_param(tnaParams, params, newParams, "ig_intr_md_for_tm"_cs, 4,
                  "ingress_intrinsic_metadata_for_tm_t"_cs, IR::Direction::Out);
        add_param(tnaParams, params, newParams, "ig_intr_md_from_prsr"_cs, 5,
                  "ingress_intrinsic_metadata_from_parser_t"_cs, IR::Direction::Out);
    } else if (parser->thread == EGRESS) {
        add_param(tnaParams, params, newParams, "pkt"_cs, 0);
        add_param(tnaParams, params, newParams, "hdr"_cs, 1);
        add_param(tnaParams, params, newParams, "eg_md"_cs, 2);
        add_param(tnaParams, params, newParams, "eg_intr_md"_cs, 3,
                  "egress_intrinsic_metadata_t"_cs, IR::Direction::Out);
        add_param(tnaParams, params, newParams, "eg_intr_md_from_prsr"_cs, 4,
                  "egress_intrinsic_metadata_from_parser_t"_cs, IR::Direction::Out);
        add_param(tnaParams, params, newParams, "eg_intr_md_for_dprsr"_cs, 5,
                  "egress_intrinsic_metadata_for_deparser_t"_cs, IR::Direction::Out);
    }

    auto newType = new IR::Type_Parser(parser->name, parser->type->annotations,
                                       parser->type->typeParameters, newParams);
    return new IR::BFN::TnaParser(parser->srcInfo, parser->name, newType, parser->constructorParams,
                                  parser->parserLocals, parser->states, tnaParams, parser->thread,
                                  parser->phase0, parser->pipeName, parser->portmap);
}

const IR::Node *RestoreParams::postorder(IR::BFN::TnaDeparser *control) {
    auto *params = control->type->getApplyParameters();
    ordered_map<cstring, cstring> tnaParams;
    auto *newParams = new IR::ParameterList();
    for (auto p : params->parameters) {
        newParams->push_back(p);
    }
    if (control->thread == INGRESS) {
        add_param(tnaParams, params, newParams, "pkt"_cs, 0);
        add_param(tnaParams, params, newParams, "hdr"_cs, 1);
        add_param(tnaParams, params, newParams, "metadata"_cs, 2);
        add_param(tnaParams, params, newParams, "ig_intr_md_for_dprsr"_cs, 3,
                  "ingress_intrinsic_metadata_for_deparser_t"_cs, IR::Direction::In);
        add_param(tnaParams, params, newParams, "ig_intr_md"_cs, 4,
                  "ingress_intrinsic_metadata_t"_cs, IR::Direction::In);
    } else if (control->thread == EGRESS) {
        add_param(tnaParams, params, newParams, "pkt"_cs, 0);
        add_param(tnaParams, params, newParams, "hdr"_cs, 1);
        add_param(tnaParams, params, newParams, "metadata"_cs, 2);
        add_param(tnaParams, params, newParams, "eg_intr_md_for_dprsr"_cs, 3,
                  "egress_intrinsic_metadata_for_deparser_t"_cs, IR::Direction::In);
        add_param(tnaParams, params, newParams, "eg_intr_md"_cs, 4,
                  "egress_intrinsic_metadata_t"_cs, IR::Direction::In);
        add_param(tnaParams, params, newParams, "eg_intr_md_from_prsr"_cs, 5,
                  "egress_intrinsic_metadata_from_parser_t"_cs, IR::Direction::In);
    }

    auto newType = new IR::Type_Control(control->name, control->type->annotations,
                                        control->type->typeParameters, newParams);
    return new IR::BFN::TnaDeparser(control->srcInfo, control->name, newType,
                                    control->constructorParams, control->controlLocals,
                                    control->body, tnaParams, control->thread, control->pipeName);
}

}  // namespace BFN
