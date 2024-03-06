#include "backends/p4tools/modules/smith/common/declarations.h"

#include "backends/p4tools/modules/smith/common/expressions.h"
#include "backends/p4tools/modules/smith/common/scope.h"
#include "backends/p4tools/modules/smith/common/statements.h"
#include "backends/p4tools/modules/smith/common/table.h"

namespace P4Tools::P4Smith {

IR::Annotations *Declarations::genAnnotation() {
    Util::SourceInfo si;
    IR::Vector<IR::Annotation> annotations;
    IR::Vector<IR::Expression> exprs;
    cstring name = IR::Annotation::nameAnnotation;
    auto str_literal = new IR::StringLiteral(randStr(6));

    exprs.push_back(str_literal);

    auto annotation = new IR::Annotation(si, name, exprs, false);
    annotations.push_back(annotation);

    return new IR::Annotations(annotations);
}

IR::Declaration_Constant *Declarations::genConstantDeclaration() {
    cstring name = randStr(6);
    TyperefProbs type_percent = {
        PCT.CONSTANTDECLARATION_BASETYPE_BIT,    PCT.CONSTANTDECLARATION_BASETYPE_SIGNED_BIT,
        PCT.CONSTANTDECLARATION_BASETYPE_VARBIT, PCT.CONSTANTDECLARATION_BASETYPE_INT,
        PCT.CONSTANTDECLARATION_BASETYPE_ERROR,  PCT.CONSTANTDECLARATION_BASETYPE_BOOL,
        PCT.CONSTANTDECLARATION_BASETYPE_STRING, PCT.CONSTANTDECLARATION_DERIVED_ENUM,
        PCT.CONSTANTDECLARATION_DERIVED_HEADER,  PCT.CONSTANTDECLARATION_DERIVED_HEADER_STACK,
        PCT.CONSTANTDECLARATION_DERIVED_STRUCT,  PCT.CONSTANTDECLARATION_DERIVED_HEADER_UNION,
        PCT.CONSTANTDECLARATION_DERIVED_TUPLE,   PCT.CONSTANTDECLARATION_TYPE_VOID,
        PCT.CONSTANTDECLARATION_TYPE_MATCH_KIND,
    };

    IR::Type *tp = Expressions().pickRndType(type_percent);

    IR::Declaration_Constant *ret = nullptr;
    // constant declarations need to be compile-time known
    P4Scope::req.compile_time_known = true;

    if (tp->is<IR::Type_Bits>() || tp->is<IR::Type_InfInt>() || tp->is<IR::Type_Boolean>() ||
        tp->is<IR::Type_Name>()) {
        auto expr = Expressions().genExpression(tp);
        ret = new IR::Declaration_Constant(name, tp, expr);
    } else {
        BUG("Type %s !supported!", tp->node_type_name());
    }
    P4Scope::req.compile_time_known = false;

    P4Scope::add_to_scope(ret);

    return ret;
}

IR::P4Action *Declarations::genActionDeclaration() {
    cstring name = randStr(5);
    IR::ParameterList *params = nullptr;
    IR::BlockStatement *blk = nullptr;
    P4Scope::start_local_scope();
    P4Scope::prop.in_action = true;
    params = genParameterList();

    blk = Statements().genBlockStatement();

    auto ret = new IR::P4Action(name, params, blk);

    P4Scope::prop.in_action = false;
    P4Scope::end_local_scope();

    P4Scope::add_to_scope(ret);

    return ret;
}

IR::IndexedVector<IR::Declaration> Declarations::genLocalControlDecls() {
    IR::IndexedVector<IR::Declaration> local_decls;

    auto vars = getRndInt(DECL.MIN_VAR, DECL.MAX_VAR);
    auto decls = getRndInt(DECL.MIN_INSTANCE, DECL.MAX_INSTANCE);
    auto actions = getRndInt(DECL.MIN_ACTION, DECL.MAX_ACTION);
    auto tables = getRndInt(DECL.MIN_TABLE, DECL.MAX_TABLE);

    // variableDeclarations
    for (int i = 0; i <= vars; i++) {
        auto var_decl = genVariableDeclaration();
        local_decls.push_back(var_decl);
    }

    // declaration_instance
    for (int i = 0; i <= decls; i++) {
        auto decl_ins = genControlDeclarationInstance();

        if (decl_ins == nullptr) {
            continue;
        }
        local_decls.push_back(decl_ins);
    }

    // actionDeclarations
    for (int i = 0; i <= actions; i++) {
        auto act_decl = genActionDeclaration();
        local_decls.push_back(act_decl);
    }

    for (int i = 0; i <= tables; i++) {
        auto tab_decl = Table().genTableDeclaration();
        local_decls.push_back(tab_decl);
    }
    return local_decls;
    // instantiations
}

IR::P4Control *Declarations::genControlDeclaration() {
    // start of new scope
    P4Scope::start_local_scope();
    cstring name = randStr(7);
    IR::ParameterList *params = genParameterList();
    IR::Type_Control *type_ctrl = new IR::Type_Control(name, params);

    IR::IndexedVector<IR::Declaration> local_decls = genLocalControlDecls();
    // apply body
    auto apply_block = Statements().genBlockStatement();

    // end of scope
    P4Scope::end_local_scope();

    // add to the whole scope
    IR::P4Control *p4ctrl = new IR::P4Control(name, type_ctrl, local_decls, apply_block);
    P4Scope::add_to_scope(p4ctrl);

    return p4ctrl;
}

IR::Declaration_Instance *Declarations::genControlDeclarationInstance() {
    auto p4_ctrls = P4Scope::get_decls<IR::P4Control>();
    size_t size = p4_ctrls.size();

    if (size == 0) {
        // FIXME: Figure out a better way to handle this nullptr
        return nullptr;
    }
    IR::Vector<IR::Argument> *args = new IR::Vector<IR::Argument>();
    const IR::P4Control *p4ctrl = p4_ctrls.at(getRndInt(0, size - 1));
    IR::Type *tp = new IR::Type_Name(p4ctrl->name);
    auto decl = new IR::Declaration_Instance(cstring(randStr(6)), tp, args);
    P4Scope::add_to_scope(decl);
    return decl;
}

IR::Type *Declarations::genDerivedTypeDeclaration() { return genHeaderTypeDeclaration(); }

IR::IndexedVector<IR::Declaration_ID> Declarations::genIdentifierList(size_t len) {
    IR::IndexedVector<IR::Declaration_ID> decl_ids;
    std::set<cstring> decl_ids_name;

    for (size_t i = 0; i < len; i++) {
        cstring name = randStr(2);
        IR::Declaration_ID *decl_id = new IR::Declaration_ID(name);

        if (decl_ids_name.find(name) != decl_ids_name.end()) {
            delete name;
            delete decl_id;
            continue;
        }

        decl_ids.push_back(decl_id);
    }

    return decl_ids;
}

IR::IndexedVector<IR::SerEnumMember> Declarations::genSpecifiedIdentifier(size_t len) {
    IR::IndexedVector<IR::SerEnumMember> members;
    std::set<cstring> members_name;

    for (size_t i = 0; i < len; i++) {
        cstring name = randStr(2);
        IR::Expression *ex = Expressions().genIntLiteral();

        if (members_name.find(name) != members_name.end()) {
            delete ex;
            continue;
        }

        auto member = new IR::SerEnumMember(name, ex);

        members.push_back(member);
    }

    return members;
}
IR::IndexedVector<IR::SerEnumMember> Declarations::genSpecifiedIdentifierList(size_t len) {
    IR::IndexedVector<IR::SerEnumMember> members;
    std::set<cstring> members_name;

    for (size_t i = 0; i < len; i++) {
        cstring name = randStr(2);
        IR::Expression *ex = Expressions().genIntLiteral();

        if (members_name.find(name) != members_name.end()) {
            delete ex;
            continue;
        }

        auto member = new IR::SerEnumMember(name, ex);

        members.push_back(member);
    }

    return members;
}

IR::Type_Enum *Declarations::genEnumDeclaration(cstring name) {
    auto decl_ids = genIdentifierList(3);

    auto ret = new IR::Type_Enum(name, decl_ids);

    P4Scope::add_to_scope(ret);
    return ret;
}

IR::Type_SerEnum *Declarations::genSerEnumDeclaration(cstring name) {
    auto members = genSpecifiedIdentifierList(3);
    IR::Type_Bits *tp = Expressions::genBitType(false);

    auto ret = new IR::Type_SerEnum(name, tp, members);

    P4Scope::add_to_scope(ret);
    return ret;
}

IR::Type *Declarations::genEnumTypeDeclaration(int type) {
    cstring name = randStr(4);
    if (type == 0) {
        return genEnumDeclaration(name);
    } else {
        return genSerEnumDeclaration(name);
    }
}

IR::Method *Declarations::genExternDeclaration() {
    cstring name = randStr(7);
    IR::Type_Method *tm = nullptr;
    P4Scope::start_local_scope();
    IR::ParameterList *params = genParameterList();

    // externs have the same type restrictions as functions
    TyperefProbs type_percent = {
        PCT.FUNCTIONDECLARATION_BASETYPE_BIT,    PCT.FUNCTIONDECLARATION_BASETYPE_SIGNED_BIT,
        PCT.FUNCTIONDECLARATION_BASETYPE_VARBIT, PCT.FUNCTIONDECLARATION_BASETYPE_INT,
        PCT.FUNCTIONDECLARATION_BASETYPE_ERROR,  PCT.FUNCTIONDECLARATION_BASETYPE_BOOL,
        PCT.FUNCTIONDECLARATION_BASETYPE_STRING, PCT.FUNCTIONDECLARATION_DERIVED_ENUM,
        PCT.FUNCTIONDECLARATION_DERIVED_HEADER,  PCT.FUNCTIONDECLARATION_DERIVED_HEADER_STACK,
        PCT.FUNCTIONDECLARATION_DERIVED_STRUCT,  PCT.FUNCTIONDECLARATION_DERIVED_HEADER_UNION,
        PCT.FUNCTIONDECLARATION_DERIVED_TUPLE,   PCT.FUNCTIONDECLARATION_TYPE_VOID,
        PCT.FUNCTIONDECLARATION_TYPE_MATCH_KIND,
    };
    IR::Type *return_type = Expressions().pickRndType(type_percent);
    tm = new IR::Type_Method(return_type, params, name);
    auto ret = new IR::Method(name, tm);
    P4Scope::end_local_scope();
    P4Scope::add_to_scope(ret);
    return ret;
}

IR::Function *Declarations::genFunctionDeclaration() {
    cstring name = randStr(7);
    IR::Type_Method *tm = nullptr;
    IR::BlockStatement *blk = nullptr;
    P4Scope::start_local_scope();
    IR::ParameterList *params = genParameterList();

    TyperefProbs type_percent = {
        PCT.FUNCTIONDECLARATION_BASETYPE_BIT,    PCT.FUNCTIONDECLARATION_BASETYPE_SIGNED_BIT,
        PCT.FUNCTIONDECLARATION_BASETYPE_VARBIT, PCT.FUNCTIONDECLARATION_BASETYPE_INT,
        PCT.FUNCTIONDECLARATION_BASETYPE_ERROR,  PCT.FUNCTIONDECLARATION_BASETYPE_BOOL,
        PCT.FUNCTIONDECLARATION_BASETYPE_STRING, PCT.FUNCTIONDECLARATION_DERIVED_ENUM,
        PCT.FUNCTIONDECLARATION_DERIVED_HEADER,  PCT.FUNCTIONDECLARATION_DERIVED_HEADER_STACK,
        PCT.FUNCTIONDECLARATION_DERIVED_STRUCT,  PCT.FUNCTIONDECLARATION_DERIVED_HEADER_UNION,
        PCT.FUNCTIONDECLARATION_DERIVED_TUPLE,   PCT.FUNCTIONDECLARATION_TYPE_VOID,
        PCT.FUNCTIONDECLARATION_TYPE_MATCH_KIND,
    };
    IR::Type *return_type = Expressions().pickRndType(type_percent);
    tm = new IR::Type_Method(return_type, params, name);

    P4Scope::prop.ret_type = return_type;
    blk = Statements().genBlockStatement(true);
    P4Scope::prop.ret_type = nullptr;

    auto ret = new IR::Function(name, tm, blk);
    P4Scope::end_local_scope();
    P4Scope::add_to_scope(ret);
    return ret;
}

IR::Type_Header *Declarations::genEthernetHeaderType() {
    IR::IndexedVector<IR::StructField> fields;
    auto eth_dst = new IR::StructField("dst_addr", new IR::Type_Bits(48, false));
    auto eth_src = new IR::StructField("src_addr", new IR::Type_Bits(48, false));
    auto eth_type = new IR::StructField("eth_type", new IR::Type_Bits(16, false));

    fields.push_back(eth_dst);
    fields.push_back(eth_src);
    fields.push_back(eth_type);

    auto ret = new IR::Type_Header(IR::ID(ETH_HEADER_T), fields);
    P4Scope::add_to_scope(ret);

    return ret;
}

IR::Type_Header *Declarations::genHeaderTypeDeclaration() {
    cstring name = randStr(6);
    IR::IndexedVector<IR::StructField> fields;
    TyperefProbs type_percent = {
        PCT.HEADERTYPEDECLARATION_BASETYPE_BIT,    PCT.HEADERTYPEDECLARATION_BASETYPE_SIGNED_BIT,
        PCT.HEADERTYPEDECLARATION_BASETYPE_VARBIT, PCT.HEADERTYPEDECLARATION_BASETYPE_INT,
        PCT.HEADERTYPEDECLARATION_BASETYPE_ERROR,  PCT.HEADERTYPEDECLARATION_BASETYPE_BOOL,
        PCT.HEADERTYPEDECLARATION_BASETYPE_STRING, PCT.HEADERTYPEDECLARATION_DERIVED_ENUM,
        PCT.HEADERTYPEDECLARATION_DERIVED_HEADER,  PCT.HEADERTYPEDECLARATION_DERIVED_HEADER_STACK,
        PCT.HEADERTYPEDECLARATION_DERIVED_STRUCT,  PCT.HEADERTYPEDECLARATION_DERIVED_HEADER_UNION,
        PCT.HEADERTYPEDECLARATION_DERIVED_TUPLE,   PCT.HEADERTYPEDECLARATION_TYPE_VOID,
        PCT.HEADERTYPEDECLARATION_TYPE_MATCH_KIND,
    };

    size_t len = getRndInt(1, 5);
    for (size_t i = 0; i < len; i++) {
        cstring field_name = randStr(4);
        IR::Type *field_tp = Expressions().pickRndType(type_percent);

        if (auto struct_tp = field_tp->to<IR::Type_Struct>()) {
            field_tp = new IR::Type_Name(struct_tp->name);
        }
        IR::StructField *sf = new IR::StructField(field_name, field_tp);
        fields.push_back(sf);
    }
    auto ret = new IR::Type_Header(name, fields);
    if (P4Scope::req.byte_align_headers) {
        auto remainder = ret->width_bits() % 8;
        if (remainder) {
            auto pad_bit = new IR::Type_Bits(8 - remainder, false);
            auto pad_field = new IR::StructField("padding", pad_bit);
            ret->fields.push_back(pad_field);
        }
    }
    P4Scope::add_to_scope(ret);

    return ret;
}

IR::Type_HeaderUnion *Declarations::genHeaderUnionDeclaration() {
    cstring name = randStr(6);

    IR::IndexedVector<IR::StructField> fields;
    auto l_types = P4Scope::get_decls<IR::Type_Header>();
    if (l_types.size() < 2) {
        BUG("Creating a header union assumes at least two headers!");
    }
    // !sure if this correct...
    size_t len = getRndInt(2, l_types.size() - 2);
    std::set<cstring> visited_headers;
    // we need to guarantee correct execution so try as long as we can
    // this is a bit dicey... do !like it
    int attempts = 0;
    while (true) {
        if (attempts >= 100) {
            BUG("We should !need this many attempts!");
        }
        attempts++;
        cstring field_name = randStr(4);
        auto hdr_tp = l_types.at(getRndInt(0, l_types.size() - 1));
        // check if we have already added this header
        if (visited_headers.find(hdr_tp->name) != visited_headers.end()) {
            continue;
        }
        auto tp_name = new IR::Type_Name(hdr_tp->name);
        IR::StructField *sf = new IR::StructField(field_name, tp_name);
        visited_headers.insert(hdr_tp->name);
        fields.push_back(sf);
        if (fields.size() == len) {
            break;
        }
    }

    auto ret = new IR::Type_HeaderUnion(name, fields);

    P4Scope::add_to_scope(ret);

    return ret;
}

IR::Type *Declarations::genHeaderStackType() {
    auto l_types = P4Scope::get_decls<IR::Type_Header>();
    if (l_types.size() < 1) {
        BUG("Creating a header stacks assumes at least one declared header!");
    }
    auto hdr_tp = l_types.at(getRndInt(0, l_types.size() - 1));
    auto stack_size = getRndInt(1, MAX_HEADER_STACK_SIZE);
    auto hdr_type_name = new IR::Type_Name(hdr_tp->name);
    auto ret = new IR::Type_Stack(hdr_type_name, new IR::Constant(stack_size));

    P4Scope::add_to_scope(ret);

    return ret;
}

IR::Type_Struct *Declarations::genStructTypeDeclaration() {
    cstring name = randStr(6);

    IR::IndexedVector<IR::StructField> fields;
    TyperefProbs type_percent = {
        PCT.STRUCTTYPEDECLARATION_BASETYPE_BIT,    PCT.STRUCTTYPEDECLARATION_BASETYPE_SIGNED_BIT,
        PCT.STRUCTTYPEDECLARATION_BASETYPE_VARBIT, PCT.STRUCTTYPEDECLARATION_BASETYPE_INT,
        PCT.STRUCTTYPEDECLARATION_BASETYPE_ERROR,  PCT.STRUCTTYPEDECLARATION_BASETYPE_BOOL,
        PCT.STRUCTTYPEDECLARATION_BASETYPE_STRING, PCT.STRUCTTYPEDECLARATION_DERIVED_ENUM,
        PCT.STRUCTTYPEDECLARATION_DERIVED_HEADER,  PCT.STRUCTTYPEDECLARATION_DERIVED_HEADER_STACK,
        PCT.STRUCTTYPEDECLARATION_DERIVED_STRUCT,  PCT.STRUCTTYPEDECLARATION_DERIVED_HEADER_UNION,
        PCT.STRUCTTYPEDECLARATION_DERIVED_TUPLE,   PCT.STRUCTTYPEDECLARATION_TYPE_VOID,
        PCT.STRUCTTYPEDECLARATION_TYPE_MATCH_KIND,
    };
    auto l_types = P4Scope::get_decls<IR::Type_Header>();
    if (l_types.size() == 0) {
        return nullptr;
    }
    size_t len = getRndInt(1, 5);

    for (size_t i = 0; i < len; i++) {
        IR::Type *field_tp = Expressions().pickRndType(type_percent);
        cstring field_name = randStr(4);
        if (field_tp->to<IR::Type_Stack>()) {
            // Right now there is now way to initialize a header stack
            // So we have to add the entire structure to the banned expressions
            P4Scope::not_initialized_structs.insert(name);
        }
        IR::StructField *sf = new IR::StructField(field_name, field_tp);
        fields.push_back(sf);
    }

    auto ret = new IR::Type_Struct(name, fields);

    P4Scope::add_to_scope(ret);

    return ret;
}

IR::Type_Struct *Declarations::genHeaderStruct() {
    IR::IndexedVector<IR::StructField> fields;

    // Tao: hard code for ethernet_t eth_hdr;
    auto eth_sf = new IR::StructField(ETH_HDR, new IR::Type_Name(ETH_HEADER_T));
    fields.push_back(eth_sf);

    size_t len = getRndInt(1, 5);
    // we can only generate very specific types for headers
    // header, header stack, header union
    std::vector<int64_t> percent = {PCT.STRUCTTYPEDECLARATION_HEADERS_HEADER,
                                    PCT.STRUCTTYPEDECLARATION_HEADERS_STACK};
    for (size_t i = 0; i < len; i++) {
        cstring field_name = randStr(4);
        IR::Type *tp = nullptr;
        switch (randInt(percent)) {
            case 0: {
                // TODO: We have to assume that this works
                auto l_types = P4Scope::get_decls<IR::Type_Header>();
                if (l_types.size() == 0) {
                    BUG("structTypeDeclaration: No available header for Headers!");
                }
                auto candidate_type = l_types.at(getRndInt(0, l_types.size() - 1));
                tp = new IR::Type_Name(candidate_type->name.name);
                break;
            }
            case 1: {
                tp = genHeaderStackType();
                // Right now there is now way to initialize a header stack
                // So we have to add the entire structure to the banned expressions
                P4Scope::not_initialized_structs.insert("Headers");
            }
        }
        fields.push_back(new IR::StructField(field_name, tp));
    }
    auto ret = new IR::Type_Struct("Headers", fields);

    P4Scope::add_to_scope(ret);

    return ret;
}

IR::Type_Declaration *Declarations::genTypeDeclaration() {
    std::vector<int64_t> percent = {PCT.TYPEDECLARATION_HEADER, PCT.TYPEDECLARATION_STRUCT,
                                    PCT.TYPEDECLARATION_UNION};
    IR::Type_Declaration *decl = nullptr;
    bool use_default_decl = false;
    switch (randInt(percent)) {
        case 0: {
            use_default_decl = true;
            break;
        }
        case 1: {
            decl = genStructTypeDeclaration();
            break;
        }
        case 2: {
            // header unions are disabled for now, need to fix assignments
            auto hdrs = P4Scope::get_decls<IR::Type_Header>();
            // we can only generate a union if we have at least two headers
            if (hdrs.size() > 1) {
                decl = genHeaderUnionDeclaration();
                if (!decl) {
                    use_default_decl = true;
                }
            } else {
                use_default_decl = true;
            }
            break;
        }
    }
    if (use_default_decl) decl = genHeaderTypeDeclaration();

    return decl;
}

IR::Type *Declarations::genType() {
    std::vector<int64_t> percent = {PCT.TYPEDEFDECLARATION_BASE, PCT.TYPEDEFDECLARATION_STRUCTLIKE,
                                    PCT.TYPEDEFDECLARATION_STACK};

    std::vector<int64_t> type_probs = {
        PCT.TYPEDEFDECLARATION_BASETYPE_BOOL,  PCT.TYPEDEFDECLARATION_BASETYPE_ERROR,
        PCT.TYPEDEFDECLARATION_BASETYPE_INT,   PCT.TYPEDEFDECLARATION_BASETYPE_STRING,
        PCT.TYPEDEFDECLARATION_BASETYPE_BIT,   PCT.TYPEDEFDECLARATION_BASETYPE_SIGNED_BIT,
        PCT.TYPEDEFDECLARATION_BASETYPE_VARBIT};
    IR::Type *tp = nullptr;
    switch (randInt(percent)) {
        case 0: {
            std::vector<int> b_types = {1};  // only bit<>
            tp = Expressions().pickRndBaseType(type_probs);
            break;
        }
        case 1: {
            auto l_types = P4Scope::get_decls<IR::Type_StructLike>();
            if (l_types.size() == 0) {
                return nullptr;
            }
            auto candidate_type = l_types.at(getRndInt(0, l_types.size() - 1));
            tp = new IR::Type_Name(candidate_type->name.name);
            break;
        }
        case 2: {
            // tp = headerStackType::gen();
            break;
        }
    }
    return tp;
}

IR::Type_Typedef *Declarations::genTypeDef() {
    cstring name = randStr(5);
    IR::Type *type = genType();
    auto ret = new IR::Type_Typedef(name, type);
    P4Scope::add_to_scope(ret);
    return ret;
}

IR::Type_Newtype *Declarations::genNewtype() {
    cstring name = randStr(5);
    IR::Type *type = nullptr;

    auto ret = new IR::Type_Newtype(name, type);
    P4Scope::add_to_scope(ret);
    return ret;
}

IR::Type *Declarations::genTypeDefOrNewType() {
    // TODO: we only have typedef now, no newtype
    return genTypeDef();
}

IR::Declaration_Variable *Declarations::genVariableDeclaration() {
    cstring name = randStr(6);

    TyperefProbs type_percent = {
        PCT.VARIABLEDECLARATION_BASETYPE_BIT,    PCT.VARIABLEDECLARATION_BASETYPE_SIGNED_BIT,
        PCT.VARIABLEDECLARATION_BASETYPE_VARBIT, PCT.VARIABLEDECLARATION_BASETYPE_INT,
        PCT.VARIABLEDECLARATION_BASETYPE_ERROR,  PCT.VARIABLEDECLARATION_BASETYPE_BOOL,
        PCT.VARIABLEDECLARATION_BASETYPE_STRING, PCT.VARIABLEDECLARATION_DERIVED_ENUM,
        PCT.VARIABLEDECLARATION_DERIVED_HEADER,  PCT.VARIABLEDECLARATION_DERIVED_HEADER_STACK,
        PCT.VARIABLEDECLARATION_DERIVED_STRUCT,  PCT.VARIABLEDECLARATION_DERIVED_HEADER_UNION,
        PCT.VARIABLEDECLARATION_DERIVED_TUPLE,   PCT.VARIABLEDECLARATION_TYPE_VOID,
        PCT.VARIABLEDECLARATION_TYPE_MATCH_KIND,
    };

    IR::Type *tp = Expressions().pickRndType(type_percent);

    IR::Declaration_Variable *ret = nullptr;

    if (tp->is<IR::Type_Bits>() || tp->is<IR::Type_InfInt>() || tp->is<IR::Type_Boolean>() ||
        tp->is<IR::Type_Name>()) {
        auto expr = Expressions().genExpression(tp);
        ret = new IR::Declaration_Variable(name, tp, expr);
    } else if (tp->is<IR::Type_Stack>()) {
        // header stacks do !have an initializer yet
        ret = new IR::Declaration_Variable(name, tp);
    } else {
        BUG("Type %s !supported!", tp->node_type_name());
    }

    P4Scope::add_to_scope(ret);

    return ret;
}

IR::Parameter *Declarations::genTypedParameter(bool if_none_dir) {
    cstring name = randStr(4);
    IR::Type *tp;
    IR::Direction dir;
    TyperefProbs type_percent;

    if (if_none_dir) {
        type_percent = {
            PCT.PARAMETER_NONEDIR_BASETYPE_BIT,    PCT.PARAMETER_NONEDIR_BASETYPE_SIGNED_BIT,
            PCT.PARAMETER_NONEDIR_BASETYPE_VARBIT, PCT.PARAMETER_NONEDIR_BASETYPE_INT,
            PCT.PARAMETER_NONEDIR_BASETYPE_ERROR,  PCT.PARAMETER_NONEDIR_BASETYPE_BOOL,
            PCT.PARAMETER_NONEDIR_BASETYPE_STRING, PCT.PARAMETER_NONEDIR_DERIVED_ENUM,
            PCT.PARAMETER_NONEDIR_DERIVED_HEADER,  PCT.PARAMETER_NONEDIR_DERIVED_HEADER_STACK,
            PCT.PARAMETER_NONEDIR_DERIVED_STRUCT,  PCT.PARAMETER_NONEDIR_DERIVED_HEADER_UNION,
            PCT.PARAMETER_NONEDIR_DERIVED_TUPLE,   PCT.PARAMETER_NONEDIR_TYPE_VOID,
            PCT.PARAMETER_NONEDIR_TYPE_MATCH_KIND,
        };
        dir = IR::Direction::None;
    } else {
        type_percent = {
            PCT.PARAMETER_BASETYPE_BIT,    PCT.PARAMETER_BASETYPE_SIGNED_BIT,
            PCT.PARAMETER_BASETYPE_VARBIT, PCT.PARAMETER_BASETYPE_INT,
            PCT.PARAMETER_BASETYPE_ERROR,  PCT.PARAMETER_BASETYPE_BOOL,
            PCT.PARAMETER_BASETYPE_STRING, PCT.PARAMETER_DERIVED_ENUM,
            PCT.PARAMETER_DERIVED_HEADER,  PCT.PARAMETER_DERIVED_HEADER_STACK,
            PCT.PARAMETER_DERIVED_STRUCT,  PCT.PARAMETER_DERIVED_HEADER_UNION,
            PCT.PARAMETER_DERIVED_TUPLE,   PCT.PARAMETER_TYPE_VOID,
            PCT.PARAMETER_TYPE_MATCH_KIND,
        };
        std::vector<int64_t> dir_percent = {PCT.PARAMETER_DIR_IN, PCT.PARAMETER_DIR_OUT,
                                            PCT.PARAMETER_DIR_INOUT};
        switch (randInt(dir_percent)) {
            case 0:
                dir = IR::Direction::In;
                break;
            case 1:
                dir = IR::Direction::Out;
                break;
            case 2:
                dir = IR::Direction::InOut;
                break;
            default:
                dir = IR::Direction::None;
        }
    }
    tp = Expressions().pickRndType(type_percent);

    return new IR::Parameter(name, dir, tp);
}
IR::Parameter *Declarations::genParameter(IR::Direction dir, cstring p_name, cstring t_name) {
    IR::Type *tp = new IR::Type_Name(new IR::Path(t_name));
    return new IR::Parameter(p_name, dir, tp);
}
IR::ParameterList *Declarations::genParameterList() {
    IR::IndexedVector<IR::Parameter> params;
    size_t total_params = getRndInt(0, 3);
    size_t num_dir_params = total_params ? getRndInt(0, total_params - 1) : 0;
    size_t num_directionless_params = total_params - num_dir_params;
    for (size_t i = 0; i < num_dir_params; i++) {
        IR::Parameter *param = genTypedParameter(false);
        if (param == nullptr) {
            BUG("param is null");
        }
        params.push_back(param);
        // add to the scope
        P4Scope::add_to_scope(param);
        // only add values that are !read-only to the modifiable types
        if (param->direction == IR::Direction::In) {
            P4Scope::add_lval(param->type, param->name.name, true);
        } else {
            P4Scope::add_lval(param->type, param->name.name, false);
        }
    }
    for (size_t i = 0; i < num_directionless_params; i++) {
        IR::Parameter *param = genTypedParameter(true);

        if (param == nullptr) {
            BUG("param is null");
        }
        params.push_back(param);
        // add to the scope
        P4Scope::add_to_scope(param);
        P4Scope::add_lval(param->type, param->name.name, true);
    }

    return new IR::ParameterList(params);
}

}  // namespace P4Tools::P4Smith
