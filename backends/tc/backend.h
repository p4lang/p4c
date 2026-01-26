/*
Copyright (C) 2023 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions
and limitations under the License.
*/

#ifndef BACKENDS_TC_BACKEND_H_
#define BACKENDS_TC_BACKEND_H_

#include <deque>

#include "backends/ebpf/psa/ebpfPsaGen.h"
#include "control-plane/p4RuntimeArchHandler.h"
#include "ebpfCodeGen.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/parseAnnotations.h"
#include "frontends/p4/parserCallGraph.h"
#include "introspection.h"
#include "ir/ir.h"
#include "lib/error.h"
#include "lib/nullstream.h"
#include "lib/stringify.h"
#include "options.h"
#include "pnaProgramStructure.h"
#include "tcAnnotations.h"
#include "tc_defines.h"

namespace P4::TC {

extern cstring PnaMainParserInputMetaFields[TC::MAX_PNA_PARSER_META];
extern cstring PnaMainInputMetaFields[TC::MAX_PNA_INPUT_META];
extern cstring PnaMainOutputMetaFields[TC::MAX_PNA_OUTPUT_META];

class PNAEbpfGenerator;

typedef struct decllist DECLLIST;
struct decllist {
    DECLLIST *link;
    IR::Declaration *decl;
};

typedef struct stmtlist STMTLIST;
struct stmtlist {
    STMTLIST *link;
    IR::Statement *stmt;
};

typedef struct container CONTAINER;
struct container {
    CONTAINER *link;
    DECLLIST *temps;
    STMTLIST *stmts;
    IR::Statement *s;
};

enum WrType {
    WR_VALUE = 1,
    WR_CONCAT,
    WR_ADD,
    WR_SUB,
    WR_MUL,
    WR_BXSMUL,
    WR_CAST,
    WR_NEG,
    WR_NOT,
    WR_SHL_X,
    WR_SHL_C,
    WR_SHRA_X,
    WR_SHRA_C,
    WR_SHRL_X,
    WR_SHRL_C,
    WR_CMP,
    WR_BITAND,
    WR_BITOR,
    WR_BITXOR,
    WR_ADDSAT,
    WR_SUBSAT,
    WR_ASSIGN,
};

class WidthRec {
 public:
    WrType type;
    union {
        // if WR_VALUE
        int value;
        // if WR_CONCAT
        struct {
            int lhsw;
            int rhsw;
            // result width is implicit: is lhsw + rhsw
        } concat;
        // if WR_ADD, WR_SUB, WR_MUL, WR_NEG, WR_NOT, WR_BITAND,
        //  WR_BITOR, WR_BITXOR
        struct {
            int w;
        } arith;
        // if WR_BXSMUL
        struct {
            int bw;
            unsigned int sv;
        } bxsmul;
        // if WR_CAST
        struct {
            int fw;
            int tw;
        } cast;
        // if WR_SHL_X, WR_SHRA_X, WR_SHRL_X
        struct {
            int lw;
            int rw;
        } shift_x;
        // if WR_SHL_C, WR_SHRA_C, WR_SRL_C
        struct {
            int lw;
            unsigned int sv;
        } shift_c;
        // if WR_CMP
        struct {
            int w;
            unsigned char cmp;
#define CMP_EQ 0x01
#define CMP_NE 0x02
#define CMP_LT 0x03
#define CMP_GE 0x04
#define CMP_GT 0x05
#define CMP_LE 0x06
#define CMP_BASE 0x07
#define CMP_SIGNED 0x08
        } cmp;
        // if WR_ADDSAT, WR_SUBSAT
        struct {
            int w;
            bool issigned;
        } sarith;
        // if WR_ASSIGN
        struct {
            int w;
        } assign;
        // Naming union to avoid c++ warnings
    } u;

 public:
    friend bool operator<(const WidthRec &l, const WidthRec &r) {
        if (l.type != r.type) return (l.type < r.type);
        switch (l.type) {
            case WR_VALUE:
                return (l.u.value < r.u.value);
                break;
            case WR_CONCAT:
                return (
                    (l.u.concat.lhsw < r.u.concat.lhsw) ||
                    ((l.u.concat.lhsw == r.u.concat.lhsw) && (l.u.concat.rhsw < r.u.concat.rhsw)));
                break;
            case WR_ADD:
            case WR_SUB:
            case WR_MUL:
            case WR_NEG:
            case WR_NOT:
            case WR_BITAND:
            case WR_BITOR:
            case WR_BITXOR:
                return (l.u.arith.w < r.u.arith.w);
                break;
            case WR_BXSMUL:
                return ((l.u.bxsmul.bw < r.u.bxsmul.bw) ||
                        ((l.u.bxsmul.bw == r.u.bxsmul.bw) && (l.u.bxsmul.sv < r.u.bxsmul.sv)));
                break;
            case WR_CAST:
                return ((l.u.cast.fw < r.u.cast.fw) ||
                        ((l.u.cast.fw == r.u.cast.fw) && (l.u.cast.tw < r.u.cast.tw)));
                break;
            case WR_SHL_X:
            case WR_SHRA_X:
            case WR_SHRL_X:
                return ((l.u.shift_x.lw < r.u.shift_x.lw) || ((l.u.shift_x.lw == r.u.shift_x.lw) &&
                                                              ((l.u.shift_x.rw < r.u.shift_x.rw))));
                break;
            case WR_SHL_C:
            case WR_SHRA_C:
            case WR_SHRL_C:
                return ((l.u.shift_c.lw < r.u.shift_c.lw) || ((l.u.shift_c.lw == r.u.shift_c.lw) &&
                                                              ((l.u.shift_c.sv < r.u.shift_c.sv))));
                break;
            case WR_CMP:
                return ((l.u.cmp.w < r.u.cmp.w) ||
                        ((l.u.cmp.w == r.u.cmp.w) && (l.u.cmp.cmp < r.u.cmp.cmp)));
                break;
            case WR_ADDSAT:
            case WR_SUBSAT:
                return ((l.u.sarith.w < r.u.sarith.w) ||
                        ((l.u.sarith.w == r.u.sarith.w) &&
                         (!l.u.sarith.issigned && r.u.sarith.issigned)));
                break;
            case WR_ASSIGN:
                return (l.u.assign.w < r.u.assign.w);
                break;
        }
        return (0);
    }
    friend bool operator==(const WidthRec &l, const WidthRec &r) {
        if (l.type != r.type) return (0);
        switch (l.type) {
            case WR_VALUE:
                return (l.u.value == r.u.value);
                break;
            case WR_CONCAT:
                return ((l.u.concat.lhsw == r.u.concat.lhsw) &&
                        (l.u.concat.rhsw) == (r.u.concat.rhsw));
                break;
            case WR_ADD:
            case WR_SUB:
            case WR_MUL:
            case WR_NEG:
            case WR_NOT:
            case WR_BITAND:
            case WR_BITOR:
            case WR_BITXOR:
                return (l.u.arith.w == r.u.arith.w);
                break;
            case WR_BXSMUL:
                return ((l.u.bxsmul.bw == r.u.bxsmul.bw) && (l.u.bxsmul.sv == r.u.bxsmul.sv));
                break;
            case WR_CAST:
                return ((l.u.cast.fw == r.u.cast.fw) && (l.u.cast.tw == r.u.cast.tw));
                break;
            case WR_SHL_X:
            case WR_SHRA_X:
            case WR_SHRL_X:
                return ((l.u.shift_x.lw == r.u.shift_x.lw) && (l.u.shift_x.rw == r.u.shift_x.rw));
                break;
            case WR_SHL_C:
            case WR_SHRA_C:
            case WR_SHRL_C:
                return ((l.u.shift_c.lw == r.u.shift_c.lw) && (l.u.shift_c.sv == r.u.shift_c.sv));
                break;
            case WR_CMP:
                return ((l.u.cmp.w == r.u.cmp.w) && (l.u.cmp.cmp == r.u.cmp.cmp));
                break;
            case WR_ADDSAT:
            case WR_SUBSAT:
                return ((l.u.sarith.w == r.u.sarith.w) &&
                        (l.u.sarith.issigned == r.u.sarith.issigned));
                break;
            case WR_ASSIGN:
                return (l.u.assign.w == r.u.assign.w);
                break;
        }
        return (1);
    }
    friend bool operator>(const WidthRec &l, const WidthRec &r) { return (r < l); }
    friend bool operator>=(const WidthRec &l, const WidthRec &r) { return (!(l < r)); }
    friend bool operator<=(const WidthRec &l, const WidthRec &r) { return (!(r < l)); }
    friend bool operator!=(const WidthRec &l, const WidthRec &r) { return (!(l == r)); }
};

class ScanWidths : public Inspector {
 private:
    int nwr;
    WidthRec *wrv;
    P4::TypeMap *typemap;
    EBPF::Target *target;
    void insert_wr(WidthRec &);
    void add_width(int);
    void add_concat(int, int);
    void add_arith(WrType, int);
    void add_sarith(WrType, int, bool);
    void add_bxsmul(int, unsigned int);
    void add_cast(unsigned int, unsigned int);
    bool bigXSmallMul(const IR::Expression *, const IR::Constant *);
    void add_shift_c(WrType, unsigned int, unsigned int);
    void add_shift_x(WrType, unsigned int, unsigned int);
    void add_cmp(unsigned int, unsigned char);
    void add_assign(unsigned int);

 public:
    explicit ScanWidths(P4::TypeMap *tm, EBPF::Target *tgt)
        : nwr(0), wrv(0), typemap(tm), target(tgt) {}
    ~ScanWidths(void) { std::free(wrv); }
    void expr_common(const IR::Expression *);
    bool preorder(const IR::Expression *) override;
    bool preorder(const IR::Concat *) override;
    bool preorder(const IR::Add *) override;
    bool preorder(const IR::Sub *) override;
    bool preorder(const IR::Mul *) override;
    bool preorder(const IR::Cast *) override;
    bool preorder(const IR::Neg *) override;
    bool preorder(const IR::Cmpl *) override;
    bool preorder(const IR::Shl *) override;
    bool preorder(const IR::Shr *) override;
    bool preorder(const IR::Equ *) override;
    bool preorder(const IR::Neq *) override;
    bool preorder(const IR::Lss *) override;
    bool preorder(const IR::Leq *) override;
    bool preorder(const IR::Grt *) override;
    bool preorder(const IR::Geq *) override;
    bool preorder(const IR::BAnd *) override;
    bool preorder(const IR::BOr *) override;
    bool preorder(const IR::BXor *) override;
    bool preorder(const IR::AddSat *) override;
    bool preorder(const IR::SubSat *) override;
    bool preorder(const IR::AssignmentStatement *) override;
    bool arith_common_2(const IR::Operation_Binary *, WrType, bool = true);
    bool arith_common_1(const IR::Operation_Unary *, WrType, bool = true);
    bool sarith_common_2(const IR::Operation_Binary *, WrType, bool = true);
    profile_t init_apply(const IR::Node *) override;
    void end_apply(const IR::Node *) override;
    void revisit_visited();
    void dump() const;
    void gen_h(EBPF::CodeBuilder *) const;
    void gen_c(EBPF::CodeBuilder *) const;
};

/**
 * Backend code generation from midend IR
 */
class ConvertToBackendIR : public Inspector {
 public:
    struct ExternInstance {
        cstring instance_name;
        unsigned instance_id;
        bool is_num_elements;
        int num_elements;
    };
    struct ExternBlock {
        cstring externId;
        cstring control_name;
        unsigned no_of_instances;
        cstring permissions;
        safe_vector<struct ExternInstance *> eInstance;
    };
    enum CounterType { PACKETS, BYTES, PACKETS_AND_BYTES };
    const IR::ToplevelBlock *tlb;
    IR::TCPipeline *tcPipeline;
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    TCOptions &options;
    unsigned int tableCount = 0;
    unsigned int actionCount = 0;
    unsigned int metadataCount = 0;
    unsigned int labelCount = 0;
    unsigned int externCount = 0;
    cstring pipelineName = nullptr;
    cstring mainParserName = nullptr;
    ordered_map<cstring, const IR::P4Action *> actions;
    ordered_map<unsigned, cstring> tableIDList;
    ordered_map<unsigned, cstring> actionIDList;
    ordered_map<unsigned, unsigned> tableKeysizeList;
    safe_vector<const IR::P4Table *> add_on_miss_tables;
    ordered_map<cstring, std::pair<cstring, cstring> *> tablePermissions;
    ordered_map<cstring, const IR::Type_Struct *> ControlStructPerExtern;
    ordered_map<cstring, struct ExternBlock *> externsInfo;

 public:
    ConvertToBackendIR(const IR::ToplevelBlock *tlb, IR::TCPipeline *pipe, P4::ReferenceMap *refMap,
                       P4::TypeMap *typeMap, TCOptions &options)
        : tlb(tlb), tcPipeline(pipe), refMap(refMap), typeMap(typeMap), options(options) {}
    void setPipelineName();
    cstring getPipelineName() { return pipelineName; };
    bool preorder(const IR::P4Program *p) override;
    void postorder(const IR::P4Action *a) override;
    void postorder(const IR::P4Table *t) override;
    void postorder(const IR::P4Program *p) override;
    void postorder(const IR::Declaration_Instance *d) override;
    void postorder(const IR::Type_Struct *ts) override;
    safe_vector<const IR::TCKey *> processExternConstructor(const IR::Type_Extern *extn,
                                                            const IR::Declaration_Instance *decl,
                                                            struct ExternInstance *instance);
    safe_vector<const IR::TCKey *> processExternControlPath(const IR::Type_Extern *extn,
                                                            const IR::Declaration_Instance *decl,
                                                            cstring eName);
    cstring getControlPathKeyAnnotation(const IR::StructField *field);
    unsigned GetAccessNumericValue(std::string_view access);
    bool isDuplicateAction(const IR::P4Action *action);
    bool isDuplicateOrNoAction(const IR::P4Action *action);
    void updateDefaultHitAction(const IR::P4Table *t, IR::TCTable *tdef);
    void updateDefaultMissAction(const IR::P4Table *t, IR::TCTable *tdef);
    void updateConstEntries(const IR::P4Table *t, IR::TCTable *tdef);
    void updateMatchType(const IR::P4Table *t, IR::TCTable *tabledef);
    void updateTimerProfiles(IR::TCTable *tabledef);
    void updatePnaDirectCounter(const IR::P4Table *t, IR::TCTable *tabledef, unsigned tentries);
    void updatePnaDirectMeter(const IR::P4Table *t, IR::TCTable *tabledef, unsigned tentries);
    bool isPnaParserMeta(const IR::Member *mem);
    bool isPnaMainInputMeta(const IR::Member *mem);
    bool isPnaMainOutputMeta(const IR::Member *mem);
    unsigned int findMappedKernelMeta(const IR::Member *mem);
    const IR::Expression *ExtractExpFromCast(const IR::Expression *exp);
    unsigned getTcType(const IR::StringLiteral *sl);
    unsigned getTableId(cstring tableName) const;
    unsigned getActionId(cstring actionName) const;
    cstring getExternId(cstring externName) const;
    unsigned getExternInstanceId(cstring externName, cstring instanceName) const;
    cstring processExternPermission(const IR::Type_Extern *ext);
    unsigned getTableKeysize(unsigned tableId) const;
    cstring externalName(const IR::IDeclaration *declaration) const;
    cstring HandleTableAccessPermission(const IR::P4Table *t);
    std::pair<cstring, cstring> *GetAnnotatedAccessPath(const IR::Annotation *anno);
    void updateAddOnMissTable(const IR::P4Table *t);
    bool checkParameterDirection(const IR::TCAction *tcAction);
    bool hasExecuteMethod(const IR::Type_Extern *extn);
    void addExternTypeInstance(const IR::Declaration_Instance *decl,
                               IR::TCExternInstance *tcExternInstance, cstring eName);
    safe_vector<const IR::TCKey *> HandleTypeNameStructField(const IR::StructField *field,
                                                             const IR::Type_Extern *extn,
                                                             const IR::Declaration_Instance *decl,
                                                             int &kId, cstring annoName);
    safe_vector<const IR::TCKey *> processCounterControlPathKeys(
        const IR::Type_Struct *extern_control_path, const IR::Type_Extern *extn,
        const IR::Declaration_Instance *decl);
    CounterType toCounterType(const int type);
};

class Extern {
 public:
    static const cstring dropPacket;
    static const cstring sendToPort;
};

class Backend : public PassManager {
 public:
    IR::ToplevelBlock *toplevel;
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    TCOptions &options;
    IR::TCPipeline *pipeline = new IR::TCPipeline();
    TC::ConvertToBackendIR *tcIR;
    TC::IntrospectionGenerator *genIJ;
    TC::ParseTCAnnotations *parseTCAnno;
    const IR::ToplevelBlock *top = nullptr;
    EbpfOptions ebpfOption;
    EBPF::Target *target;
    const PNAEbpfGenerator *ebpf_program;
    const ScanWidths *widths;

 public:
    explicit Backend(IR::ToplevelBlock *toplevel, P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                     TCOptions &options)
        : toplevel(toplevel), refMap(refMap), typeMap(typeMap), options(options), widths(0) {
        setName("BackEnd");
    }
    bool process();
    bool ebpfCodeGen(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, const IR::P4Program *);
    void serialize() const;
    bool serializeIntrospectionJson(std::ostream &out) const;
    bool emitCFile();
};

}  // namespace P4::TC

#endif /* BACKENDS_TC_BACKEND_H_ */
