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

#include "bf-p4c/arch/v1model.h"

#include <fstream>
#include <map>
#include <set>

#include "bf-p4c/arch/bridge_metadata.h"
#include "bf-p4c/arch/fromv1.0/add_metadata_parser_states.h"
#include "bf-p4c/arch/fromv1.0/checksum.h"
#include "bf-p4c/arch/fromv1.0/parser_counter.h"
#include "bf-p4c/arch/program_structure.h"
#include "bf-p4c/device.h"
#include "bf-p4c/midend.h"
#include "bf-p4c/midend/type_checker.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "lib/bitops.h"
#include "midend/eliminateSerEnums.h"
#include "midend/local_copyprop.h"
#include "midend/validateProperties.h"

namespace BFN {

//////////////////////////////////////////////////////////////////////////////////////////////
namespace V1 {

/** \ingroup SimpleSwitchTranslation */
class FixupBackwardCompatibility : public Transform {
 public:
    FixupBackwardCompatibility() {}

    const IR::Node *postorder(IR::Parameter *param) {
        auto ctxt = findOrigCtxt<IR::Type_Control>();
        if (!ctxt) return param;
        if (ctxt->name != "EgressT") return param;
        if (auto tn = param->type->to<IR::Type_Name>()) {
            if (tn->path->name != "egress_intrinsic_metadata_t") return param;
            param->direction = IR::Direction::InOut;
        }
        return param;
    }
};

/** \ingroup SimpleSwitchTranslation */
class LoadTargetArchitecture : public Inspector {
    ProgramStructure *structure;

 public:
    explicit LoadTargetArchitecture(ProgramStructure *structure) : structure(structure) {
        CHECK_NULL(structure);
    }

    void setupMetadataRenameMap() {
        int portWidth = Device::portBitWidth();
        structure->addMetadata(
            INGRESS, MetadataField{"standard_metadata"_cs, "egress_spec"_cs, portWidth},
            MetadataField{"ig_intr_md_for_tm"_cs, "ucast_egress_port"_cs, portWidth});

        structure->addMetadata(EGRESS,
                               MetadataField{"standard_metadata"_cs, "egress_spec"_cs, portWidth},
                               MetadataField{"eg_intr_md"_cs, "egress_port"_cs, portWidth});

        structure->addMetadata(
            INGRESS, MetadataField{"standard_metadata"_cs, "egress_port"_cs, portWidth},
            MetadataField{"ig_intr_md_for_tm"_cs, "ucast_egress_port"_cs, portWidth});

        structure->addMetadata(EGRESS,
                               MetadataField{"standard_metadata"_cs, "egress_port"_cs, portWidth},
                               MetadataField{"eg_intr_md"_cs, "egress_port"_cs, portWidth});

        structure->addMetadata(MetadataField{"standard_metadata"_cs, "ingress_port"_cs, portWidth},
                               MetadataField{"ig_intr_md"_cs, "ingress_port"_cs, portWidth});

        structure->addMetadata(EGRESS,
                               MetadataField{"standard_metadata"_cs, "packet_length"_cs, 32},
                               MetadataField{"eg_intr_md"_cs, "pkt_length"_cs, 16});

        structure->addMetadata(
            MetadataField{"standard_metadata"_cs, "clone_spec"_cs, 32},
            MetadataField{COMPILER_META, "mirror_id"_cs, Device::cloneSessionIdWidth()});

        structure->addMetadata(INGRESS, MetadataField{"standard_metadata"_cs, "drop"_cs, 1},
                               MetadataField{"ig_intr_md_for_dprsr"_cs, "drop_ctl"_cs, 3});

        structure->addMetadata(EGRESS, MetadataField{"standard_metadata"_cs, "drop"_cs, 1},
                               MetadataField{"eg_intr_md_for_dprsr"_cs, "drop_ctl"_cs, 3});

        structure->addMetadata(INGRESS, MetadataField{"standard_metadata"_cs, "mcast_grp"_cs, 16},
                               MetadataField{"ig_intr_md_for_tm"_cs, "mcast_grp_a"_cs, 16});

        structure->addMetadata(INGRESS,
                               MetadataField{"standard_metadata"_cs, "checksum_error"_cs, 1},
                               MetadataField{"ig_intr_md_from_prsr"_cs, "parser_err"_cs, 1, 12});

        structure->addMetadata(EGRESS,
                               MetadataField{"standard_metadata"_cs, "checksum_error"_cs, 1},
                               MetadataField{"eg_intr_md_from_prsr"_cs, "parser_err"_cs, 1, 12});

        structure->addMetadata(MetadataField{"standard_metadata"_cs, "instance_type"_cs, 32},
                               MetadataField{COMPILER_META, "instance_type"_cs, 32});

        structure->addMetadata(INGRESS, MetadataField{"ig_intr_md_for_tm"_cs, "drop_ctl"_cs, 3},
                               MetadataField{"ig_intr_md_for_dprsr"_cs, "drop_ctl"_cs, 3});

        structure->addMetadata(EGRESS, MetadataField{"eg_intr_md_for_oport"_cs, "drop_ctl"_cs, 3},
                               MetadataField{"eg_intr_md_for_dprsr"_cs, "drop_ctl"_cs, 3});

        structure->addMetadata(
            MetadataField{"ig_intr_md_from_parser_aux"_cs, "ingress_global_tstamp"_cs, 48},
            MetadataField{"ig_intr_md_from_prsr"_cs, "global_tstamp"_cs, 48});

        structure->addMetadata(
            MetadataField{"standard_metadata"_cs, "ingress_global_timestamp"_cs, 48},
            MetadataField{"ig_intr_md_from_prsr"_cs, "global_tstamp"_cs, 48});

        structure->addMetadata(
            EGRESS, MetadataField{"eg_intr_md_from_parser_aux"_cs, "egress_global_tstamp"_cs, 48},
            MetadataField{"eg_intr_md_from_prsr"_cs, "global_tstamp"_cs, 48});

        structure->addMetadata(
            INGRESS, MetadataField{"ig_intr_md_from_parser_aux"_cs, "ingress_parser_err"_cs, 16},
            MetadataField{"ig_intr_md_from_prsr"_cs, "parser_err"_cs, 16});

        structure->addMetadata(
            EGRESS, MetadataField{"eg_intr_md_from_parser_aux"_cs, "egress_parser_err"_cs, 16},
            MetadataField{"eg_intr_md_from_prsr"_cs, "parser_err"_cs, 16});

        structure->addMetadata(
            EGRESS, MetadataField{"standard_metadata"_cs, "egress_global_timestamp"_cs, 48},
            MetadataField{"eg_intr_md_from_prsr"_cs, "global_tstamp"_cs, 48});

        structure->addMetadata(EGRESS, MetadataField{"standard_metadata"_cs, "enq_qdepth"_cs, 19},
                               MetadataField{"eg_intr_md"_cs, "enq_qdepth"_cs, 19});

        structure->addMetadata(EGRESS,
                               MetadataField{"standard_metadata"_cs, "enq_timestamp"_cs, 32},
                               MetadataField{"eg_intr_md"_cs, "enq_tstamp"_cs, 18});

        structure->addMetadata(EGRESS, MetadataField{"standard_metadata"_cs, "deq_qdepth"_cs, 19},
                               MetadataField{"eg_intr_md"_cs, "deq_qdepth"_cs, 19});

        structure->addMetadata(EGRESS, MetadataField{"eg_intr_md"_cs, "deq_timedelta"_cs, 32},
                               MetadataField{"eg_intr_md"_cs, "deq_timedelta"_cs, 18});

        structure->addMetadata(EGRESS,
                               MetadataField{"eg_intr_md_from_parser_aux"_cs, "clone_src"_cs, 4},
                               MetadataField{COMPILER_META, "clone_src"_cs, 4});

        structure->addMetadata(
            EGRESS, MetadataField{"eg_intr_md_from_parser_aux"_cs, "clone_digest_id"_cs, 4},
            MetadataField{COMPILER_META, "clone_digest_id"_cs, 4});

        structure->addMetadata(EGRESS, MetadataField{"standard_metadata"_cs, "egress_rid"_cs, 16},
                               MetadataField{"eg_intr_md"_cs, "egress_rid"_cs, 16});
#ifdef HAVE_JBAY
        structure->addMetadata(INGRESS,
                               MetadataField{"ig_intr_md_for_mb"_cs, "mirror_io_select"_cs, 1},
                               MetadataField{"ig_intr_md_for_dprsr"_cs, "mirror_io_select"_cs, 1});
        structure->addMetadata(EGRESS,
                               MetadataField{"eg_intr_md_for_mb"_cs, "mirror_io_select"_cs, 1},
                               MetadataField{"eg_intr_md_for_dprsr"_cs, "mirror_io_select"_cs, 1});
        structure->addMetadata(INGRESS, MetadataField{"ig_intr_md_for_mb"_cs, "mirror_hash"_cs, 13},
                               MetadataField{"ig_intr_md_for_dprsr"_cs, "mirror_hash"_cs, 13});
        structure->addMetadata(EGRESS, MetadataField{"eg_intr_md_for_mb"_cs, "mirror_hash"_cs, 13},
                               MetadataField{"eg_intr_md_for_dprsr"_cs, "mirror_hash"_cs, 13});
        structure->addMetadata(
            INGRESS, MetadataField{"ig_intr_md_for_mb"_cs, "mirror_ingress_cos"_cs, 3},
            MetadataField{"ig_intr_md_for_dprsr"_cs, "mirror_ingress_cos"_cs, 3});
        structure->addMetadata(
            EGRESS, MetadataField{"eg_intr_md_for_mb"_cs, "mirror_ingress_cos"_cs, 3},
            MetadataField{"eg_intr_md_for_dprsr"_cs, "mirror_ingress_cos"_cs, 3});
        structure->addMetadata(
            INGRESS, MetadataField{"ig_intr_md_for_mb"_cs, "mirror_deflect_on_drop"_cs, 1},
            MetadataField{"ig_intr_md_for_dprsr"_cs, "mirror_deflect_on_drop"_cs, 1});
        structure->addMetadata(
            EGRESS, MetadataField{"eg_intr_md_for_mb"_cs, "mirror_deflect_on_drop"_cs, 1},
            MetadataField{"eg_intr_md_for_dprsr"_cs, "mirror_deflect_on_drop"_cs, 1});

        // XXX(TF2LAB-105): disabled due to JBAY-A0 TM BUG
        // structure->addMetadata(
        //     INGRESS,
        //     MetadataField{"ig_intr_md_for_mb"_cs, "mirror_copy_to_cpu_ctrl"_cs, 1},
        //     MetadataField{"ig_intr_md_for_dprsr"_cs, "mirror_copy_to_cpu_ctrl"_cs, 1});
        // structure->addMetadata(
        //     EGRESS,
        //     MetadataField{"eg_intr_md_for_mb"_cs, "mirror_copy_to_cpu_ctrl"_cs, 1},
        //     MetadataField{"eg_intr_md_for_dprsr"_cs, "mirror_copy_to_cpu_ctrl"_cs, 1});
        structure->addMetadata(
            INGRESS, MetadataField{"ig_intr_md_for_mb"_cs, "mirror_multicast_ctrl"_cs, 1},
            MetadataField{"ig_intr_md_for_dprsr"_cs, "mirror_multicast_ctrl"_cs, 1});
        structure->addMetadata(
            EGRESS, MetadataField{"eg_intr_md_for_mb"_cs, "mirror_multicast_ctrl"_cs, 1},
            MetadataField{"eg_intr_md_for_dprsr"_cs, "mirror_multicast_ctrl"_cs, 1});
        structure->addMetadata(
            INGRESS, MetadataField{"ig_intr_md_for_mb"_cs, "mirror_egress_port"_cs, portWidth},
            MetadataField{"ig_intr_md_for_dprsr"_cs, "mirror_egress_port"_cs, portWidth});
        structure->addMetadata(
            EGRESS, MetadataField{"eg_intr_md_for_mb"_cs, "mirror_egress_port"_cs, portWidth},
            MetadataField{"eg_intr_md_for_dprsr"_cs, "mirror_egress_port"_cs, portWidth});
        structure->addMetadata(INGRESS, MetadataField{"ig_intr_md_for_mb"_cs, "mirror_qid"_cs, 7},
                               MetadataField{"ig_intr_md_for_dprsr"_cs, "mirror_qid"_cs, 7});
        structure->addMetadata(EGRESS, MetadataField{"eg_intr_md_for_mb"_cs, "mirror_qid"_cs, 7},
                               MetadataField{"eg_intr_md_for_dprsr"_cs, "mirror_qid"_cs, 7});
        structure->addMetadata(
            INGRESS, MetadataField{"ig_intr_md_for_mb"_cs, "mirror_coalesce_length"_cs, 8},
            MetadataField{"ig_intr_md_for_dprsr"_cs, "mirror_coalesce_length"_cs, 8});
        structure->addMetadata(
            EGRESS, MetadataField{"eg_intr_md_for_mb"_cs, "mirror_coalesce_length"_cs, 8},
            MetadataField{"eg_intr_md_for_dprsr"_cs, "mirror_coalesce_length"_cs, 8});
#endif
    }

    void analyzeTofinoModel() {
        for (auto decl : structure->targetTypes) {
            if (auto v = decl->to<IR::Type_Enum>()) {
                structure->enums.emplace(v->name, v);
            } else if (auto v = decl->to<IR::Type_SerEnum>()) {
                structure->ser_enums.emplace(v->name, v);
            } else if (auto v = decl->to<IR::Type_Error>()) {
                for (auto mem : v->members) {
                    structure->errors.emplace(mem->name);
                }
            }
        }
    }

    // TODO: Code to generate extra type declarations that we do not wish to publish yet.

    /* header pktgen_generic_header_t {
        bit<3> _pad0;
        bit<2> pipe_id;
        bit<3> app_id;
        bit<8> key_msb;
        bit<16> batch_id;
        bit<16> packet_id; } */
    const IR::Node *create_pktgen_generic_header_t() {
        auto fields = new IR::IndexedVector<IR::StructField>(
            {new IR::StructField("_pad0"_cs, IR::Type_Bits::get(3)),
             new IR::StructField("pipe_id"_cs, IR::Type_Bits::get(2)),
             new IR::StructField("app_id"_cs, IR::Type_Bits::get(3)),
             new IR::StructField("key_msb"_cs, IR::Type_Bits::get(8)),
             new IR::StructField("batch_id"_cs, IR::Type_Bits::get(16)),
             new IR::StructField("packet_id"_cs, IR::Type_Bits::get(16))});
        return new IR::Type_Header("pktgen_generic_header_t"_cs, *fields);
    }

    /* extern math_unit<T, U> {
        math_unit(bool invert, int<2> shift, int<6> scale, U data);
        T execute(in T x);
    } */
    const IR::Node *create_math_unit() {
        auto methods = new IR::Vector<IR::Method>({
            new IR::Method(
                "math_unit"_cs,
                new IR::Type_Method(
                    new IR::ParameterList({new IR::Parameter("invert"_cs, IR::Direction::None,
                                                             IR::Type_Boolean::get()),
                                           new IR::Parameter("shift"_cs, IR::Direction::None,
                                                             IR::Type_Bits::get(2, true)),
                                           new IR::Parameter("scale"_cs, IR::Direction::None,
                                                             IR::Type_Bits::get(6, true)),
                                           new IR::Parameter("data"_cs, IR::Direction::None,
                                                             new IR::Type_Name("U"_cs))}),
                    "math_unit"_cs),
                false),
            new IR::Method(
                "execute"_cs,
                new IR::Type_Method(new IR::Type_Name("T"_cs),
                                    new IR::ParameterList({new IR::Parameter(
                                        "x"_cs, IR::Direction::In, new IR::Type_Name("T"))}),
                                    "execute"_cs),
                false),
        });
        auto typeVars = new IR::IndexedVector<IR::Type_Var>(
            {new IR::Type_Var("T"_cs), new IR::Type_Var("U"_cs)});
        auto typeParams = new IR::TypeParameters(*typeVars);
        return new IR::Type_Extern("math_unit"_cs, typeParams, *methods);
    }

    void postorder(const IR::P4Program *) override {
        setupMetadataRenameMap();

        /// append device specific architecture definition

        // CODE_HACK
        // we are concatenating all the include files that define an architecture
        // because the current parsing does not take context into account, and thus
        // we can't safely separate metadata definitions (which are device specific)
        // from control block definitions (which are not device specific).
        char *drvP4IncludePath = getenv("P4C_16_INCLUDE_PATH");
        std::filesystem::path path(drvP4IncludePath ? drvP4IncludePath : p4includePath);
        char tempPath[PATH_MAX];
        snprintf(tempPath, PATH_MAX - 1, "/tmp/arch_XXXXXX.p4");
        std::vector<const char *> filenames;
        if (Device::currentDevice() == Device::TOFINO) {
            filenames.push_back("tofino1_specs.p4");
            filenames.push_back("tofino1_base.p4");
        }
#if HAVE_JBAY
        else if (Device::currentDevice() == Device::JBAY) {
            filenames.push_back("tofino2_specs.p4");
            filenames.push_back("tofino2_base.p4");
        }
#endif  // HAVE_JBAY
        else
            BUG("Unsupported device id %s"_cs, Device::currentDevice());
        filenames.push_back("tofino/stratum.p4");
        filenames.push_back("tofino/p4_14_prim.p4");

        // create a temporary file. The safe method is to create and open the file
        // in exclusive mode. Since we can only open a C++ stream with a hack, we'll close
        // the file and then open it again as an ofstream.
        auto fd = mkstemps(tempPath, 3);
        if (fd < 0) {
            error("Error mkstemp(%1%): %2%", tempPath, strerror(errno));
            return;
        }
        // close the file descriptor and open as stream
        close(fd);
        std::ofstream result(tempPath, std::ios::out);
        if (!result.is_open()) {
            error("Failed to open arch include file %1%", tempPath);
            return;
        }
        for (auto f : filenames) {
            std::filesystem::path fPath = path / f;
            std::ifstream inFile(fPath, std::ios::in);
            if (inFile.is_open()) {
                result << inFile.rdbuf();
                inFile.close();
            } else {
                error("Failed to open architecture include file %1%", fPath);
                result.close();
                unlink(tempPath);
                return;
            }
        }
        result.close();
        structure->include(cstring(tempPath), &structure->targetTypes);
        unlink(tempPath);

        // TODO: add extra type declaration that are not ready to publish
        structure->targetTypes.push_back(create_pktgen_generic_header_t());
        structure->targetTypes.push_back(create_math_unit());

        analyzeTofinoModel();

        if (structure->backward_compatible)
            structure->targetTypes = *(structure->targetTypes.apply(FixupBackwardCompatibility()));
    }
};

//////////////////////////////////////////////////////////////////////////////////////////////

/** \ingroup SimpleSwitchTranslation */
class RemoveNodesWithNoMapping : public Transform {
    // following fields are showing up in struct H, because in P4-14
    // these structs are declared as header type.
    // In TNA, most of these metadata are individual parameter to
    // the control block, and shall be removed from struct H.
    std::set<cstring> removeAllOccurences = {"generator_metadata_t_0"_cs,
                                             "ingress_parser_control_signals"_cs,
                                             "standard_metadata_t"_cs,
                                             "egress_intrinsic_metadata_t"_cs,
                                             "egress_intrinsic_metadata_for_mirror_buffer_t"_cs,
                                             "egress_intrinsic_metadata_for_output_port_t"_cs,
                                             "egress_intrinsic_metadata_from_parser_aux_t"_cs,
                                             "ingress_intrinsic_metadata_t"_cs,
                                             "ingress_intrinsic_metadata_for_mirror_buffer_t"_cs,
                                             "ingress_intrinsic_metadata_for_tm_t"_cs,
                                             "ingress_intrinsic_metadata_from_parser_aux_t"_cs};

    std::set<cstring> removeDeclarations = {
        "pktgen_generic_header_t"_cs, "pktgen_port_down_header_t"_cs, "pktgen_recirc_header_t"_cs,
        "pktgen_timer_header_t"_cs};

 public:
    RemoveNodesWithNoMapping() { setName("RemoveNodesWithNoMapping"); }

    const IR::Node *preorder(IR::Type_Header *node) {
        auto it = removeAllOccurences.find(node->name);
        if (it != removeAllOccurences.end()) return nullptr;
        it = removeDeclarations.find(node->name);
        if (it != removeDeclarations.end()) return nullptr;
        return node;
    }

    const IR::Node *preorder(IR::Type_Struct *node) {
        auto it = removeAllOccurences.find(node->name);
        if (it != removeAllOccurences.end()) return nullptr;
        it = removeDeclarations.find(node->name);
        if (it != removeDeclarations.end()) return nullptr;
        return node;
    }

    const IR::Node *preorder(IR::StructField *node) {
        auto header = findContext<IR::Type_Struct>();
        if (!header) return node;
        if (header->name != "headers") return node;

        auto type = node->type->to<IR::Type_Name>();
        if (!type) return node;

        auto it = removeAllOccurences.find(type->path->name);
        if (it != removeAllOccurences.end()) return nullptr;
        return node;
    }

    /// Given a IR::Member of the following structure:
    ///
    /// Member member=mcast_grp_b
    ///   type: Type_Bits size=16 isSigned=0
    ///   expr: Member member=ig_intr_md_for_tm
    ///     type: Type_Header name=ingress_intrinsic_metadata_for_tm_t
    ///     expr: PathExpression
    ///       type: Type_Name...
    ///       path: Path name=hdr absolute=0
    ///
    /// transform it to the following:
    ///
    /// Member member=mcast_grp_b
    ///   type: Type_Bits size=16 isSigned=0
    ///   expr: PathExpression
    ///     type: Type_Name...
    ///     path: Path name=ig_intr_md_for_tm
    const IR::Node *preorder(IR::Member *mem) {
        auto submem = mem->expr->to<IR::Member>();
        if (!submem) return mem;
        auto submemType = submem->type->to<IR::Type_Header>();
        if (!submemType) return mem;
        auto it = removeAllOccurences.find(submemType->name);
        if (it == removeAllOccurences.end()) return mem;

        auto newType = new IR::Type_Name(submemType->name);
        auto newName = new IR::Path(submem->member);
        auto newExpr = new IR::PathExpression(newType, newName);
        auto newMem = new IR::Member(mem->srcInfo, mem->type, newExpr, mem->member);
        return newMem;
    }
};

//////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * \ingroup SimpleSwitchTranslation
 * @pre: assume no nested control block or parser block,
 *       as a result, all declarations within a control block have different names.
 * @post: all user provided names for metadata are converted to standard names
 *       assumed by the translation map in latter pass.
 */
class NormalizeProgram : public Transform {
    ordered_map<cstring, std::vector<const IR::Node *> *> namescopes;
    ordered_map<cstring, cstring> renameMap;
    ordered_map<cstring, cstring> aliasMap;

 public:
    NormalizeProgram() {}

    void pushParam(const IR::Node *node, cstring param_type, cstring rename) {
        const IR::ParameterList *list;
        if (auto ctrl = node->to<IR::P4Control>()) {
            list = ctrl->type->getApplyParameters();
        } else if (auto parser = node->to<IR::P4Parser>()) {
            list = parser->type->getApplyParameters();
        } else {
            BUG("Unknown block type %1%"_cs, node);
        }

        for (auto p : list->parameters) {
            if (!p->type->is<IR::Type_Name>()) continue;
            auto type_name = p->type->to<IR::Type_Name>();
            auto type = type_name->path->name;
            if (type != param_type) continue;
            auto name = p->name;
            auto it = namescopes.find(name);
            if (it == namescopes.end()) {
                auto stack = new std::vector<const IR::Node *>();
                // We don't need to save something that does not need renaming
                if (name == rename) continue;
                stack->push_back(getOriginal<IR::Node>());
                namescopes.emplace(name, stack);
                renameMap.emplace(name, rename);
            } else {
                auto &stack = it->second;
                stack->push_back(getOriginal<IR::Node>());
            }
        }
    }

    void popParam(const IR::Node *node, cstring param_type) {
        const IR::ParameterList *list = nullptr;
        if (auto ctrl = node->to<IR::P4Control>()) {
            list = ctrl->type->getApplyParameters();
        } else if (auto parser = node->to<IR::P4Parser>()) {
            list = parser->type->getApplyParameters();
        }
        CHECK_NULL(list);

        for (auto p : list->parameters) {
            if (!p->type->is<IR::Type_Name>()) continue;
            auto type_name = p->type->to<IR::Type_Name>();
            auto type = type_name->path->name;
            if (type != param_type) continue;
            auto name = p->name;
            auto it = namescopes.find(name);
            if (it != namescopes.end()) {
                auto &stack = it->second;
                BUG_CHECK(stack->size() >= 1, "Cannot pop empty stack");
                stack->pop_back();
            }
        }
    }

    const IR::Node *preorder(IR::P4Control *node) override {
        pushParam(node, "standard_metadata_t"_cs, "standard_metadata"_cs);
        return node;
    }

    const IR::Node *postorder(IR::P4Control *node) override {
        popParam(node, "standard_metadata_t"_cs);
        return node;
    }

    const IR::Node *preorder(IR::P4Parser *node) override {
        pushParam(node, "standard_metadata_t"_cs, "standard_metadata"_cs);
        return node;
    }

    const IR::Node *postorder(IR::P4Parser *node) override {
        popParam(node, "standard_metadata_t"_cs);
        return node;
    }

    const IR::Node *preorder(IR::Declaration_Variable *node) override {
        auto name = node->name.name;
        auto it = namescopes.find(name);
        if (it != namescopes.end()) {
            auto &stack = it->second;
            stack->push_back(node->to<IR::Node>());
        }
        return node;
    }

    const IR::Node *postorder(IR::Declaration_Variable *node) override {
        auto name = node->name.name;
        auto it = namescopes.find(name);
        if (it != namescopes.end()) {
            auto &stack = it->second;
            stack->pop_back();
        }
        return node;
    }

    const IR::Node *postorder(IR::PathExpression *node) override {
        auto path = node->path->name;
        auto it = namescopes.find(path);
        if (it != namescopes.end()) {
            auto renameIt = renameMap.find(path);
            if (renameIt != renameMap.end()) {
                return new IR::PathExpression(node->srcInfo, node->type,
                                              new IR::Path(renameIt->second));
            }
        }
        return node;
    }

    const IR::Node *postorder(IR::Parameter *node) override {
        auto it = namescopes.find(node->name);
        if (it != namescopes.end()) {
            auto renameIt = renameMap.find(node->name);
            if (renameIt != renameMap.end()) {
                auto rename = renameIt->second;
                return new IR::Parameter(node->srcInfo, rename, node->annotations, node->direction,
                                         node->type);
            }
        }
        return node;
    }

    /**
     * recursively check if a fromName is aliased to toName
     */
    bool isStandardMetadata(cstring fromName) {
        auto it = aliasMap.find(fromName);
        if (it != aliasMap.end()) {
            auto toName_ = it->second;
            if (isStandardMetadata(toName_))
                return true;
            else if (toName_ == "standard_metadata_t")
                return true;
            else
                return false;
        } else {
            return false;
        }
    }

    const IR::Node *preorder(IR::Type_Typedef *node) override {
        if (auto name = node->type->to<IR::Type_Name>()) {
            aliasMap.emplace(node->name, name->path->name);  // std_meta_t -> standard_metadata_t
        }
        return node;
    }

    const IR::Node *postorder(IR::Type_Typedef *node) override {
        if (isStandardMetadata(node->name)) {
            return nullptr;
        }
        return node;
    }

    const IR::Node *postorder(IR::Type_Name *node) override {
        if (isStandardMetadata(node->path->name)) {
            return new IR::Type_Name(node->srcInfo, new IR::Path("standard_metadata_t"));
        }
        return node;
    }
};

//////////////////////////////////////////////////////////////////////////////////////////////

/** \ingroup SimpleSwitchTranslation
 * This pass collects all top level p4program declarations.
 */
class AnalyzeProgram : public Inspector {
    ProgramStructure *structure;

    template <class P4Type, class BlockType>
    void analyzeArchBlock(const IR::ToplevelBlock *blk, cstring name, cstring type) {
        auto main = blk->getMain();
        auto ctrl = main->findParameterValue(name);
        ERROR_CHECK(ctrl != nullptr, "%1%: could not find parameter %2%"_cs, main, name);
        ERROR_CHECK(ctrl->is<BlockType>(), "%1%: main package match the expected model", main);
        auto block = ctrl->to<BlockType>()->container;
        BUG_CHECK(block != nullptr, "Unable to find %1% block in V1Switch", name);
        structure->blockNames.emplace(type, block->name);
    }

 public:
    explicit AnalyzeProgram(ProgramStructure *structure) : structure(structure) {
        CHECK_NULL(structure);
        setName("AnalyzeProgram");
    }

    // *** architectural declarations ***
    // matchKindDeclaration
    // constantDeclarations
    // externDeclarations
    // parserDeclarations
    // controlDeclarations
    // *** program & architectural declarations ***
    // actionDeclarations
    void postorder(const IR::Type_Action *node) override {
        structure->action_types.push_back(node);
    }

    // errorDeclaration
    void postorder(const IR::Type_Error *node) override {
        for (auto m : node->members) {
            structure->errors.emplace(m->name);
        }
    }

    // typeDeclarations
    void postorder(const IR::Type_StructLike *node) override {
        structure->type_declarations.emplace(node->name, node);
    }

    void postorder(const IR::Type_Typedef *node) override {
        structure->type_declarations.emplace(node->name, node);
    }

    void postorder(const IR::Type_Enum *node) override {
        structure->type_declarations.emplace(node->name, node);
    }

    // *** program only declarations ***
    // instantiation - parser
    void postorder(const IR::P4Parser *node) override {
        structure->parsers.emplace(node->name, node);
        // additional info for translation
        auto params = node->getApplyParameters();
        if (params->parameters.size() >= 2) {
            structure->user_metadata = node->type->applyParams->parameters.at(2);
        } else {
            error("Parser in v1model must have at least 2 parameters");
        }
    }

    // instantiation - control
    void postorder(const IR::P4Control *node) override {
        structure->controls.emplace(node->name, node);
    }

    // instantiation - extern
    void postorder(const IR::Declaration_Instance *node) override {
        // look for global instances
        auto control = findContext<IR::P4Control>();
        if (control) return;
        auto parser = findContext<IR::P4Parser>();
        if (parser) return;
        // ignore main()
        if (auto type = node->type->to<IR::Type_Specialized>()) {
            auto typeName = type->baseType->to<IR::Type_Name>();
            if (typeName && typeName->path->name == "V1Switch") return;
        }
        structure->global_instances.emplace(node->name, node);
    }

    // instantiation - program -- check validity
    bool preorder(const IR::P4Program *) override {
        auto main = structure->toplevel->getMain();
        ERROR_CHECK(main != nullptr, ErrorType::ERR_INVALID,
                    "program: does not instantiate `main`");
        return true;
    }

    // instantiation - program
    void postorder(const IR::P4Program *) override {
        auto params = structure->toplevel->getMain()->getConstructorParameters();
        if (params->parameters.size() != 6) {
            std::stringstream msg;
            msg << "Expecting 6 parameters instead of " << params->parameters.size()
                << " in the 'main' instance." << std::endl;
            msg << "Did you specify the correct architecture using --arch?";

            error("%1%", msg.str());
            return;
        }
        analyzeArchBlock<IR::P4Parser, IR::ParserBlock>(structure->toplevel, "p"_cs,
                                                        ProgramStructure::INGRESS_PARSER);
        analyzeArchBlock<IR::P4Parser, IR::ParserBlock>(structure->toplevel, "p"_cs,
                                                        ProgramStructure::EGRESS_PARSER);
        analyzeArchBlock<IR::P4Control, IR::ControlBlock>(structure->toplevel, "ig"_cs,
                                                          ProgramStructure::INGRESS);
        analyzeArchBlock<IR::P4Control, IR::ControlBlock>(structure->toplevel, "eg"_cs,
                                                          ProgramStructure::EGRESS);
        analyzeArchBlock<IR::P4Control, IR::ControlBlock>(structure->toplevel, "dep"_cs,
                                                          ProgramStructure::INGRESS_DEPARSER);
        analyzeArchBlock<IR::P4Control, IR::ControlBlock>(structure->toplevel, "dep"_cs,
                                                          ProgramStructure::EGRESS_DEPARSER);
        analyzeArchBlock<IR::P4Control, IR::ControlBlock>(structure->toplevel, "vr"_cs,
                                                          "verifyChecksum"_cs);
        analyzeArchBlock<IR::P4Control, IR::ControlBlock>(structure->toplevel, "ck"_cs,
                                                          "updateChecksum"_cs);
    }

    void end_apply() override {
        // add 'compiler_generated_metadata_t'
        auto cgm = new IR::Type_Struct("compiler_generated_metadata_t"_cs);

        // Inject new fields for mirroring.
        cgm->fields.push_back(new IR::StructField(
            "mirror_id"_cs, IR::Type::Bits::get(Device::cloneSessionIdWidth())));
        cgm->fields.push_back(new IR::StructField("mirror_source"_cs, IR::Type::Bits::get(8)));
        // TODO: we can probably remove these two fields.
        cgm->fields.push_back(new IR::StructField("clone_src"_cs, IR::Type::Bits::get(4)));
        cgm->fields.push_back(new IR::StructField("clone_digest_id"_cs, IR::Type::Bits::get(4)));
        // Used to support separate egress_start parser state.
        cgm->fields.push_back(new IR::StructField("instance_type"_cs, IR::Type::Bits::get(32)));

        structure->type_declarations.emplace("compiler_generated_metadata_t"_cs, cgm);

        // Add an instance of parser counter
        {
            auto type = new IR::Type_Name("ParserCounter");
            auto decl = new IR::Declaration_Instance("ig_prsr_ctrl_parser_counter"_cs, type,
                                                     new IR::Vector<IR::Argument>());
            structure->ingressParserDeclarations.push_back(decl);
            structure->egressParserDeclarations.push_back(decl);
        }

        // Add an instance of parser priority
        {
            auto type = new IR::Type_Name("ParserPriority");
            auto decl = new IR::Declaration_Instance("ig_prsr_ctrl_priority"_cs, type,
                                                     new IR::Vector<IR::Argument>());
            structure->ingressParserDeclarations.push_back(decl);
            structure->egressParserDeclarations.push_back(decl);
        }
    }
};

//////////////////////////////////////////////////////////////////////////////////////////////

/** \ingroup SimpleSwitchTranslation */
class ConstructSymbolTable : public Inspector {
    ProgramStructure *structure;
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    const V1::TranslateParserChecksums *parserChecksums;
    /**
     * cloneIndex assignment algorithm:
     * - we assign a unique index per field list per gress
     * - there could be multiple actions calling clone within a table
     * - for a gress, a map of hashes is generated for a field list string
     * - the clone index is the value stored for hash key of element within the
     * map
     */
    std::map<unsigned long, unsigned> cloneIndexHashes[2];
    /**
     * resubmitIndex assignment algorithm:
     * - we assign a unique index per resubmit field list
     * - there could be multiple actions calling resubmit within a table
     * - can only be called in one gress
     * - a map of hashes is generated for a field list string
     * - the resubmit index is the value stored for hash key of element within
     * the map
     */
    std::map<unsigned long, unsigned> resubmitIndexHashes;
    std::set<unsigned> resubmitIndexSet;
    // digestIndex is similar to resubmitIndex and generate_digest() can only be called in ingress
    std::map<unsigned long, unsigned> digestIndexHashes;

    std::set<cstring> globals;

    // used to avoid create mirror.emit multiple times in the deparser
    std::set<std::pair<gress_t, unsigned>> dedupCloneIndex;
    std::set<std::pair<gress_t, unsigned>> dedupResubmitIndex;

    using DigestFieldInfo =
        std::tuple<cstring /* name */, const IR::Type *, const IR::Expression *>;

    const CollectGlobalPragma &collect_pragma;

    enum CloneFunctionType { CLONE = 0, CLONE3, CLONE_PRESERVING_FIELD_LIST };

 public:
    ConstructSymbolTable(ProgramStructure *structure, P4::ReferenceMap *refMap,
                         P4::TypeMap *typeMap, const V1::TranslateParserChecksums *parserChecksums,
                         const CollectGlobalPragma &collect_pragma)
        : structure(structure),
          refMap(refMap),
          typeMap(typeMap),
          parserChecksums(parserChecksums),
          collect_pragma(collect_pragma) {
        CHECK_NULL(structure);
        setName("ConstructSymbolTable");
    }

    cstring genUniqueName(cstring base_name) {
        auto uniqueName = cstring::make_unique(structure->unique_names, base_name, '_');
        structure->unique_names.insert(uniqueName);
        return uniqueName;
    }

    const IR::Type_Header *convertTupleTypeToHeaderType(cstring prefix,
                                                        const IR::Vector<IR::Type> *components,
                                                        bool first) {
        if (first) {
            auto fields = new IR::IndexedVector<IR::StructField>();
            int index = 0;
            for (auto t : *components) {
                cstring fname = "__field_"_cs + std::to_string(index);
                auto *fieldAnnotations = new IR::Annotations();
                if (index != 0)
                    fieldAnnotations->annotations.push_back(
                        new IR::Annotation(IR::ID("flexible"), {}));
                if (auto nestedTuple = t->to<IR::Type_BaseList>()) {
                    convertTupleTypeToHeaderType(prefix + fname, &nestedTuple->components, false);
                    cstring stName = prefix + fname + "_struct_t"_cs;
                    auto ttype = new IR::Type_Name(stName);
                    fields->push_back(new IR::StructField(IR::ID(fname), fieldAnnotations, ttype));
                } else if (t->is<IR::Type_Name>()) {
                    fields->push_back(new IR::StructField(IR::ID(fname), fieldAnnotations, t));
                } else {
                    auto ttype = IR::Type::Bits::get(t->width_bits());
                    fields->push_back(new IR::StructField(IR::ID(fname), fieldAnnotations, ttype));
                }
                index++;
            }
            cstring hdName = prefix + "_header_t"_cs;
            auto type = new IR::Type_Header(hdName, *fields);
            return type;
        } else {
            auto fields = new IR::IndexedVector<IR::StructField>();
            int index = 0;
            for (auto t : *components) {
                cstring fname = "__field_"_cs + std::to_string(index);
                if (auto nestedTuple = t->to<IR::Type_BaseList>()) {
                    convertTupleTypeToHeaderType(prefix + fname, &nestedTuple->components, false);
                    cstring stName = prefix + fname + "_struct_t"_cs;
                    auto *fieldAnnotations =
                        new IR::Annotations({new IR::Annotation(IR::ID("flexible"), {})});
                    fields->push_back(new IR::StructField(IR::ID(fname), fieldAnnotations,
                                                          new IR::Type_Name(stName)));
                } else {
                    auto *fieldAnnotations =
                        new IR::Annotations({new IR::Annotation(IR::ID("flexible"), {})});
                    fields->push_back(new IR::StructField(IR::ID(fname), fieldAnnotations,
                                                          IR::Type::Bits::get(t->width_bits())));
                }
                index++;
            }
            cstring stName = prefix + "_struct_t"_cs;
            auto type = new IR::Type_Struct(stName, *fields);
            structure->targetTypes.push_back(type);
        }
        return nullptr;
    }

    const IR::Type_Header *convertTypeNameToHeaderType(
        cstring prefix, const IR::Type_Name *st, const IR::Type_BaseList *fixedPositionTypes) {
        cstring hdName = prefix + "_header_t"_cs;
        auto fields = new IR::IndexedVector<IR::StructField>();
        int index = 0;
        for (auto t : fixedPositionTypes->components) {
            cstring fname = "__field_"_cs + std::to_string(index);
            auto ttype = IR::Type::Bits::get(t->width_bits());
            auto *fieldAnnotations =
                new IR::Annotations({new IR::Annotation(IR::ID("flexible"), {})});
            fields->push_back(new IR::StructField(IR::ID(fname), fieldAnnotations, ttype));
            index++;
        }
        cstring fname = "__field_"_cs + std::to_string(index);
        fields->push_back(new IR::StructField(IR::ID(fname), st));
        return new IR::Type_Header(hdName, *fields);
    }

    /**
     * V1model represents field list as a list expression, which is not sufficient
     * for tofino header repacking optimization. This function creates a header type
     * based on the list expression, which is then used by the backend header repacking
     * algorithm to optimize the header layout. This header type should be internal
     * to the compiler. No compiler output should rely on the generated header type.
     */
    const IR::StructExpression *convertFieldList(
        cstring prefix, std::vector<DigestFieldInfo> *digestFieldsFromSource,
        std::vector<DigestFieldInfo> *digestFieldsGeneratedByCompiler = nullptr,
        bool isHeader = true) {
        // initialize the header variable with structure initializer
        auto initializer = new IR::IndexedVector<IR::NamedExpression>();
        if (digestFieldsGeneratedByCompiler != nullptr) {
            for (auto f : *digestFieldsGeneratedByCompiler) {
                auto elem = new IR::NamedExpression(std::get<0>(f), std::get<2>(f));
                initializer->push_back(elem);
            }
        }
        for (auto f : *digestFieldsFromSource) {
            LOG3(" source type " << std::get<1>(f));
            /// if there is a nested tuple, convert it to a nested StructExpression
            /// and do so recursively.
            if (std::get<1>(f)->is<IR::Type_BaseList>()) {
                auto list = std::get<2>(f)->to<IR::ListExpression>();
                BUG_CHECK(list, "Expected a ListExpression");
                auto digestFieldsFromTuple = new std::vector<DigestFieldInfo>();
                int index = 0;
                for (auto em : list->components) {
                    cstring fname = "__field_"_cs + std::to_string(index);
                    DigestFieldInfo info = std::make_tuple(fname, em->type, em);
                    digestFieldsFromTuple->push_back(info);
                    index++;
                }
                auto nestedPrefix = prefix + std::get<0>(f);
                auto fl = convertFieldList(nestedPrefix, digestFieldsFromTuple, nullptr, false);
                auto elem = new IR::NamedExpression(std::get<0>(f), fl);
                initializer->push_back(elem);
            } else if (std::get<2>(f)->is<IR::StructExpression>()) {
                initializer->append(std::get<2>(f)->to<IR::StructExpression>()->components);
            } else {
                auto elem = new IR::NamedExpression(std::get<0>(f), std::get<2>(f));
                initializer->push_back(elem);
            }
        }
        cstring headerTypeName = prefix + (isHeader ? "_header_t"_cs : "_struct_t"_cs);
        return new IR::StructExpression(new IR::Type_Name(headerTypeName), *initializer);
    }

    /*
     * following extern methods in v1model.p4 need to be converted to an extern instance
     * and a method call on the instance.
     * random, digest, mark_to_drop, hash, verify_checksum, update_checksum, resubmit
     * recirculate, clone, clone3, clone_preserving_field_list, truncate
     */
    void cvtDigestFunction(const IR::MethodCallStatement *node) {
        /*
         * translate digest() function in ingress control block to
         *
         * ig_intr_md_for_dprsr.digest_type = n;
         *
         */
        auto mce = node->methodCall->to<IR::MethodCallExpression>();
        BUG_CHECK(mce != nullptr, "malformed IR in digest() function");
        auto control = findContext<IR::P4Control>();
        ERROR_CHECK(control != nullptr, "digest() must be used in a control block");
        ERROR_CHECK(control->name == structure->getBlockName(ProgramStructure::INGRESS),
                    "digest() can only be used in %1%",
                    structure->getBlockName(ProgramStructure::INGRESS));
        IR::PathExpression *path = new IR::PathExpression("ig_intr_md_for_dprsr");
        auto mem = new IR::Member(path, "digest_type");
        unsigned digestId = getIndex(node, digestIndexHashes);
        if (digestId > Device::maxDigestId()) {
            error("Too many digest() calls in %1%", control->name);
            return;
        }
        unsigned bits = static_cast<unsigned>(std::ceil(std::log2(Device::maxDigestId())));
        auto idx = new IR::Constant(IR::Type::Bits::get(bits), digestId);
        auto stmt = new IR::AssignmentStatement(mem, idx);
        structure->_map.emplace(node, stmt);

        ERROR_CHECK(mce->typeArguments->size() == 1, "Expected 1 type parameter for %1%",
                    mce->method);
        auto *typeArg = mce->typeArguments->at(0);
        auto *typeName = typeArg->to<IR::Type_Name>();
        ERROR_CHECK(typeName != nullptr, "Expected type T in digest to be a typeName %1%", typeArg);
        auto fieldList = refMap->getDeclaration(typeName->path);
        auto declAnno = fieldList->getAnnotation("name"_cs);

        ERROR_CHECK(typeName != nullptr, "Wrong argument type for %1%", typeArg);
        /*
         * Add header definition for top level
         * header __digest_header_t {
         *   ...
         * }
         * In the ingress deparser, add the following code
         * Digest<T>() learn_1;
         * if (ig_intr_md_for_dprsr.digest_type == n)
         *    learn_1.pack({...});
         *
         */
        auto field_list = mce->arguments->at(1);

        if (field_list->expression->is<IR::Type_BaseList>()) {
            for (auto t : field_list->expression->to<IR::ListExpression>()->components) {
                LOG3("name " << t << " type " << t->type);
            }
        } else if (field_list->expression->is<IR::StructExpression>()) {
            for (auto t : field_list->expression->to<IR::StructExpression>()->components) {
                LOG3("name " << t->expression << " type " << t->expression->type);
            }
        }

        if (field_list->expression->is<IR::Type_BaseList>() ||
            field_list->expression->is<IR::Type_Name>()) {
            cstring uniqName = genUniqueName("__digest"_cs);
            // createNewType
            // createNewFieldList

            auto fieldList = convertFieldList(uniqName, {}, {});
            auto args = new IR::Vector<IR::Argument>(new IR::Argument(fieldList));
            auto expr = new IR::PathExpression(new IR::Path(typeName->path->name));
            auto member = new IR::Member(expr, "pack");
            auto typeArgs = new IR::Vector<IR::Type>();
            auto mcs =
                new IR::MethodCallStatement(new IR::MethodCallExpression(member, typeArgs, args));

            auto condExprPath = new IR::Member(
                new IR::PathExpression(new IR::Path("ig_intr_md_for_dprsr")), "digest_type");
            auto condExpr = new IR::Equ(condExprPath, idx);
            auto cond = new IR::IfStatement(condExpr, mcs, nullptr);
            structure->ingressDeparserStatements.push_back(cond);

            auto declArgs = new IR::Vector<IR::Argument>({});
            auto declTypeArgs = new IR::Vector<IR::Type>();
            declTypeArgs->push_back(new IR::Type_Name(IR::ID(uniqName + "_header_t")));
            auto declType = new IR::Type_Specialized(new IR::Type_Name("Digest"), declTypeArgs);
            auto annotations = declAnno ? new IR::Annotations({declAnno}) : new IR::Annotations();
            auto decl =
                new IR::Declaration_Instance(typeName->path->name, annotations, declType, declArgs);
            structure->ingressDeparserDeclarations.push_back(decl);
        } else if (auto st = field_list->expression->to<IR::StructExpression>()) {
            auto args = new IR::Vector<IR::Argument>(new IR::Argument(st));
            auto expr = new IR::PathExpression(new IR::Path(typeName->path->name));
            auto member = new IR::Member(expr, "pack");
            auto typeArgs = new IR::Vector<IR::Type>();
            auto mcs =
                new IR::MethodCallStatement(new IR::MethodCallExpression(member, typeArgs, args));

            auto condExprPath = new IR::Member(
                new IR::PathExpression(new IR::Path("ig_intr_md_for_dprsr")), "digest_type");
            auto condExpr = new IR::Equ(condExprPath, idx);
            auto cond = new IR::IfStatement(condExpr, mcs, nullptr);
            structure->ingressDeparserStatements.push_back(cond);

            auto declArgs = new IR::Vector<IR::Argument>({});
            auto declTypeArgs = new IR::Vector<IR::Type>();
            declTypeArgs->push_back(st->type);
            auto declType = new IR::Type_Specialized(new IR::Type_Name("Digest"), declTypeArgs);
            auto annotations = declAnno ? new IR::Annotations({declAnno}) : new IR::Annotations();
            auto decl =
                new IR::Declaration_Instance(typeName->path->name, annotations, declType, declArgs);
            structure->ingressDeparserDeclarations.push_back(decl);
        }
    }

    /**
     * invalidate_digest
     *
     * translate the invalid_digest() call in ingress control block to
     * invalidate(ig_intr_md_for_dprsr.digest_type)
     *
     */
    void cvtInvalidateDigestFunction(const IR::MethodCallStatement *node) {
        /*
         * \TODO: Check that generate_digest has been called before
         */
        auto mce = node->methodCall->to<IR::MethodCallExpression>();
        BUG_CHECK(mce != nullptr, "malformed IR in digest() function");
        auto control = findContext<IR::P4Control>();
        ERROR_CHECK(control != nullptr, "digest() must be used in a control block");
        ERROR_CHECK(control->name == structure->getBlockName(ProgramStructure::INGRESS),
                    "digest() can only be used in %1%",
                    structure->getBlockName(ProgramStructure::INGRESS));
        IR::PathExpression *path = new IR::PathExpression("ig_intr_md_for_dprsr");
        auto mem = new IR::Member(IR::Type::Bits::get(3), path, "digest_type");
        auto stmt = new IR::MethodCallStatement(
            node->srcInfo,
            new IR::MethodCallExpression(
                node->srcInfo, new IR::PathExpression(IR::ID(node->srcInfo, "invalidate"_cs)),
                new IR::Vector<IR::Type>({mem->type}),
                new IR::Vector<IR::Argument>({new IR::Argument(mem)})));
        structure->_map.emplace(node, stmt);
    }

    void addResidualFields(cstring cloneHeaderName,
                           std::vector<DigestFieldInfo> *digestFieldsFromSource,
                           cstring cloneType) {
        if (parserChecksums->needBridging.empty()) return;
        auto header = structure->type_declarations.at(cloneHeaderName)->to<IR::Type_Header>();
        auto fields = new IR::IndexedVector<IR::StructField>();
        for (auto f : header->fields) {
            fields->push_back(f);
        }
        int idx = fields->size();
        for (auto &gressToStateMap : parserChecksums->destToGressToState) {
            if (gressToStateMap.second.count(EGRESS)) {
                // First check if the residual checksum is in the clone path
                // If yes, then add residual in the field list
                auto checsksumState = gressToStateMap.second.at(EGRESS);
                auto parser = parserChecksums->parserGraphs.get_parser(checsksumState);
                cstring cloneEntry;
                if (cloneType == "E2E") {
                    cloneEntry = "start_e2e_mirrored"_cs;
                } else {
                    cloneEntry = "start_i2e_mirrored"_cs;
                }
                auto cloneEntryPoint = parserChecksums->parserGraphs.get_state(parser, cloneEntry);
                if (cloneEntryPoint) {
                    if (!parserChecksums->parserGraphs.is_descendant(checsksumState->name,
                                                                     cloneEntry)) {
                        continue;
                    }
                }
                cstring fieldName = "__field_"_cs + std::to_string(idx);
                fields->push_back(new IR::StructField(IR::ID(fieldName), IR::Type::Bits::get(16)));
                auto tpl =
                    std::make_tuple(fieldName, IR::Type::Bits::get(16),
                                    parserChecksums->residualChecksums.at(gressToStateMap.first));
                digestFieldsFromSource->push_back(tpl);
                idx++;
            }
        }
        auto newHeader = new IR::Type_Header(cloneHeaderName, *fields);
        structure->type_declarations[cloneHeaderName] = newHeader;
        return;
    }

    void cvtCloneFunction(const IR::MethodCallStatement *node, CloneFunctionType cloneFuncType) {
        LOG1("cvtCloneFunction: (id= " << node->id << " ) " << node);
        auto mce = node->methodCall->to<IR::MethodCallExpression>();
        BUG_CHECK(mce != nullptr, "malformed IR in clone() function");
        auto control = findContext<IR::P4Control>();
        ERROR_CHECK(control != nullptr, "clone() must be used in a control block");

        const bool isIngress =
            (control->name == structure->getBlockName(ProgramStructure::INGRESS));
        const gress_t gress = isIngress ? INGRESS : EGRESS;
        auto *deparserMetadataPath =
            new IR::PathExpression(isIngress ? "ig_intr_md_for_dprsr" : "eg_intr_md_for_dprsr");
        auto *compilerMetadataPath = new IR::PathExpression(COMPILER_META);
        // Generate a fresh index for this clone field list. This is used by the
        // hardware to select the correct mirror table entry, and to select the
        // correct parser for this field list.
        unsigned cloneId = getIndex(node, cloneIndexHashes[gress]);
        // COMPILER-914: In Tofino, Disable clone id - 0 which is reserved in
        // i2e due to a hardware bug. Hence, valid clone ids are 1 - 7.  All
        // clone id's 0 - 7 are valid for e2e
        // Using @disable_reserved_i2e_drop_implementation pragma overrides
        // reserving clone id 0. This pragma is supported by Glass.
        if ((Device::currentDevice() == Device::TOFINO) && (gress == INGRESS) &&
            !collect_pragma.exists(PragmaDisableI2EReservedDropImplementation::name)) {
            cloneId++;
        }

        unsigned bits = static_cast<unsigned>(std::ceil(std::log2(Device::maxCloneId(gress))));
        auto *idx = new IR::Constant(IR::Type::Bits::get(bits), cloneId);

        // mirror type and mirror source are prepended to mirror field list.
        auto digestFieldsGeneratedByCompiler = new std::vector<DigestFieldInfo>();
        {
            auto *block = new IR::BlockStatement;

            // Construct a value for `mirror_source`, which is
            // compiler-generated metadata that's prepended to the user field
            // list. Its layout (in network order) is:
            //   [  0    1       2          3         4       5    6   7 ]
            //     [unused] [coalesced?] [gress] [mirrored?] [mirror_type]
            // Here `gress` is 0 for I2E mirroring and 1 for E2E mirroring.
            //
            // This information is used to set intrinsic metadata in the egress
            // parser. The `mirrored?` bit is particularly important; if that
            // bit is zero, the egress parser expects the following bytes to be
            // bridged metadata rather than mirrored fields.
            //
            // TODO: Glass is able to reuse `mirror_type` for last three
            // bits of this data, which eliminates the need for an extra PHV
            // container. We'll start doing that soon as well, but we need to
            // work out some issues with PHV allocation constraints first.
            auto *mirrorSource = new IR::Member(compilerMetadataPath, "mirror_source");
            const unsigned sourceIdx = idx->asInt();
            const unsigned isMirroredTag = 1 << 3;
            const unsigned gressTag = isIngress ? 0 : 1 << 4;
            auto *source =
                new IR::Constant(IR::Type::Bits::get(8), sourceIdx | isMirroredTag | gressTag);
            block->components.push_back(new IR::AssignmentStatement(mirrorSource, source));
            auto tpl = std::make_tuple("__field_0"_cs, IR::Type::Bits::get(8), mirrorSource);
            digestFieldsGeneratedByCompiler->push_back(tpl);

            // Set `mirror_type`, which is used as the digest selector in the
            // deparser (in other words, it selects the field list to use).
            auto *mirrorIdx = new IR::Member(deparserMetadataPath, "mirror_type");
            block->components.push_back(new IR::AssignmentStatement(mirrorIdx, idx));

            // Set `mirror_id`, which configures the mirror session id that the
            // hardware uses to route mirrored packets in the TM.
            ERROR_CHECK(mce->arguments->size() >= 2, "No mirror session id specified: %1%", mce);
            auto *mirrorId = new IR::Member(compilerMetadataPath, "mirror_id");
            auto *mirrorIdValue = mce->arguments->at(1)->expression;
            /// v1model mirror_id is 32bit, cast to bit<10>
            auto *castedMirrorIdValue =
                new IR::Cast(IR::Type::Bits::get(Device::cloneSessionIdWidth()), mirrorIdValue);
            block->components.push_back(new IR::AssignmentStatement(mirrorId, castedMirrorIdValue));
            structure->_map.emplace(node, block);
        }

        /*
         * generate statement in ingress/egress deparser to prepend mirror metadata
         *
         * Mirror mirror();
         * if (ig_intr_md_for_dprsr.mirror_type == N)
         *    mirror.emit({});
         */

        // Only instantiate the extern for the first instance of clone()
        auto findDecl = [](std::vector<const IR::Declaration *> decls,
                           cstring name) -> const IR::Declaration * {
            for (auto d : decls)
                if (d->getName() == name) return d;
            return nullptr;
        };

        const IR::Declaration *mirrorDeclared =
            isIngress ? findDecl(structure->ingressDeparserDeclarations, "mirror"_cs)
                      : findDecl(structure->egressDeparserDeclarations, "mirror"_cs);

        if (mirrorDeclared == nullptr) {
            auto declArgs = new IR::Vector<IR::Argument>({});
            auto declType = new IR::Type_Name("Mirror");
            auto decl = new IR::Declaration_Instance("mirror"_cs, declType, declArgs);
            if (isIngress)
                structure->ingressDeparserDeclarations.push_back(decl);
            else
                structure->egressDeparserDeclarations.push_back(decl);
        }

        if (dedupCloneIndex.count(std::make_pair(gress, cloneId)))
            return;
        else
            dedupCloneIndex.insert(std::make_pair(gress, cloneId));

        cstring generatedCloneHeaderTypeName = genUniqueName("__clone"_cs);
        cstring cloneHeaderName;
        cstring cloneType = mce->arguments->at(0)->expression->to<IR::Member>()->member;
        auto digestFieldsFromSource = new std::vector<DigestFieldInfo>();
        // clone3 is depracated
        if (mce->typeArguments->size() > 0) {
            if (cloneFuncType == CLONE3) {
                auto fl = mce->arguments->at(2)->to<IR::Argument>();
                BUG_CHECK(fl != nullptr, "invalid clone3 method argument");
                if (auto list = fl->expression->to<IR::ListExpression>()) {
                    if (auto tpl_type = fl->expression->type->to<IR::Type_BaseList>()) {
                        // generate a header type from tuple, used by repacking algorithm
                        auto components = new IR::Vector<IR::Type>();
                        components->push_back(IR::Type::Bits::get(8));  // mirror_source
                        components->append(tpl_type->components);
                        auto header_type = convertTupleTypeToHeaderType(
                            generatedCloneHeaderTypeName, components, true);
                        structure->type_declarations.emplace(header_type->name, header_type);
                        LOG3("create header " << header_type->name.name);
                        cloneHeaderName = header_type->name;
                        // generate a struct initializer for the header type
                        int index = 1;  // first element was used for mirror_source
                        for (auto elem : list->components) {
                            cstring fname = "__field_"_cs + std::to_string(index);
                            LOG3("name " << fname << " type " << elem->type << " expr " << elem);
                            DigestFieldInfo info = std::make_tuple(fname, elem->type, elem);
                            digestFieldsFromSource->push_back(info);
                            index++;
                        }
                    } else if (auto type_name = fl->expression->type->to<IR::Type_Name>()) {
                        auto header_type = convertTypeNameToHeaderType(generatedCloneHeaderTypeName,
                                                                       type_name, {});
                        structure->type_declarations.emplace(header_type->name, header_type);
                    } else {
                        error(ErrorType::ERR_UNEXPECTED, "clone field list %1% not supported ",
                              fl->expression);
                    }
                } else if (auto path = fl->expression->to<IR::PathExpression>()) {
                    DigestFieldInfo info = std::make_tuple(path->path->name, path->type, path);
                    digestFieldsFromSource->push_back(info);
                } else if (auto mem = fl->expression->to<IR::Member>()) {
                    DigestFieldInfo info = std::make_tuple(mem->member.name, mem->type, mem);
                    digestFieldsFromSource->push_back(info);
                }
            }
        } else if (cloneFuncType == CLONE_PRESERVING_FIELD_LIST) {
            // Look into the third argument (field list index)
            auto arg = mce->arguments->at(2)->to<IR::Argument>();
            BUG_CHECK(arg != nullptr, "invalid clone_preserving_field_list method argument");
            convertPreservingFieldList(arg, control, "clone"_cs, generatedCloneHeaderTypeName,
                                       &cloneHeaderName, digestFieldsFromSource);
        } else {
            // has no data
            cstring hdName = generatedCloneHeaderTypeName + "_header_t"_cs;
            auto fields = new IR::IndexedVector<IR::StructField>();
            fields->push_back(new IR::StructField(IR::ID("__field_0"), IR::Type::Bits::get(8)));
            auto type = new IR::Type_Header(hdName, *fields);
            cloneHeaderName = hdName;
            structure->type_declarations.emplace(hdName, type);
        }
        addResidualFields(cloneHeaderName, digestFieldsFromSource, cloneType);

        const IR::StructExpression *fieldList = convertFieldList(
            generatedCloneHeaderTypeName, digestFieldsFromSource, digestFieldsGeneratedByCompiler);

        auto args = new IR::Vector<IR::Argument>();
        args->push_back(new IR::Argument(new IR::Member(compilerMetadataPath, "mirror_id")));
        args->push_back(new IR::Argument(fieldList));

        auto pathExpr = new IR::PathExpression(new IR::Path("mirror"));
        auto member = new IR::Member(pathExpr, "emit");
        auto typeArgs = new IR::Vector<IR::Type>(
            {new IR::Type_Name(IR::ID(generatedCloneHeaderTypeName + "_header_t"))});
        auto mcs =
            new IR::MethodCallStatement(new IR::MethodCallExpression(member, typeArgs, args));
        auto condExprPath = new IR::Member(deparserMetadataPath, "mirror_type");
        auto condExpr = new IR::Equ(condExprPath, idx);
        auto cond = new IR::IfStatement(condExpr, mcs, nullptr);
        if (isIngress)
            structure->ingressDeparserStatements.push_back(cond);
        else
            structure->egressDeparserStatements.push_back(cond);
    }

    void cvtDropFunction(const IR::MethodCallStatement *node) {
        auto control = findContext<IR::P4Control>();
        if (control && control->name == structure->getBlockName(ProgramStructure::INGRESS)) {
            auto path = new IR::Member(new IR::PathExpression("ig_intr_md_for_dprsr"), "drop_ctl");
            auto val = new IR::Constant(IR::Type::Bits::get(3), 1);
            auto stmt = new IR::AssignmentStatement(path, val);
            structure->_map.emplace(node, stmt);
        } else if (control && control->name == structure->getBlockName(ProgramStructure::EGRESS)) {
            auto path = new IR::Member(new IR::PathExpression("eg_intr_md_for_dprsr"), "drop_ctl");
            auto val = new IR::Constant(IR::Type::Bits::get(3), 1);
            auto stmt = new IR::AssignmentStatement(path, val);
            structure->_map.emplace(node, stmt);
        }
    }

    const IR::Expression *cast_if_needed(const IR::Expression *expr, int srcWidth, int dstWidth) {
        if (srcWidth == dstWidth) return expr;
        if (srcWidth < dstWidth) return new IR::Cast(IR::Type::Bits::get(dstWidth), expr);
        if (srcWidth > dstWidth) return new IR::Slice(expr, dstWidth - 1, 0);
        return expr;
    }

    /// execute_meter_with_color is converted to a meter extern
    void cvtExecuteMeterFunction(const IR::MethodCallStatement *node) {
        auto control = findContext<IR::P4Control>();
        ERROR_CHECK(control != nullptr,
                    "execute_meter_with_color() must be used in a control block");
        auto mce = node->methodCall->to<IR::MethodCallExpression>();

        auto inst = mce->arguments->at(0)->expression->to<IR::PathExpression>();
        BUG_CHECK(inst != nullptr, "Invalid meter instance %1%", inst);
        auto path = inst->to<IR::PathExpression>()->path;
        auto pathExpr = new IR::PathExpression(path->name);

        auto method = new IR::Member(node->srcInfo, pathExpr, "execute");
        auto args = new IR::Vector<IR::Argument>();
        auto index_arg = mce->arguments->at(1);
        args->push_back(new IR::Argument(index_arg->srcInfo, "index"_cs, index_arg->expression));
        auto pre_color_arg = mce->arguments->at(3);
        auto pre_color_size = pre_color_arg->expression->type->width_bits();
        auto pre_color_expr = pre_color_arg->expression;
        auto castedExpr = cast_if_needed(pre_color_expr, pre_color_size, 8);
        args->push_back(
            new IR::Argument(pre_color_arg->srcInfo, "color"_cs,
                             new IR::Cast(new IR::Type_Name("MeterColor_t"), castedExpr)));
        auto methodCall = new IR::MethodCallExpression(node->srcInfo, method, args);

        auto meterColor = mce->arguments->at(2)->expression;
        auto meter_color_size = meterColor->type->width_bits();
        IR::AssignmentStatement *assign = nullptr;
        if (meter_color_size >= 8) {
            assign = new IR::AssignmentStatement(
                meterColor, new IR::Cast(IR::Type::Bits::get(meter_color_size), methodCall));
        } else {
            assign = new IR::AssignmentStatement(
                meterColor, new IR::Slice(methodCall, meter_color_size - 1, 0));
        }
        structure->_map.emplace(node, assign);
    }

    void convertHashPrimitive(const IR::MethodCallStatement *node, cstring hashName,
                              int hashWidth) {
        auto mce = node->methodCall->to<IR::MethodCallExpression>();
        ERROR_CHECK(mce->arguments->size() > 4, "insufficient arguments to hash() function");
        auto pDest = mce->arguments->at(0)->expression;
        auto pBase = mce->arguments->at(2)->expression;
        auto pMax = mce->arguments->at(4)->expression;

        auto dstWidth = pDest->type->width_bits();
        // Add data unconditionally.
        auto args = new IR::Vector<IR::Argument>({mce->arguments->at(3)});
        auto typeArgs = new IR::Vector<IR::Type>({mce->arguments->at(3)->expression->type});
        if (pMax->to<IR::Constant>() == nullptr || pBase->to<IR::Constant>() == nullptr)
            BUG("Only compile-time constants are supported for hash base offset and max value");

        if (pBase->to<IR::Constant>()->asInt() != 0)
            error("The initial offset for a hash calculation function has to be zero");

        int bit_size = 0;
        if (auto *constant = pMax->to<IR::Constant>()) {
            auto value = constant->asUint64();
            if (value != 0) {
                bit_size = bitcount(value - 1);
                if ((1ULL << bit_size) != value)
                    error(
                        "%1%: The hash offset must be a power of 2 in a hash calculation "
                        "instead of %2%",
                        node, value);
            }
        }
        auto member = new IR::Member(new IR::PathExpression(hashName), "get");
        IR::Expression *methodCall =
            new IR::MethodCallExpression(node->srcInfo, member, typeArgs, args);
        // only introduce slice if
        // hash.get() returns value that is wider than the specified max width.
        if (bit_size != 0 && hashWidth > bit_size)
            methodCall =
                new IR::Slice(methodCall, new IR::Constant(bit_size - 1), new IR::Constant(0));

        LOG3("dst width " << dstWidth << " bit size " << bit_size);

        if (dstWidth > bit_size)
            methodCall = new IR::Cast(IR::Type::Bits::get(dstWidth), methodCall);
        else if (dstWidth < bit_size)
            methodCall =
                new IR::Slice(methodCall, new IR::Constant(dstWidth - 1), new IR::Constant(0));

        auto result = new IR::AssignmentStatement(pDest, methodCall);
        structure->_map.emplace(node, result);
    }

    /// hash function is converted to an instance of hash extern in the enclosed control block
    void cvtHashFunction(const IR::MethodCallStatement *node) {
        auto control = findContext<IR::P4Control>();
        ERROR_CHECK(control != nullptr, "hash() must be used in a control block");
        const bool isIngress =
            (control->name == structure->getBlockName(ProgramStructure::INGRESS));

        auto mce = node->methodCall->to<IR::MethodCallExpression>();
        BUG_CHECK(mce != nullptr, "Malformed IR: method call expression cannot be nullptr");

        ERROR_CHECK(mce->arguments->size() > 3, "hash extern must have at least 4 arguments");
        auto typeArgs = new IR::Vector<IR::Type>({mce->typeArguments->at(0)});
        int hashWidth = mce->typeArguments->at(0)->width_bits();

        auto block = findContext<IR::BlockStatement>();
        auto annotations = new IR::Annotations();
        if (block) {
            for (auto annot : block->annotations->annotations) annotations->add(annot);
        }

        auto hashType = new IR::Type_Specialized(new IR::Type_Name("Hash"), typeArgs);
        auto hashName = cstring::make_unique(structure->unique_names, "hash"_cs, '_');
        structure->unique_names.insert(hashName);
        convertHashPrimitive(node, hashName, hashWidth);

        // create hash instance
        auto algorithm = mce->arguments->at(1)->clone();
        if (auto typeName = algorithm->to<IR::Member>()) {
            structure->typeNamesToDo.emplace(typeName, typeName);
            LOG3("add " << typeName << " to translation map");
        }
        auto hashArgs = new IR::Vector<IR::Argument>({algorithm});
        auto hashInst = new IR::Declaration_Instance(hashName, annotations, hashType, hashArgs);

        if (isIngress) {
            structure->ingressDeclarations.push_back(hashInst->to<IR::Declaration>());
        } else {
            structure->egressDeclarations.push_back(hashInst->to<IR::Declaration>());
        }
    }

    /// resubmit function is converted to an assignment on resubmit_type
    void cvtResubmitFunction(const IR::MethodCallStatement *node) {
        /*
         * translate resubmit() function in ingress control block to
         *
         * ig_intr_md_for_dprsr.resubmit_type = n;
         *
         */
        auto mce = node->methodCall->to<IR::MethodCallExpression>();
        BUG_CHECK(mce != nullptr, "malformed IR in resubmit() function");
        auto control = findContext<IR::P4Control>();
        ERROR_CHECK(control != nullptr, "resubmit() must be used in a control block");
        ERROR_CHECK(control->name == structure->getBlockName(ProgramStructure::INGRESS),
                    "resubmit() can only be used in ingress control");
        IR::PathExpression *path = new IR::PathExpression("ig_intr_md_for_dprsr");
        auto mem = new IR::Member(path, "resubmit_type");
        unsigned resubmitId = getIndex(node, resubmitIndexHashes);
        if (resubmitId > Device::maxResubmitId()) {
            error("Too many resubmit() calls in %1%", control->name);
            return;
        }
        unsigned bits = static_cast<unsigned>(std::ceil(std::log2(Device::maxResubmitId())));
        auto idx = new IR::Constant(IR::Type::Bits::get(bits), resubmitId);
        auto stmt = new IR::AssignmentStatement(mem, idx);
        structure->_map.emplace(node, stmt);

        if (resubmitIndexSet.count(resubmitId)) {
            LOG3("Already generated resubmit.emit for id " << resubmitId);
            return;
        } else {
            resubmitIndexSet.insert(resubmitId);
        }
        /*
         * Add header definition to top level;
         * header $resubmit_header_t {
         *   bit<8> select;
         *   bit<n> field_0; @flexible;
         *   ...
         * }
         * In the ingress deparser, add the following code
         * Resubmit() resubmit;
         * if (ig_intr_md_for_dprsr.resubmit_type == n) {
         *    resubmit.emit<T>({ ... });
         * }
         */

        // Only instantiate the extern for the first instance of resubmit()
        auto findDecl = [](std::vector<const IR::Declaration *> decls,
                           cstring name) -> const IR::Declaration * {
            for (auto d : decls)
                if (d->getName() == name) return d;
            return nullptr;
        };
        const IR::Declaration *resubmitDeclared =
            findDecl(structure->ingressDeparserDeclarations, "resubmit"_cs);

        if (resubmitDeclared == nullptr) {
            auto declArgs = new IR::Vector<IR::Argument>({});
            auto declType = new IR::Type_Name("Resubmit");
            auto decl = new IR::Declaration_Instance("resubmit"_cs, declType, declArgs);
            structure->ingressDeparserDeclarations.push_back(decl);
        }

        cstring generatedResubmitHeaderTypeName = genUniqueName("__resubmit"_cs);

        auto digestFieldsGeneratedByCompiler = new std::vector<DigestFieldInfo>();
        /**
         * We used bit<8> for resubmit select field to avoid padding alignment
         * issue in the backend. This might be sub-optimal as the padding bit
         * could be overlayed with other fields.
         */
        auto info = std::make_tuple("__field_0"_cs, IR::Type::Bits::get(8),
                                    new IR::Cast(IR::Type::Bits::get(8), mem));
        digestFieldsGeneratedByCompiler->push_back(info);

        auto digestFieldsFromSource = new std::vector<DigestFieldInfo>();
        // resubmit (which is deprecated)
        if (mce->typeArguments->size() > 0 && mce->typeArguments->at(0)->is<IR::Type_BaseList>()) {
            if (auto arg = mce->arguments->at(0)->to<IR::Argument>()) {
                auto fl = arg->expression;  // resubmit field list
                if (auto list = fl->to<IR::ListExpression>()) {
                    if (auto tpl = list->type->to<IR::Type_BaseList>()) {
                        auto components = new IR::Vector<IR::Type>();
                        components->push_back(IR::Type::Bits::get(8));
                        components->append(tpl->components);
                        auto header_type = convertTupleTypeToHeaderType(
                            generatedResubmitHeaderTypeName, components, true);
                        structure->type_declarations.emplace(header_type->name, header_type);
                        LOG3("create header " << header_type->name.name << " " << header_type);
                        // generate a struct initializer for the header type
                        int index = 1;  // first element was used for mirror_source
                        for (auto elem : list->components) {
                            cstring fname = "__field_"_cs + std::to_string(index);
                            LOG3("name " << fname << " type " << elem->type << " expr " << elem);
                            DigestFieldInfo info = std::make_tuple(fname, elem->type, elem);
                            digestFieldsFromSource->push_back(info);
                            index++;
                        }
                    }
                }
            }
            // resubmit_preserving_field_list
        } else if (mce->arguments->size() > 0 &&
                   (mce->arguments->at(0)->expression->is<IR::Cast>() ||
                    mce->arguments->at(0)->expression->is<IR::Constant>())) {
            // Look into the first argument (field list index)
            auto arg = mce->arguments->at(0)->to<IR::Argument>();
            BUG_CHECK(arg != nullptr, "invalid resubmit_preserving_field_list method argument");
            convertPreservingFieldList(arg, control, "resubmit"_cs, generatedResubmitHeaderTypeName,
                                       nullptr, digestFieldsFromSource);
        } else {
            // has no data
            cstring hdName = generatedResubmitHeaderTypeName + "_header_t"_cs;
            auto fields = new IR::IndexedVector<IR::StructField>();
            fields->push_back(new IR::StructField(IR::ID("__field_0"), IR::Type::Bits::get(8)));
            auto type = new IR::Type_Header(hdName, *fields);
            structure->type_declarations.emplace(hdName, type);
        }

        auto fieldList = convertFieldList(generatedResubmitHeaderTypeName, digestFieldsFromSource,
                                          digestFieldsGeneratedByCompiler);
        /// compiler inserts resubmit_type as the format id to
        /// identify the resubmit group, it is 3 bits in size,
        /// and padded to byte boundary
        auto args = new IR::Vector<IR::Argument>(new IR::Argument(fieldList));
        auto expr = new IR::PathExpression(new IR::Path("resubmit"));
        auto member = new IR::Member(expr, "emit");
        auto typeArgs = new IR::Vector<IR::Type>(
            {new IR::Type_Name(IR::ID(generatedResubmitHeaderTypeName + "_header_t"))});
        auto mcs =
            new IR::MethodCallStatement(new IR::MethodCallExpression(member, typeArgs, args));

        auto condExprPath = new IR::Member(
            new IR::PathExpression(new IR::Path("ig_intr_md_for_dprsr")), "resubmit_type");
        auto condExpr = new IR::Equ(condExprPath, idx);
        auto cond = new IR::IfStatement(condExpr, mcs, nullptr);
        structure->ingressDeparserStatements.push_back(cond);
    }

    void convertRandomPrimitive(const IR::MethodCallStatement *node, cstring randName) {
        auto mce = node->methodCall->to<IR::MethodCallExpression>();

        auto dest = mce->arguments->at(0)->expression;
        // check hi bound must be 2**W-1
        auto max_value = mce->arguments->at(2)->expression->to<IR::Constant>()->value;
        if (max_value < LONG_MIN || max_value > LONG_MAX)
            error("%s: The random declaration %s max size %d is too large to be supported",
                  node->srcInfo, randName, max_value);
        bool isPowerOfTwoMinusOne = false;
        isPowerOfTwoMinusOne = ((max_value & (max_value + 1)) == 0);
        if (!isPowerOfTwoMinusOne)
            error("%s: The random declaration %s max size must be a power of two minus one",
                  node->srcInfo, randName);
        // check lo bound must be zero
        auto min_value = mce->arguments->at(1)->expression->to<IR::Constant>()->value;
        if (min_value != 0)
            error("%s: The random declaration %s min size must be zero", node->srcInfo, randName);
        auto args = new IR::Vector<IR::Argument>();
        LOG3("convert " << node->methodCall << " to " << randName << "(" << min_value << ", "
                        << max_value << ")");

        auto method = new IR::PathExpression(randName);
        auto member = new IR::Member(method, "get");
        auto call = new IR::MethodCallExpression(node->srcInfo, member, args);
        structure->_map.emplace(node, new IR::AssignmentStatement(dest, call));
    }

    void cvtRandomFunction(const IR::MethodCallStatement *node) {
        auto control = findContext<IR::P4Control>();
        ERROR_CHECK(control != nullptr, "random() must be used in a control block");
        const bool isIngress =
            (control->name == structure->getBlockName(ProgramStructure::INGRESS));
        auto mce = node->methodCall->to<IR::MethodCallExpression>();
        BUG_CHECK(mce != nullptr, "Malformed IR: method call expression cannot be nullptr");

        auto baseType = mce->arguments->at(0)->expression;
        auto typeArgs = new IR::Vector<IR::Type>({baseType->type});
        auto type = new IR::Type_Specialized(new IR::Type_Name("Random"), typeArgs);
        auto param = new IR::Vector<IR::Argument>();
        auto randName = cstring::make_unique(structure->unique_names, "random"_cs, '_');
        structure->unique_names.insert(randName);
        convertRandomPrimitive(node, randName);

        auto randInst = new IR::Declaration_Instance(randName, type, param);

        if (isIngress) {
            structure->ingressDeclarations.push_back(randInst->to<IR::Declaration>());
        } else {
            structure->egressDeclarations.push_back(randInst->to<IR::Declaration>());
        }
    }

    void cvtRecirculateRawFunction(const IR::MethodCallStatement *node) {
        auto control = findContext<IR::P4Control>();
        ERROR_CHECK(control != nullptr, "recirculate() must be used in a control block");
        ERROR_CHECK(control->name == structure->getBlockName(ProgramStructure::INGRESS),
                    "Recirculate must be called in Ingress");
        auto mce = node->methodCall->to<IR::MethodCallExpression>();
        BUG_CHECK(mce != nullptr, "Malformed IR: method call expression cannot be nullptr");
        IR::PathExpression *tm_meta = new IR::PathExpression("ig_intr_md_for_tm");
        auto egress_spec = new IR::Member(tm_meta, "ucast_egress_port");
        IR::PathExpression *intr_meta = new IR::PathExpression("ig_intr_md");
        auto ingress_port = new IR::Member(intr_meta, "ingress_port");
        int portWidth = Device::portBitWidth();

        auto stmts = new IR::IndexedVector<IR::StatOrDecl>();
        stmts->push_back(
            new IR::AssignmentStatement(new IR::Slice(egress_spec, 6, 0),
                                        new IR::Slice(mce->arguments->at(0)->expression, 6, 0)));
        stmts->push_back(
            new IR::AssignmentStatement(new IR::Slice(egress_spec, portWidth - 1, 7),
                                        new IR::Slice(ingress_port, portWidth - 1, 7)));
        structure->_map.emplace(node, stmts);
    }

    void implementUpdateChecksum(const IR::MethodCallStatement *uc, cstring which,
                                 std::vector<gress_t> deparserUpdateLocations, bool zerosAsOnes) {
        auto mc = uc->methodCall->to<IR::MethodCallExpression>();

        auto condition = mc->arguments->at(0)->expression;
        auto fieldlist = mc->arguments->at(1)->expression;
        auto destfield = mc->arguments->at(2)->expression;

        for (auto location : deparserUpdateLocations) {
            IR::Declaration_Instance *decl = createChecksumDeclaration(structure, uc);

            if (location == INGRESS)
                structure->ingressDeparserDeclarations.push_back(decl);
            else if (location == EGRESS)
                structure->egressDeparserDeclarations.push_back(decl);

            IR::Vector<IR::Expression> exprs;

            if (fieldlist->is<IR::ListExpression>())
                for (auto f : fieldlist->to<IR::ListExpression>()->components) exprs.push_back(f);
            else if (fieldlist->is<IR::StructExpression>())
                for (auto f : fieldlist->to<IR::StructExpression>()->components)
                    exprs.push_back(f->expression);

            if (which == "update_checksum_with_payload") {
                if (parserChecksums->residualChecksums.count(destfield)) {
                    auto pr = parserChecksums->residualChecksums.at(destfield);
                    exprs.push_back(pr);
                    if (parserChecksums->residualChecksumPayloadFields.count(destfield)) {
                        for (auto f :
                             parserChecksums->residualChecksumPayloadFields.at(destfield)) {
                            exprs.push_back(f);
                            LOG4("add payload field " << f << " to " << destfield);
                        }
                    }
                }
            }

            IR::ListExpression *list = new IR::ListExpression(exprs);
            auto arguments = new IR::Vector<IR::Argument>;
            arguments->push_back(new IR::Argument(list));
            if (zerosAsOnes) {
                arguments->push_back(new IR::Argument(new IR::BoolLiteral(1)));
            }
            auto updateCall = new IR::MethodCallExpression(
                mc->srcInfo, new IR::Member(new IR::PathExpression(decl->name), "update"),
                new IR::Vector<IR::Type>({list->type}), arguments);
            IR::Statement *stmt = new IR::AssignmentStatement(destfield, updateCall);

            if (auto boolLiteral = condition->to<IR::BoolLiteral>()) {
                if (!boolLiteral->value) {
                    // Do not add the if-statement if the condition is always true.
                    stmt = nullptr;
                }
            } else {
                stmt = new IR::IfStatement(condition, stmt, nullptr);
            }

            if (stmt) {
                if (location == INGRESS)
                    structure->ingressDeparserStatements.push_back(stmt);
                else if (location == EGRESS)
                    structure->egressDeparserStatements.push_back(stmt);
            }
        }
    }

    bool getZerosAsOnes(const IR::BlockStatement *block, const IR::MethodCallStatement *call) {
        for (auto annot : block->annotations->annotations) {
            if (annot->name.name == "zeros_as_ones") {
                auto mc = call->methodCall->to<IR::MethodCallExpression>();
                if (annot->expr[0]->equiv(*mc)) return true;
            }
        }
        return false;
    }

    void cvtUpdateChecksum(const IR::MethodCallStatement *call, cstring which) {
        auto block = findContext<IR::BlockStatement>();
        std::vector<gress_t> deparserUpdateLocations =
            getChecksumUpdateLocations(call->methodCall->to<IR::MethodCallExpression>(), block,
                                       "calculated_field_update_location"_cs);
        bool zerosAsOnes = getZerosAsOnes(block, call);
        if (analyzeChecksumCall(call, which))
            implementUpdateChecksum(call, which, deparserUpdateLocations, zerosAsOnes);
    }

    /// build up a table for all metadata member that need to be translated.
    void postorder(const IR::Member *node) override {
        /// header/struct names appeared in p4_14_include/tofino/intrinsic_metadata.p4
        ordered_set<cstring> toTranslateInControl = {"standard_metadata"_cs,
                                                     "ig_intr_md_for_tm"_cs,
                                                     "ig_intr_md"_cs,
                                                     "ig_pg_md"_cs,
                                                     "ig_intr_md_for_mb"_cs,
                                                     "ig_intr_md_from_parser_aux"_cs,
                                                     "eg_intr_md"_cs,
                                                     "eg_intr_md_for_mb"_cs,
                                                     "eg_intr_md_from_parser_aux"_cs,
                                                     "eg_intr_md_for_oport"_cs};
        ordered_set<cstring> toTranslateInParser = {"standard_metadata"_cs, "ig_prsr_ctrl"_cs};
        auto gress = findOrigCtxt<IR::P4Control>();
        if (gress) {
            if (auto member = node->expr->to<IR::Member>()) {
                auto it = toTranslateInControl.find(member->member.name);
                if (it != toTranslateInControl.end()) {
                    structure->membersToDo.emplace(node, node);
                }
            } else if (auto expr = node->expr->to<IR::PathExpression>()) {
                auto path = expr->path->to<IR::Path>();
                CHECK_NULL(path);
                auto it = toTranslateInControl.find(path->name);
                if (it != toTranslateInControl.end()) {
                    if (gress->name == structure->getBlockName(ProgramStructure::INGRESS)) {
                        structure->pathsThread.emplace(node, INGRESS);
                        structure->pathsToDo.emplace(node, node);
                    } else if (gress->name == structure->getBlockName(ProgramStructure::EGRESS)) {
                        structure->pathsThread.emplace(node, EGRESS);
                        structure->pathsToDo.emplace(node, node);
                    } else {
                        LOG1("WARNING: path " << node << " in " << gress->name.name
                                              << " is not translated");
                    }
                }
            } else if (auto expr = node->expr->to<IR::TypeNameExpression>()) {
                auto tn = expr->typeName->to<IR::Type_Name>();
                CHECK_NULL(tn);
                structure->typeNamesToDo.emplace(node, node);
            } else {
                LOG1("WARNING: Unable to translate path " << node);
            }
        }
        auto parser = findOrigCtxt<IR::P4Parser>();
        if (parser) {
            if (auto expr = node->expr->to<IR::PathExpression>()) {
                auto path = expr->path->to<IR::Path>();
                CHECK_NULL(path);
                auto it = toTranslateInParser.find(path->name);
                if (it == toTranslateInParser.end()) return;
                if (auto type = expr->type->to<IR::Type_Name>()) {
                    if (type->path->name == "ingress_parser_control_signals") {
                        if (auto stmt = findContext<IR::AssignmentStatement>()) {
                            if (node->member == "parser_counter") {
                                ParserCounterConverter cvt(structure);
                                structure->_map.emplace(stmt, stmt->apply(cvt));
                            } else if (node->member == "priority") {
                                ParserPriorityConverter cvt(structure);
                                structure->_map.emplace(stmt, stmt->apply(cvt));
                            }
                            // handle parser counter in select() expression
                        } else if (auto select = findOrigCtxt<IR::SelectExpression>()) {
                            ParserCounterSelectionConverter cvt;
                            structure->_map.emplace(select, select->apply(cvt));
                        } else {
                            BUG("Invalid expression for parser counter");
                        }
                    }
                } else if (expr->type->is<IR::Type_Struct>()) {
                    structure->pathsToDo.emplace(node, node);
                } else {
                    LOG1("WARNING: metadata " << node << " is not converted");
                }
            } else if (auto expr = node->expr->to<IR::TypeNameExpression>()) {
                auto tn = expr->typeName->to<IR::Type_Name>();
                CHECK_NULL(tn);
                structure->typeNamesToDo.emplace(node, node);
            } else {
                LOG1("WARNING: Expression " << node << " is not converted");
            }
        }
    }

    void cvtActionProfile(const IR::Declaration_Instance *node) {
        auto type = node->type->to<IR::Type_Name>();
        ERROR_CHECK(type->path->name == "action_profile",
                    "action_profile converter cannot be applied to %1%", type->path->name);

        type = new IR::Type_Name("ActionProfile");
        ERROR_CHECK(node->arguments->size() == 1, "action_profile must have a size argument");
        auto size = node->arguments->at(0)->expression->to<IR::Constant>()->asInt();
        auto *args = new IR::Vector<IR::Argument>();
        if (size == 0) {
            LOG1("WARNING: action_profile " << node->name.name
                                            << "is specified with size 0, "
                                               "default to 1024.");
            size = 1024;  // default size is set to 1024, to be consistent with glass
            args->push_back(new IR::Argument(new IR::Constant(IR::Type_Bits::get(32), size)));
        } else {
            args->push_back(node->arguments->at(0));
        }
        auto result =
            new IR::Declaration_Instance(node->srcInfo, node->name, node->annotations, type, args);
        structure->_map.emplace(node, result);
    }

    void cvtRegister(const IR::Declaration_Instance *node) {
        auto type = node->type->to<IR::Type_Specialized>();
        ERROR_CHECK(type != nullptr, "Invalid type for register converter");
        ERROR_CHECK(type->baseType->path->name == "register",
                    "register converter cannot be applied to %1%", type->baseType->path->name);
        BUG_CHECK(node->arguments->size() == 1, "register must have exactly one argument");
        BUG_CHECK(type->arguments->size() == 1 || type->arguments->size() == 2,
                  "register must have one or two type arguments");

        auto typeArgs = new IR::Vector<IR::Type>();
        auto eltype = type->arguments->at(0);
        if (auto bits = eltype->to<IR::Type::Bits>()) {
            auto width = 1 << ceil_log2(bits->size);  // round up to a power of 2
            if (width > 1 && width < 8) width = 8;    // 2/4 are not usable on tofino
            if (width != bits->size) {
                // Not a valid tofino register width -- round up to the next valid width?
                if (structure->backward_compatible && width <= 64) {
                    eltype = IR::Type::Bits::get(width, bits->isSigned);
                    warning("register %s width %d not supported, rounding up to %d", node,
                            bits->size, width);
                } else {
                    error("register %s width %d not supported", node, bits->size);
                }
            }
        }
        typeArgs->push_back(eltype);

        auto args = new IR::Vector<IR::Argument>();

        if (node->arguments->at(0)->expression->to<IR::Constant>()->asInt()) {
            args->push_back(node->arguments->at(0));
            if (type->arguments->size() == 2)
                typeArgs->push_back(type->arguments->at(1));
            else
                typeArgs->push_back(IR::Type::Bits::get(32));
            type = new IR::Type_Specialized(new IR::Type_Name("Register"), typeArgs);
        } else {
            type = new IR::Type_Specialized(new IR::Type_Name("DirectRegister"), typeArgs);
        }

        auto decl =
            new IR::Declaration_Instance(node->srcInfo, node->name, node->annotations, type, args);
        structure->_map.emplace(node, decl);
    }

    void cvtActionSelector(const IR::Declaration_Instance *node) {
        auto type = node->type->to<IR::Type_Name>();
        ERROR_CHECK(type->path->name == "action_selector",
                    "action_selector converter cannot be applied to %1%", type->path->name);
        auto declarations = new IR::IndexedVector<IR::Declaration>();

        // Create a new instance of Hash<W>
        auto typeArgs = new IR::Vector<IR::Type>();
        auto w = node->arguments->at(2)->expression->to<IR::Constant>()->asInt();
        typeArgs->push_back(IR::Type::Bits::get(w));

        auto args = new IR::Vector<IR::Argument>();
        args->push_back(node->arguments->at(0));

        auto specializedType = new IR::Type_Specialized(new IR::Type_Name("Hash"), typeArgs);
        auto hashName = cstring::make_unique(structure->unique_names, node->name + "__hash_", '_');
        structure->unique_names.insert(hashName);
        declarations->push_back(new IR::Declaration_Instance(hashName, specializedType, args));

        args = new IR::Vector<IR::Argument>();
        auto size = node->arguments->at(1)->expression->to<IR::Constant>()->asInt();
        if (size == 0) {
            LOG1("WARNING: action_profile " << node->name.name
                                            << "is specified with size 0, "
                                               "default to 1024.");
            size = 1024;  // default size is set to 1024, to be consistent with glass
            args->push_back(new IR::Argument(new IR::Constant(IR::Type_Bits::get(32), size)));
        } else {
            args->push_back(node->arguments->at(1));
        }
        // hash
        args->push_back(new IR::Argument(new IR::PathExpression(hashName)));
        // selector_mode
        auto sel_mode = new IR::Member(new IR::TypeNameExpression("SelectorMode_t"), "FAIR");
        if (auto anno = node->annotations->getSingle("mode"_cs)) {
            auto mode = anno->expr.at(0)->to<IR::StringLiteral>();
            if (mode->value == "resilient")
                sel_mode->member = IR::ID("RESILIENT");
            else if (mode->value != "fair" && mode->value != "non_resilient")
                BUG("Selector mode provided for the selector is not supported", node);
        } else {
            LOG1("WARNING: No mode specified for the selector %s. Assuming fair" << node);
        }
        args->push_back(new IR::Argument(sel_mode));

        type = new IR::Type_Name("ActionSelector");
        declarations->push_back(
            new IR::Declaration_Instance(node->srcInfo, node->name, node->annotations, type, args));
        structure->_map.emplace(node, declarations);
    }

    void cvtCounterDecl(const IR::Declaration_Instance *node) {
        auto typeArgs = new IR::Vector<IR::Type>();
        // type<W>
        if (auto anno = node->annotations->getSingle("min_width"_cs)) {
            auto min_width = anno->expr.at(0)->as<IR::Constant>().asInt();
            typeArgs->push_back(IR::Type::Bits::get(min_width));
        } else {
            auto min_width = IR::Type::Bits::get(64);  // Do not impose LRT
            typeArgs->push_back(min_width);
            LOG1("WARNING: Could not infer min_width for counter %s, using bit<64>" << node);
        }
        // type<S>
        if (auto type = node->type->to<IR::Type_Specialized>()) {
            typeArgs->push_back(type->arguments->at(0));
        } else if (auto s = node->arguments->at(0)->expression->to<IR::Constant>()) {
            typeArgs->push_back(s->type->to<IR::Type_Bits>());
        }

        auto specializedType = new IR::Type_Specialized(new IR::Type_Name("Counter"), typeArgs);
        auto decl = new IR::Declaration_Instance(node->srcInfo, node->name, node->annotations,
                                                 specializedType, node->arguments);
        structure->_map.emplace(node, decl);
    }

    void cvtMeterDecl(const IR::Declaration_Instance *node) {
        auto typeArgs = new IR::Vector<IR::Type>();
        if (auto type = node->type->to<IR::Type_Specialized>()) {
            typeArgs->push_back(type->arguments->at(0));
        } else if (auto s = node->arguments->at(0)->expression->to<IR::Constant>()) {
            typeArgs->push_back(s->type->to<IR::Type_Bits>());
        }
        auto specializedType = new IR::Type_Specialized(new IR::Type_Name("Meter"), typeArgs);
        auto decl = new IR::Declaration_Instance(node->srcInfo, node->name, node->annotations,
                                                 specializedType, node->arguments);
        structure->_map.emplace(node, decl);
    }

    void cvtDirectMeterDecl(const IR::Declaration_Instance *node) {
        auto decl = new IR::Declaration_Instance(node->srcInfo, node->name, node->annotations,
                                                 new IR::Type_Name("DirectMeter"), node->arguments);
        structure->_map.emplace(node, decl);
    }

    void cvtDirectCounterDecl(const IR::Declaration_Instance *node) {
        auto typeArgs = new IR::Vector<IR::Type>();
        if (auto anno = node->annotations->getSingle("min_width"_cs)) {
            auto min_width = anno->expr.at(0)->as<IR::Constant>().asInt();
            typeArgs->push_back(IR::Type::Bits::get(min_width));
        } else {
            // Do not impose LRT
            auto min_width = IR::Type::Bits::get(64);
            typeArgs->push_back(min_width);
            LOG1("WARNING: Could not infer min_width for counter %s, using bit<64>" << node);
        }
        auto specializedType =
            new IR::Type_Specialized(new IR::Type_Name("DirectCounter"), typeArgs);
        auto decl = new IR::Declaration_Instance(node->srcInfo, node->name, node->annotations,
                                                 specializedType, node->arguments);
        structure->_map.emplace(node, decl);
    }

    void cvtExternDeclaration(const IR::Declaration_Instance *node, cstring name) {
        if (name == "counter") {
            cvtCounterDecl(node);
        } else if (name == "direct_counter") {
            cvtDirectCounterDecl(node);
        } else if (name == "meter") {
            cvtMeterDecl(node);
        } else if (name == "direct_meter") {
            cvtDirectMeterDecl(node);
        } else if (name == "action_profile") {
            cvtActionProfile(node);
        } else if (name == "action_selector") {
            cvtActionSelector(node);
        } else if (name == "register") {
            cvtRegister(node);
        }
    }

    void postorder(const IR::Declaration_Instance *node) override {
        if (node->type->is<IR::Type_Specialized>()) {
            auto type = node->type->to<IR::Type_Specialized>()->baseType;
            if (type->path->name == "V1Switch") {
                auto type_h = node->type->to<IR::Type_Specialized>()->arguments->at(0);
                auto type_m = node->type->to<IR::Type_Specialized>()->arguments->at(1);
                structure->type_h = type_h->to<IR::Type_Name>()->path->name;
                structure->type_m = type_m->to<IR::Type_Name>()->path->name;
            }
            cvtExternDeclaration(node, type->path->name);
        } else if (auto type = node->type->to<IR::Type_Name>()) {
            cvtExternDeclaration(node, type->path->name);
        }
    }

    void postorder(const IR::MethodCallStatement *node) override {
        auto mce = node->methodCall->to<IR::MethodCallExpression>();
        CHECK_NULL(mce);
        auto mi = P4::MethodInstance::resolve(node, refMap, typeMap);
        if (auto em = mi->to<P4::ExternMethod>()) {
            cstring name = em->actualExternType->name;
            if (name == "direct_meter") {
                MeterConverter cvt(structure, refMap, true);
                structure->_map.emplace(node, node->apply(cvt));
            } else if (name == "meter") {
                MeterConverter cvt(structure, refMap, false);
                structure->_map.emplace(node, node->apply(cvt));
            } else if (name == "register") {
                RegisterConverter cvt(structure);
                structure->_map.emplace(node, node->apply(cvt));
            }
            // Counter method needs no translation
        } else if (auto ef = mi->to<P4::ExternFunction>()) {
            cstring name = ef->method->name;
            if (name == "hash") {
                cvtHashFunction(node);
            } else if (name == "resubmit" || name == "resubmit_preserving_field_list") {
                cvtResubmitFunction(node);
            } else if (name == "mark_to_drop" || name == "drop") {
                cvtDropFunction(node);
            } else if (name == "random") {
                cvtRandomFunction(node);
            } else if (name == "digest") {
                cvtDigestFunction(node);
            } else if (name == "clone") {
                cvtCloneFunction(node, /* cloneFuncType = */ CLONE);
            } else if (name == "clone3") {
                cvtCloneFunction(node, /* cloneFuncType = */ CLONE3);
            } else if (name == "clone_preserving_field_list") {
                cvtCloneFunction(node, /* cloneFuncType = */ CLONE_PRESERVING_FIELD_LIST);
            } else if (name == "invalidate_digest") {
                cvtInvalidateDigestFunction(node);
            } else if (name == "update_checksum" || name == "update_checksum_with_payload") {
                cvtUpdateChecksum(node, name);
            } else if (name == "execute_meter_with_color") {
                cvtExecuteMeterFunction(node);
            } else if (name == "recirculate_raw") {
                cvtRecirculateRawFunction(node);
            }
        }
    }

    // if a path refers to a global declaration, move
    // the global declaration to local control;
    void postorder(const IR::PathExpression *node) override {
        auto path = node->path;
        auto it = structure->global_instances.find(path->name);
        if (it != structure->global_instances.end()) {
            if (globals.find(path->name) != globals.end()) return;
            auto control = findContext<IR::P4Control>();
            if (control == nullptr) {
                error(
                    "Unable to reference global instance '%1%' from non-control block. "
                    "Please check that '%1%' is not a reserved keyword (including in P4-16).",
                    path->name);
                return;
            }
            if (control->name == structure->getBlockName(ProgramStructure::INGRESS)) {
                structure->ingressDeclarations.push_back(it->second);
                globals.insert(path->name);
            } else if (control->name == structure->getBlockName(ProgramStructure::EGRESS)) {
                structure->egressDeclarations.push_back(it->second);
                globals.insert(path->name);
            } else {
                BUG("unexpected reference to global instance from %1%", control->name);
            }
        }
    }

    void postorder(const IR::Property *node) override {
        if (node->name == "support_timeout") {
            auto idle_timeout = new IR::Property("idle_timeout"_cs, node->annotations, node->value,
                                                 node->isConstant);
            structure->_map.emplace(node, idle_timeout);
        }
    }

 private:
    /**
     * Check if the argument is constant or cast and extract the
     * constant out of it
     */
    const IR::Constant *getConstantFromArg(const IR::Argument *argument) {
        auto cast = argument->expression->to<IR::Cast>();
        const IR::Constant *cst = nullptr;
        if (cast) {
            cst = cast->expr->to<IR::Constant>();
        } else {
            cst = argument->expression->to<IR::Constant>();
        }
        return cst;
    }

    /**
     * Extract a field list based on the index. This is needed for
     * *_preserving_field_list P4 primitives, where the list is not
     * entirely in the primitive arguments but it is pointed to by an index.
     * Fields that should be used are annotated with this index.
     */
    void findFieldList(const IR::Type_StructLike *st, unsigned index,
                       std::vector<IR::Expression *> *fl, IR::Expression *exp) {
        // Go through the fields
        for (auto f : st->fields) {
            // Recursively check the fields themselves
            if (auto nestedSt = f->type->to<IR::Type_StructLike>()) {
                auto nexp = new IR::Member(f->type, exp, IR::ID(f->name));
                findFieldList(nestedSt, index, fl, nexp);
            } else {
                // Check the field_list annotation
                auto anno = f->getAnnotations()->getSingle("field_list"_cs);
                if (anno == nullptr) continue;
                for (auto e : anno->expr) {
                    auto cst = e->to<IR::Constant>();
                    if (cst == nullptr) {
                        error("%1%: Annotation must be a constant integer", e);
                        continue;
                    }
                    unsigned id = cst->asUnsigned();
                    // If this is the field_list we are looking for, add the field
                    if (id == index) {
                        LOG4("found field " << f << " belonging to a field list " << index);
                        auto nexp = new IR::Member(f->type, exp, IR::ID(f->name));
                        fl->push_back(nexp);
                    }
                }
            }
        }
    }

    /**
     * This function encapsulates steps that are the same for
     * converting *_preserving_field_list primitives.
     * @arg indexArg IR argument that should hold the field list index value.
     * @arg control IR control block that invokes the primitive.
     * @arg primitive Name of the primitive that is converted (for logging purposes).
     * @arg generatedName Name of the header that should be generated.
     * @arg headerName Used to return name of the header type created (if not nullptr).
     * @arg digestFields Adds the fields that should be preserved into this list.
     */
    void convertPreservingFieldList(const IR::Argument *indexArg, const IR::P4Control *control,
                                    cstring primitive, cstring generatedName, cstring *headerName,
                                    std::vector<DigestFieldInfo> *digestFields) {
        const IR::Constant *cst = getConstantFromArg(indexArg);
        BUG_CHECK(cst != nullptr, "invalid %1%_preserving_field_list method argument", primitive);
        unsigned index = cst->asUnsigned();  // This is the field list index
        // Go through the metadata of the control block
        auto params = control->type->getApplyParameters();
        BUG_CHECK(params->size() == 3, "%1%: expected 3 parameters", control);
        auto metaParam = params->parameters.at(1);
        auto metaParamType = typeMap->getType(metaParam, true);
        BUG_CHECK(metaParamType != nullptr, "could not find type for %1%", metaParamType);
        auto metaParamSt = metaParamType->to<IR::Type_Struct>();
        BUG_CHECK(metaParamSt != nullptr, "expected Type_Struct %1%", metaParamSt);
        // Get all the fields that are annotated with the field list of a given index
        auto fl = new std::vector<IR::Expression *>();
        auto pathExp = new IR::PathExpression(metaParamType, new IR::Path(metaParam->name));
        findFieldList(metaParamSt, index, fl, pathExp);
        // Generate a header type, used by repacking algorithm
        auto components = new IR::Vector<IR::Type>();
        components->push_back(IR::Type::Bits::get(8));  // mirror_source
        for (auto f : *fl) {
            components->push_back(f->type);
        }
        auto headerType = convertTupleTypeToHeaderType(generatedName, components, true);
        structure->type_declarations.emplace(headerType->name, headerType);
        LOG3("create header " << headerType->name.name);
        if (headerName != nullptr) *headerName = headerType->name;
        // generate a struct initializer for the header type
        int i = 1;  // first element was used for mirror_source
        for (auto f : *fl) {
            cstring fname = "__field_"_cs + std::to_string(i);
            LOG3("name " << fname << " type " << f->type << " expr " << f);
            DigestFieldInfo info = std::make_tuple(fname, f->type, f);
            digestFields->push_back(info);
            i++;
        }
    }

    /**
     * Find the "first" table in which the action is present
     */
    const IR::P4Table *findTable(const IR::P4Control *control, const IR::P4Action *action) {
        auto tables = control->getDeclarations()->where([action](const IR::IDeclaration *d) {
            if (!d->is<IR::P4Table>()) return false;
            auto t = d->to<IR::P4Table>();
            LOG3("Searching for " << action->getName().name << " in table " << t->getName().name);
            return t->getActionList()->getDeclaration(action->getName()) != nullptr;
        });
        try {
            auto t = tables->single();
            LOG3("found " << t->getName().name << " as table candidate");
            return t->to<IR::P4Table>();
        } catch (...) {
            tables->reset();  // Enumerators are weird!!
            if (tables->count() > 1)
                P4C_UNIMPLEMENTED("action %1% present in multiple tables", action->getName().name);
            return nullptr;
        }
        return nullptr;
    }

    /**
     * Concatenate a field string with list of fields within a method call
     * argument. The field list is specified as a ListExpression
     */
    void generate_fields_string(const IR::MethodCallStatement *node, std::string &fieldString) {
        std::ostringstream fieldListString;
        for (auto argument : *node->methodCall->arguments) {
            if (auto fieldList = argument->expression->to<IR::ListExpression>()) {
                for (auto comp : fieldList->components) {
                    fieldListString << comp;
                }
            } else if (auto fieldList = argument->expression->to<IR::StructExpression>()) {
                for (auto comp : fieldList->components) {
                    fieldListString << comp;
                }
            } else {
                const IR::Constant *cst = getConstantFromArg(argument);
                fieldListString << cst;
            }
            fieldString += fieldListString.str();
        }
    }
    /**
     * Generate a 64-bit hash for input field string, lookup hash vector for
     * presence of hash, add new entry if not found. This function is common to
     * resubmit, digest and clone indexing
     */
    unsigned getIndex(const IR::MethodCallStatement *node,
                      std::map<unsigned long, unsigned> &hashIndexMap) {
        std::string fieldsString;
        generate_fields_string(node, fieldsString);
        std::hash<std::string> hashFn;

        auto fieldsStringHash = hashFn(fieldsString);
        if (hashIndexMap.count(fieldsStringHash) == 0)
            hashIndexMap.emplace(fieldsStringHash, hashIndexMap.size());

        return hashIndexMap[fieldsStringHash];
    }
};

/**
 * \ingroup SimpleSwitchTranslation
 * In P4-14, struct field can only be Type_Bits, therefore, when translating to
 * P4-16, all the corresponding struct field should also be Type_Bits.
 * However, target architectures such as jbay.p4 or tofino.p4 have Type_Boolean
 * in some of the struct fields, this pass will convert these fields to
 * Type_Bits.  The conversion only apply to struct fields that are defined in
 * the target architecture file and are annotated with "__intrinsic_metadata".
 */
class LoweringType : public Transform {
    std::map<cstring, unsigned> enum_encoding;

 public:
    LoweringType() {}

    const IR::Node *postorder(IR::StructField *node) override {
        auto ctxt = findOrigCtxt<IR::Type_StructLike>();
        if (!ctxt) return node;
        if (!ctxt->annotations->getSingle("__intrinsic_metadata"_cs)) return node;
        if (!node->type->is<IR::Type_Boolean>()) return node;
        return new IR::StructField(node->srcInfo, node->name, node->annotations,
                                   IR::Type_Bits::get(1));
    }

    const IR::Node *postorder(IR::Type_Enum *node) override {
        if (node->name == "MeterColor_t") enum_encoding.emplace(node->name, 2);
        return node;
    }

    const IR::Node *postorder(IR::Type_Name *node) override {
        auto name = node->path->name;
        if (enum_encoding.count(name)) {
            auto size = enum_encoding.at(name);
            return IR::Type_Bits::get(size);
        }
        return node;
    }
};

template <class T>
inline const T *findContext(const Visitor::Context *c) {
    while ((c = c->parent))
        if (auto *rv = dynamic_cast<const T *>(c->node)) return rv;
    return nullptr;
}

/**
 * \ingroup SimpleSwitchTranslation
 * Run copy propagation before translate v1model program to tna to eliminate
 * the following code pattern
 * standard_metadata_1 = standard_metadata;
 * ....
 * standard_metadata = standard_metadata_1;
 * However, we do not wish to copy propagate field reference in the clone3(),
 * recirculate(), etc, as the translation will need to convert them to
 * appropriate metadata in tna.
 */
bool skipMethodCallStatement(const Visitor::Context *ctxt, const IR::Expression *) {
    auto c = findContext<IR::MethodCallStatement>(ctxt);
    if (!c) return true;

    auto name = c->methodCall->method->to<IR::PathExpression>();
    if (!name) return true;

    if (name->path->name == "clone3" || name->path->name == "clone_preserving_field_list" ||
        name->path->name == "resubmit" || name->path->name == "resubmit_preserving_field_list" ||
        name->path->name == "digest" || name->path->name == "hash" ||
        name->path->name == "recirculate" ||
        name->path->name == "recirculate_preserving_field_list")
        return false;

    return true;
}

/** \ingroup SimpleSwitchTranslation */
bool skipCond(const Visitor::Context *ctxt, const IR::Expression *expr) {
    return skipMethodCallStatement(ctxt, expr) && BFN::skipRegisterActionOutput(ctxt, expr);
}

}  // namespace V1

bool AddAdjustByteCount::preorder(IR::Declaration_Instance *decl) {
    auto control = findContext<IR::P4Control>();
    if (!control) return false;

    if (control->name != structure->getBlockName(ProgramStructure::EGRESS)) return decl;
    if (auto type = decl->type->to<IR::Type_Specialized>()) {
        auto bType = type->baseType;
        if (auto path = bType->path) {
            auto declName = path->name;
            if (((declName == "Counter") || (declName == "Meter")) ||
                (declName == "DirectCounter") || (declName == "DirectMeter")) {
                auto *new_abc_member = new IR::Member(new IR::PathExpression("meta"),
                                                      IR::ID("__bfp4c_bridged_metadata"));
                auto new_abc_method =
                    new IR::PathExpression(IR::Type::Bits::get(32), new IR::Path("sizeInBytes"));
                auto new_abc_expr = new IR::MethodCallExpression(
                    new_abc_method,
                    new IR::Vector<IR::Type>(
                        {new IR::Type_Name("__bfp4c_bridged_metadata_header")}),
                    new IR::Vector<IR::Argument>({new IR::Argument(new_abc_member)}));
                decl->annotations =
                    decl->annotations->addAnnotation("adjust_byte_count"_cs, new_abc_expr);
                LOG3(
                    "Adding annotation "
                    "'@adjust_byte_count(sizeInBytes(meta.__bfp4c_bridge_metadata))' to decl: "
                    << decl);
                return true;
            }
        }
    }
    return true;
}

/**
 * The general work flow of architecture translation consists of the following steps:
 * - analyze original source program to build a programStructure that represent original program.
 * - construct symbol tables of each IR type.
 * - iterate through the symbol tables, and perform transformation on each entry.
 * - update program structure with transform IR node from symbol table.
 * - reprint a valid P16 program to program structure.
 */
SimpleSwitchTranslation::SimpleSwitchTranslation(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                                                 BFN_Options &options) {
    setName("Translation");
    addDebugHook(options.getDebugHook());
    refMap->setIsV1(true);
    auto typeChecking = new BFN::TypeChecking(refMap, typeMap);
    auto evaluator = new P4::EvaluatorPass(refMap, typeMap);
    auto structure = new BFN::V1::ProgramStructure;
    if (options.backward_compatible) structure->backward_compatible = true;
    auto parserChecksums = new V1::TranslateParserChecksums(structure, refMap, typeMap);
    auto collect_pragma = new CollectGlobalPragma();
    addPasses({
        collect_pragma,
        new P4::ValidateTableProperties(
            {"implementation"_cs, "size"_cs, "counters"_cs, "meters"_cs, "support_timeout"_cs}),
        new P4::LocalCopyPropagation(typeMap, typeChecking, V1::skipCond),
        new BFN::TypeChecking(refMap, typeMap, true),
        new RemoveExternMethodCallsExcludedByAnnotation,
        new V1::NormalizeProgram(),
        evaluator,
        new VisitFunctor(
            [structure, evaluator]() { structure->toplevel = evaluator->getToplevelBlock(); }),
        new TranslationFirst(),
        new V1::LoadTargetArchitecture(structure),
        new V1::RemoveNodesWithNoMapping(),
        new V1::AnalyzeProgram(structure),
        parserChecksums,
        new V1::ConstructSymbolTable(structure, refMap, typeMap, parserChecksums, *collect_pragma),
        new GenerateTofinoProgram(structure),
        new TranslationLast(),
        new V1::LoweringType(),
        new V1::InsertChecksumError(parserChecksums),
        new V1::InsertChecksumDeposit(parserChecksums),
        new AddMetadataParserStates(refMap, typeMap),
        new BFN::AddAdjustByteCount(structure, refMap, typeMap),
        new P4::EliminateSerEnums(typeMap),
        new BFN::FindArchitecture(),
        new TranslationLast(),
    });
}

}  // namespace BFN
