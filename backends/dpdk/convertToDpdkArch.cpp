/**
 * DPDK architecture assume the following control block signature
 *
 * control ingress(header h, metadata m);
 * control egress(header h, metadata m);
 *
 * We need to convert psa control blocks to this form.
 */

#include "convertToDpdkArch.h"
#include "sexp.h"

namespace DPDK {

bool isSimpleExpression(const IR::Expression *e){
    if(e->is<IR::Member>() or
        e->is<IR::PathExpression>() or
        e->is<IR::Constant>() or
        e->is<IR::BoolLiteral>())
        return true;
    return false;
}

cstring TypeStruct2Name(const cstring s){
    if(s == "psa_ingress_parser_input_metadata_t"){
        return "psa_ingress_parser_input_metadata";
    }
    else if(s == "psa_ingress_input_metadata_t"){
        return "psa_ingress_input_metadata";
    }
    else if(s == "psa_ingress_output_metadata_t"){
        return "psa_ingress_output_metadata";
    }
    else if(s == "psa_egress_parser_input_metadata_t"){
        return "psa_egress_parser_input_metadata";
    }
    else if(s == "psa_egress_input_metadata_t"){
        return "psa_egress_input_metadata";
    }
    else if(s == "psa_egress_output_metadata_t"){
        return "psa_egress_output_metadata";
    }
    else if(s == "psa_egress_deparser_input_metadata_t"){
        return "psa_egress_deparser_input_metadata";
    }
    else{
        return "local_metadata";
    }
}

const IR::Node* ConvertToDpdkArch::postorder(IR::P4Program* p) {
    // std::cout << p << std::endl;
    return p;
}

const IR::Type_Control* ConvertToDpdkArch::rewriteControlType(const IR::Type_Control* c, cstring name) {
    auto applyParams = new IR::ParameterList();
    if (name == "Ingress" || name == "Egress") {
        applyParams->push_back(c->applyParams->parameters.at(0));
        applyParams->push_back(c->applyParams->parameters.at(1));
    } else if (name == "IngressDeparser") {
        applyParams->push_back(c->applyParams->parameters.at(0));
        applyParams->push_back(c->applyParams->parameters.at(4));
        applyParams->push_back(c->applyParams->parameters.at(5));
    } else if (name == "EgressDeparser") {
        applyParams->push_back(c->applyParams->parameters.at(0));
        applyParams->push_back(c->applyParams->parameters.at(3));
        applyParams->push_back(c->applyParams->parameters.at(4));
    }
    auto tc = new IR::Type_Control(c->name, c->annotations,
            c->typeParameters, applyParams);
    return tc;
}

// translate control block signature in arch.p4
const IR::Node* ConvertToDpdkArch::postorder(IR::Type_Control* c) {
    auto ctxt = findOrigCtxt<IR::P4Control>();
    if (ctxt)
        return c;
    return rewriteControlType(c, c->name);
}

const IR::Type_Parser* ConvertToDpdkArch::rewriteParserType(const IR::Type_Parser* p, cstring name) {

    auto applyParams = new IR::ParameterList();
    if (name == "IngressParser") {
        applyParams->push_back(p->applyParams->parameters.at(0));
        applyParams->push_back(p->applyParams->parameters.at(1));
        applyParams->push_back(p->applyParams->parameters.at(2));
    } else if (name == "EgressParser") {
        applyParams->push_back(p->applyParams->parameters.at(0));
        applyParams->push_back(p->applyParams->parameters.at(1));
        applyParams->push_back(p->applyParams->parameters.at(2));
    }
    auto tp = new IR::Type_Parser(p->name, p->annotations,
            p->typeParameters, applyParams);

    return tp;
}

const IR::Node* ConvertToDpdkArch::postorder(IR::Type_Parser* p) {
    auto ctxt = findOrigCtxt<IR::P4Parser>();
    if (ctxt)
        return p;
    return rewriteParserType(p, p->name);
}

const IR::Node* ConvertToDpdkArch::postorder(IR::P4Control* c) {
    auto orig = getOriginal();
    if (block_info->count(orig) != 0) {
        auto bi = block_info->at(orig);
        LOG1("bi " << bi.pipe << " " << bi.gress << " " << bi.block);
        auto tc = rewriteControlType(c->type, bi.pipe);
        auto cont = new IR::P4Control(c->name, tc, c->constructorParams,
                c->controlLocals, c->body);
        LOG1(cont);
        return cont;
    }
    return c;
}

const IR::Node* ConvertToDpdkArch::postorder(IR::P4Parser* p) {
    auto orig = getOriginal();
    if (block_info->count(orig) != 0) {
        auto bi = block_info->at(orig);
        LOG1("bi " << bi.pipe << " " << bi.gress << " " << bi.block);
        auto tp = rewriteParserType(p->type, bi.pipe);
        auto prsr = new IR::P4Parser(p->name, tp, p->constructorParams,
                p->parserLocals, p->states);
        LOG1(prsr);
        return prsr;
    }
    return p;
}

const IR::Node* ConvertToDpdkArch::postorder(IR::Type_StructLike* s) {
    return s;
}

const IR::Node* ConvertToDpdkArch::postorder(IR::PathExpression* p) {
    return p;
}

const IR::Node* ConvertToDpdkArch::postorder(IR::Member* m) {
    LOG1("m " << m);
    return m;
}

void ParsePsa::parseIngressPipeline(const IR::PackageBlock* block) {
    auto ingress_parser = block->getParameterValue("ip");
    BlockInfo ip("IngressParser", INGRESS, PARSER);
    BUG_CHECK(ingress_parser->is<IR::ParserBlock>(), "Expected ParserBlock");
    toBlockInfo.emplace(ingress_parser->to<IR::ParserBlock>()->container, ip);

    auto ingress = block->getParameterValue("ig");
    BlockInfo ig("Ingress", INGRESS, PIPELINE);
    BUG_CHECK(ingress->is<IR::ControlBlock>(), "Expected ControlBlock");
    toBlockInfo.emplace(ingress->to<IR::ControlBlock>()->container, ig);

    auto ingress_deparser = block->getParameterValue("id");
    BUG_CHECK(ingress_deparser->is<IR::ControlBlock>(), "Expected ControlBlock");
    BlockInfo id("IngressDeparser", INGRESS, DEPARSER);
    toBlockInfo.emplace(ingress_deparser->to<IR::ControlBlock>()->container, id);
}

void ParsePsa::parseEgressPipeline(const IR::PackageBlock* block) {
    auto egress_parser = block->getParameterValue("ep");
    BUG_CHECK(egress_parser->is<IR::ParserBlock>(), "Expected ParserBlock");
    BlockInfo ep("EgressParser", EGRESS, PARSER);
    toBlockInfo.emplace(egress_parser->to<IR::ParserBlock>()->container, ep);

    auto egress = block->getParameterValue("eg");
    BUG_CHECK(egress->is<IR::ControlBlock>(), "Expected ControlBlock");
    BlockInfo eg("Egress", EGRESS, PIPELINE);
    toBlockInfo.emplace(egress->to<IR::ControlBlock>()->container, eg);

    auto egress_deparser = block->getParameterValue("ed");
    BUG_CHECK(egress_deparser->is<IR::ControlBlock>(), "Expected ControlBlock");
    BlockInfo ed("EgressDeparser", EGRESS, DEPARSER);
    toBlockInfo.emplace(egress_deparser->to<IR::ControlBlock>()->container, ed);
}

bool ParsePsa::preorder(const IR::PackageBlock* block) {
    auto decl = block->node->to<IR::Declaration_Instance>();
    // If no declaration found (anonymous instantiation) get the pipe name from arch definition
    cstring pipe = decl->Name();
    BUG_CHECK(!pipe.isNullOrEmpty(),
        "Cannot determine pipe name for pipe block");

    auto ingress = block->getParameterValue("ingress");
    if (auto block = ingress->to<IR::PackageBlock>())
        parseIngressPipeline(block);
    auto egress = block->getParameterValue("egress");
    if (auto block = egress->to<IR::PackageBlock>())
        parseEgressPipeline(block);

    // collect user-provided structure
    LOG1(block->instanceType);
    // for (auto n : block->instanceType->constantValue) {
    //     LOG1("n " << n);
    // }
    return false;
}

// bool ParsePsa::preorder(const IR::Type_Struct* s){
//     s->fields
// }

void CollectMetadataHeaderInfo::pushMetadata(const IR::Parameter* p){
    
    for(auto m: used_metadata){
        // std::cout << "m:" << m->type->to<IR::Type_Name>()->path->name.name << std::endl;
        // std::cout << "p:" << p->type->to<IR::Type_Name>()->path->name.name << std::endl;
        if(m->to<IR::Type_Name>()->path->name.name == p->type->to<IR::Type_Name>()->path->name.name){
            return;
        }
    }
    // std::cout <<"pushed:" << p->type->to<IR::Type_Name>()->path->name.name << std::endl;
    used_metadata.push_back(p->type);
}

bool CollectMetadataHeaderInfo::preorder(const IR::P4Program* p){
    for(auto kv : *toBlockInfo){
        if(kv.second.pipe == "IngressParser"){
            auto parser = kv.first->to<IR::P4Parser>();
            auto local_metadata = parser->getApplyParameters()->getParameter(2);
            local_metadata_type = local_metadata->type->to<IR::Type_Name>()->path->name;
            auto header = parser->getApplyParameters()->getParameter(1);
            header_type = header->type->to<IR::Type_Name>()->path->name;
            pushMetadata(parser->getApplyParameters()->getParameter(2));
            pushMetadata(parser->getApplyParameters()->getParameter(3));
            pushMetadata(parser->getApplyParameters()->getParameter(4));
            pushMetadata(parser->getApplyParameters()->getParameter(5));
        }
        else if(kv.second.pipe == "Ingress"){
            auto control = kv.first->to<IR::P4Control>();
            pushMetadata(control->getApplyParameters()->getParameter(1));
            pushMetadata(control->getApplyParameters()->getParameter(2));
            pushMetadata(control->getApplyParameters()->getParameter(3));
        }
        else if(kv.second.pipe == "IngressParser"){
            auto deparser = kv.first->to<IR::P4Control>();
            pushMetadata(deparser->getApplyParameters()->getParameter(1));
            pushMetadata(deparser->getApplyParameters()->getParameter(2));
            pushMetadata(deparser->getApplyParameters()->getParameter(3));
            pushMetadata(deparser->getApplyParameters()->getParameter(5));
            pushMetadata(deparser->getApplyParameters()->getParameter(6));
        }
        else if(kv.second.pipe == "EgressParser"){
            auto parser = kv.first->to<IR::P4Parser>();
            pushMetadata(parser->getApplyParameters()->getParameter(2));
            pushMetadata(parser->getApplyParameters()->getParameter(3));
            pushMetadata(parser->getApplyParameters()->getParameter(4));
            pushMetadata(parser->getApplyParameters()->getParameter(5));
            pushMetadata(parser->getApplyParameters()->getParameter(6));
        }
        else if(kv.second.pipe == "Egress"){
            auto control = kv.first->to<IR::P4Control>();
            pushMetadata(control->getApplyParameters()->getParameter(1));
            pushMetadata(control->getApplyParameters()->getParameter(2));
            pushMetadata(control->getApplyParameters()->getParameter(3));
        }
        else if(kv.second.pipe == "EgressDeparser"){
            auto deparser = kv.first->to<IR::P4Control>();
            pushMetadata(deparser->getApplyParameters()->getParameter(1));
            pushMetadata(deparser->getApplyParameters()->getParameter(2));
            pushMetadata(deparser->getApplyParameters()->getParameter(4));
            pushMetadata(deparser->getApplyParameters()->getParameter(5));
            pushMetadata(deparser->getApplyParameters()->getParameter(6));
        }

    }
    return true;
}



bool CollectMetadataHeaderInfo::preorder(const IR::Type_Struct *s){
    for(auto m: used_metadata){
        if(m->to<IR::Type_Name>()->path->name.name == s->name.name){
            for(auto field : s->fields){
                fields.push_back(new IR::StructField(IR::ID(TypeStruct2Name(s->name.name) + "_" + field->name), field->type));
            }
            return true;
        }        
    }
    return true;
 }

const IR::Node* ReplaceMetadataHeaderName::postorder(IR::P4Program* p){
    // std::cout << p << std::endl;
    return p;
}


const IR::Node *ReplaceMetadataHeaderName::preorder(IR::Member *m){
    if(auto p = m->expr->to<IR::PathExpression>()){   
        auto declaration = refMap->getDeclaration(p->path);
        if (auto decl = declaration->to<IR::Parameter>()) {
            if (auto type = decl->type->to<IR::Type_Name>()) {
                if (type->path->name == "psa_ingress_parser_input_metadata_t" ||
                    type->path->name == "psa_ingress_input_metadata_t" ||
                    type->path->name == "psa_ingress_output_metadata_t" ||
                    type->path->name == "psa_egress_parser_input_metadata_t" ||
                    type->path->name == "psa_egress_input_metadata_t" ||
                    type->path->name == "psa_egress_output_metadata_t" ||
                    type->path->name == "psa_egress_deparser_input_metadata_t") {
                    return new IR::Member(new IR::PathExpression(IR::ID("m")), IR::ID(TypeStruct2Name(type->path->name.name) + "_" + m->member.name));
                }
                else if(type->path->name == info->header_type) {
                    return new IR::Member(new IR::PathExpression(IR::ID("h")), IR::ID(m->member.name));
                }
                else if(type->path->name == info->local_metadata_type){
                    return new IR::Member(new IR::PathExpression(IR::ID("m")), IR::ID("local_metadata_" + m->member.name));
                }
            }
        }
    }    
    return m;
}

const IR::Node *ReplaceMetadataHeaderName::preorder(IR::Parameter *p){
    if(auto type = p->type->to<IR::Type_Name>()){
        if(type->path->name == info->header_type){
            return new IR::Parameter(IR::ID("h"), p->direction, p->type);
        }
        else if(type->path->name == info->local_metadata_type){
            return new IR::Parameter(IR::ID("m"), p->direction, p->type);
        }
    }
    return p;
}

const IR::Node *InjectJumboStruct::preorder(IR::Type_Struct* s){
    if(s->name == info->local_metadata_type){
        return new IR::Type_Struct(s->name, info->fields);
    }
    return s;
}

IR::IndexedVector<IR::Declaration> *
StatementUnroll::findDeclarationList(const IR::P4Control *control, const IR::P4Parser *parser){
    IR::IndexedVector<IR::Declaration> *decls;
    if(parser) {
        auto res = decl_map.find(parser);
        if(res != decl_map.end()){
            decls = res->second;
        }
        else{
            decls = new IR::IndexedVector<IR::Declaration>;
            decl_map.emplace(parser, decls);
        }
    }
    else if(control) {
        auto res = decl_map.find(control);
        if(res != decl_map.end()){
            decls = res->second;
        }
        else{
            decls = new IR::IndexedVector<IR::Declaration>;
            decl_map.emplace(control, decls);            
        }
    }
    return decls;
}


const IR::Node *StatementUnroll::preorder(IR::AssignmentStatement *a){
    auto code_block = new IR::IndexedVector<IR::StatOrDecl>;
    auto right = a->right;
    auto control = findOrigCtxt<IR::P4Control>();
    auto parser = findOrigCtxt<IR::P4Parser>();
    auto decls = findDeclarationList(control, parser);

    if(right->is<IR::MethodCallExpression>()) {
        prune();
        return a;
    }
    else if(auto bin = right->to<IR::Operation_Binary>()) {
        ExpressionUnroll::sanity(bin->right);
        ExpressionUnroll::sanity(bin->left);
        auto left_unroller = new ExpressionUnroll(collector);
        auto right_unroller = new ExpressionUnroll(collector);
        bin->left->apply(*left_unroller);
        const IR::Expression * left_tmp = left_unroller->root;
        bin->right->apply(*right_unroller);
        const IR::Expression * right_tmp = right_unroller->root;
        if(not left_tmp) left_tmp = bin->left;
        if(not right_tmp) right_tmp = bin->right;
        for(auto s: left_unroller->stmt)
            code_block->push_back(s);
        for(auto d: left_unroller->decl)
            decls->push_back(d);
        for(auto s: right_unroller->stmt)
            code_block->push_back(s);
        for(auto d: right_unroller->decl)
            decls->push_back(d);
        prune();
        if(right->is<IR::Add>()){
            a->right = new IR::Add(left_tmp, right_tmp);
        }
        else if(right->is<IR::Sub>()){
            a->right = new IR::Sub(left_tmp, right_tmp);
        }
        else if(right->is<IR::Shl>()){
            a->right = new IR::Shl(left_tmp, right_tmp);
        }
        else if(right->is<IR::Shr>()){
            a->right = new IR::Shr(left_tmp, right_tmp);
        }
        else if(right->is<IR::Equ>()){
            a->right = new IR::Equ(left_tmp, right_tmp);
        }
        else {
            std::cerr << right->node_type_name() << std::endl;
            BUG("not implemented.");
        }
        code_block->push_back(a);
        return new IR::BlockStatement(*code_block);
    }
    else if(isSimpleExpression(right)){
        prune();
    }
    else if(auto un = right->to<IR::Operation_Unary>()){
        auto code_block = new IR::IndexedVector<IR::StatOrDecl>;
        ExpressionUnroll::sanity(un->expr);
        auto unroller = new ExpressionUnroll(collector);
        un->expr->apply(*unroller);
        prune();
        const IR::Expression *un_tmp = unroller->root;
        for(auto s: unroller->stmt)
            code_block->push_back(s);
        for(auto d: unroller->decl)
            decls->push_back(d);
        if(not un_tmp) un_tmp = un->expr;
        if(auto n = right->to<IR::Neg>()){
            a->right = new IR::Neg(un_tmp);
        }
        else if(auto c = right->to<IR::Cmpl>()){
            a->right = new IR::Cmpl(un_tmp);
        }
        else if(auto ln = right->to<IR::LNot>()){
            a->right = new IR::LNot(un_tmp);
        }
        else if(auto c = right->to<IR::Cast>()){
            a->right = new IR::Cast(c->destType, un_tmp);
        }
        else{
            std::cerr << right->node_type_name() << std::endl;
            BUG("Not implemented.");
        }
        code_block->push_back(a);
        return new IR::BlockStatement(*code_block);
    }
    else{
        std::cerr << right->node_type_name() << std::endl;
        BUG("not implemented");
    }
    return a;
}

const IR::Node *StatementUnroll::postorder(IR::IfStatement *i){
    auto code_block = new IR::IndexedVector<IR::StatOrDecl>;
    ExpressionUnroll::sanity(i->condition);
    auto unroller = new ExpressionUnroll(collector);
    i->condition->apply(*unroller);
    for(auto i:unroller->stmt)
        code_block->push_back(i);

    auto control = findOrigCtxt<IR::P4Control>();
    auto parser = findOrigCtxt<IR::P4Parser>();
    auto decls = findDeclarationList(control, parser);

    for(auto d: unroller->decl)
        decls->push_back(d);
    if(unroller->root) {
        i->condition = unroller->root;
    }
    code_block->push_back(i);
    return new IR::BlockStatement(*code_block);
}

const IR::Node *StatementUnroll::preorder(IR::MethodCallStatement *m){
    // visit(m->methodCall);
    // prune();    
    return m;
}

const IR::Node *StatementUnroll::postorder(IR::P4Control *a){
    auto control = getOriginal();
    auto res = decl_map.find(control);
    if(res == decl_map.end()){
        return a;
    }
    for(auto d: *(res->second))
        a->controlLocals.push_back(d);
    return a;
}
const IR::Node *StatementUnroll::postorder(IR::P4Parser *a){
    auto parser = getOriginal();
    auto res = decl_map.find(parser);
    if(res == decl_map.end()){
        return a;
    }
    for(auto d: *(res->second))
        a->parserLocals.push_back(d);
    return a;
}


bool ExpressionUnroll::preorder(const IR::Operation_Unary *u){
    sanity(u->expr);
    visit(u->expr);
    const IR::Expression *un_expr;
    if(root){
        if(auto neg = u->to<IR::Neg>()){
            un_expr = new IR::Neg(root);
        }
        else if(auto neg = u->to<IR::Cmpl>()){
            un_expr = new IR::Cmpl(root);
        }
        else if(auto neg = u->to<IR::LNot>()){
            un_expr = new IR::LNot(root);
        }
        else{
            std::cout << u->node_type_name() << std::endl;
            BUG("Not Implemented");
        }
    }
    else{
        un_expr = u->expr;
    }
    root = new IR::PathExpression(IR::ID(collector->get_next_tmp()));
    stmt.push_back(new IR::AssignmentStatement(root, un_expr));

}

bool ExpressionUnroll::preorder(const IR::Operation_Binary *bin){
    sanity(bin->left);
    sanity(bin->right);
    visit(bin->left);
    const IR::Expression *left_root = root;
    visit(bin->right);
    const IR::Expression *right_root = root;
    if(not left_root) left_root = bin->left;
    if(not right_root) right_root = bin->right;
    
    root = new IR::PathExpression(IR::ID(collector->get_next_tmp()));
    const IR::Expression *bin_expr;
    decl.push_back(new IR::Declaration_Variable(root->path->name, bin->type));
    if(bin->is<IR::Add>()) {
        bin_expr = new IR::Add(left_root, right_root);
    }
    else if(bin->is<IR::Sub>()) {
        bin_expr = new IR::Sub(left_root, right_root);
    }
    else if(bin->is<IR::Shl>()) {
        bin_expr = new IR::Shl(left_root, right_root);
    }
    else if(bin->is<IR::Shr>()) {
        bin_expr = new IR::Shr(left_root, right_root);
    }
    else if(bin->is<IR::Equ>()) {
        bin_expr = new IR::Equ(left_root, right_root);
    }
    else if(bin->is<IR::LAnd>()) {
        bin_expr = new IR::LAnd(left_root, right_root);
    }
    else if(bin->is<IR::Leq>()) {
        bin_expr = new IR::Leq(left_root, right_root);
    }
    else if(bin->is<IR::Lss>()) {
        bin_expr = new IR::Lss(left_root, right_root);
    }
    else if(bin->is<IR::Geq>()) {
        bin_expr = new IR::Geq(left_root, right_root);
    }
    else if(bin->is<IR::Grt>()) {
        bin_expr = new IR::Grt(left_root, right_root);
    }
    else if(bin->is<IR::Neq>()) {
        bin_expr = new IR::Neq(left_root, right_root);
    }
    else{
        std::cerr << bin->node_type_name() << std::endl;
        BUG("not implemented");
    }
    stmt.push_back(new IR::AssignmentStatement(root, bin_expr));
    return false;
}

bool ExpressionUnroll::preorder(const IR::MethodCallExpression *m){
    auto args = new IR::Vector<IR::Argument>;
    for(auto arg: *m->arguments){
        sanity(arg->expression);
        visit(arg->expression);
        if(not root) args->push_back(arg);
        else args->push_back(new IR::Argument(root));
    }
    root = new IR::PathExpression(IR::ID(collector->get_next_tmp()));
    decl.push_back(new IR::Declaration_Variable(root->path->name, m->type));
    auto new_m = new IR::MethodCallExpression(m->method, args);
    stmt.push_back(new IR::AssignmentStatement(root, new_m));
    return false;
}

bool ExpressionUnroll::preorder(const IR::Member *member){
    root = nullptr;
    return false;
}

bool ExpressionUnroll::preorder(const IR::PathExpression *path){
    root = nullptr;
    return false;
}

bool ExpressionUnroll::preorder(const IR::Constant *c){
    root = nullptr;
    return false;
}
bool ExpressionUnroll::preorder(const IR::BoolLiteral *b){
    root = nullptr;
    return false;
}


}  // namespace DPDK
