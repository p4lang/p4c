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

#ifndef BF_P4C_MAU_GEN_PRIM_JSON_H_
#define BF_P4C_MAU_GEN_PRIM_JSON_H_

#include "lib/json.h"
#include "bf-p4c/common/asm_output.h"
#include "bf-p4c/mau/ixbar_expr.h"
#include "bf-p4c/mau/mau_visitor.h"

// Generate Primitive Info for actions before instruction adjustment. Once
// instruction adjustment is applied it merges/splits instructions and we loose
// the initial p4 info on the operands. This info is passed off to the assembler
// to plug into respective actions which is then picked up by the model for
// logging
// Following Primitives are supported:
// - ModifyFieldPrimitive
// - DirectAluPrimitive
// - ExecuteStatefulAluPrimitive
// - DropPrimitive
// - AddHeaderPrimitive
// - RemoveHeaderPrimitive
// - ShiftPrimitive
// - ExecuteMeterPrimitive
//
// Primitives info is generated for model logging and is associated with
// a 'primitives' node (in context.json) for each action within a table.
//
// The 'GeneratePrimitiveInfo' pass does the job of generating a
// <testname>.prim.json file with all action primitives and is later
// merged into the context.json by the assembler. This is separated out
// from being generated directly into the assembly file purely to keep
// the assembly concise and readable.
//
// The primitives node requires instruction details like destination
// field, source operand info and operation type which requires the
// compiler to have done instruction selection and phv allocation.
//
// This pass should however always be called before any instruction
// adjustment (splitting) occurs as the logging only needs to output the
// overall instruction execution as specified in the p4 program. This
// should also happen before table placement which can cause table
// splitting across stages.
//
// TBD: Compiler while optimizing may split tables across
// stages. This could result in multiple scenarios - e.g.
// - all split stages have same set of actions
// - one split stage has a different set of actions as compared to the others
// - one split stage is a no match table and others have a gateway
// - one split stage has an indirect resource and others dont
// - one split stage has a partial action completed in the other stages
//
// With current schema the action primitives (used for logging) are
// populated per table and are stage agnostic. They assume the actions
// are same in each stage.
//
// Model logging in such cases will be inconsistent/missing since model
// logs actions per stage. This will require an update to the schema to
// represent the actions node within a stage_table.  In addition to
// actions, the indirect_resource node may also need to be moved
// similarly.
//
// Overall this is a significant change as the updated schema
// must be supported by the driver/model. The schema should also convey
// the original p4 action and how it is split across the stages to give
// a clear idea during logging.

using namespace P4;

class GeneratePrimitiveInfo : public MauInspector {
 private:
    const PhvInfo &phv;
    Util::JsonObject &_primNode;
    Util::JsonArray *_tables = nullptr;
    bool preorder(const IR::MAU::Table *tbl) override;
    void add_primitive(Util::JsonArray *primitives, Util::JsonObject *prim);
    void gen_action_json(const IR::MAU::Table *tbl,
            const IR::MAU::Action *act, Util::JsonObject *_action);
    Util::JsonObject *add_op_json(Util::JsonObject *prim, const std::string op,
            const std::string type, cstring name);
    void validate_add_op_json(Util::JsonObject *_primitive, const std::string
            op_name, const IR::Expression *exp);
    Util::JsonObject *add_stful_op_json(Util::JsonObject *prim, const
            std::string op, const std::string op_pfx, const std::string type,
            cstring name);
    void add_hash_dist_json(Util::JsonObject *_primitive, const std::string
            prim_name, const std::string dst_type, const cstring dst_name,
            const IR::Expression *dst, const IR::MAU::HashDist *hd);
    Visitor::profile_t init_apply(const IR::Node *root) override;
    void end_apply() override;

 public:
    explicit GeneratePrimitiveInfo(const PhvInfo &p, Util::JsonObject &primNode)
        : phv(p), _primNode(primNode) {
        visitDagOnce = false;
        _tables = new Util::JsonArray();
    }
};

#endif /* BF_P4C_MAU_GEN_PRIM_JSON_H_ */
