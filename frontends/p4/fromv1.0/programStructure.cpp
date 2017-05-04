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

#include "programStructure.h"

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>

#include <set>
#include <algorithm>

#include "setup.h"
#include "lib/path.h"
#include "lib/gmputil.h"
#include "converters.h"

#include "frontends/common/options.h"
#include "frontends/parsers/parserDriver.h"
#include "frontends/p4/reservedWords.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/tableKeyNames.h"

namespace P4V1 {

ProgramStructure::ProgramStructure() :
        v1model(P4V1::V1Model::instance), p4lib(P4::P4CoreLibrary::instance),
        types(&allNames), metadata(&allNames), headers(&allNames), stacks(&allNames),
        controls(&allNames), parserStates(&allNames), tables(&allNames),
        actions(&allNames), counters(&allNames), registers(&allNames), meters(&allNames),
        action_profiles(nullptr), field_lists(nullptr), field_list_calculations(&allNames),
        action_selectors(nullptr), extern_types(&allNames), externs(&allNames),
        calledActions("actions"), calledControls("controls"), calledCounters("counters"),
        calledMeters("meters"), calledRegisters("registers"), calledExterns("externs"),
        parsers("parsers"), parserPacketIn(nullptr), parserHeadersOut(nullptr),
        verifyChecksums(nullptr), updateChecksums(nullptr),
        deparser(nullptr), latest(nullptr) {
    ingress = nullptr;
    declarations = new IR::IndexedVector<IR::Node>();
    emptyTypeArguments = new IR::Vector<IR::Type>();
    conversionContext.clear();
    for (auto c : P4::reservedWords)
        allNames.emplace(c);
}

const IR::Annotations*
ProgramStructure::addNameAnnotation(cstring name, const IR::Annotations* annos) {
    if (annos == nullptr)
        annos = IR::Annotations::empty;
    return annos->addAnnotationIfNew(IR::Annotation::nameAnnotation, new IR::StringLiteral(name));
}

cstring ProgramStructure::makeUniqueName(cstring base) {
    cstring name = cstring::make_unique(allNames, base, '_');
    allNames.emplace(name);
    return name;
}

bool ProgramStructure::isHeader(const IR::ConcreteHeaderRef* nhr) const {
    cstring name = nhr->ref->name.name;
    auto hdr = headers.get(name);
    if (hdr != nullptr)
        return true;
    auto stack = stacks.get(name);
    if (stack != nullptr)
        return true;
    auto meta = metadata.get(name);
    if (meta != nullptr)
        return false;
    BUG("Unexpected reference %1%", nhr);
}

void ProgramStructure::checkHeaderType(const IR::Type_StructLike* hdr, bool metadata) {
    LOG1("Checking " << hdr << " " << (metadata ? "M" : "H"));
    for (auto f : hdr->fields) {
        if (f->type->is<IR::Type_Varbits>()) {
            if (metadata)
                ::error("%1%: varbit types illegal in metadata", f);
        } else if (!f->type->is<IR::Type_Bits>()) {
            // These come from P4-14, so they cannot be anything else
            BUG("%1%: unexpected type", f); }
    }
}

void ProgramStructure::createType(const IR::Type_StructLike* type, bool header,
                                   std::unordered_set<const IR::Type*> *converted) {
    if (converted->count(type))
        return;
    converted->emplace(type);
    auto type_name = types.get(type);
    auto newType = type->apply(TypeConverter(this));
    if (newType->name.name != type_name) {
        auto annos = addNameAnnotation(type->name.name, type->annotations);
        if (header) {
            newType = new IR::Type_Header(newType->srcInfo, type_name, annos, newType->fields);
        } else {
            newType = new IR::Type_Struct(newType->srcInfo, type_name, annos, newType->fields);
        }
    }
    if (header)
        finalHeaderType.emplace(type->externalName(), newType);
    checkHeaderType(newType, !header);
    LOG3("Added type " << dbp(newType) << " named " << type_name << " from " << dbp(type));
    declarations->push_back(newType);
    converted->emplace(newType);
}

void ProgramStructure::createTypes() {
    std::unordered_set<const IR::Type *> converted;
    // Metadata first
    for (auto it : metadata) {
        auto type = it.first->type;
        if (type->name.name == v1model.standardMetadataType.name)
            continue;
        createType(type, false, &converted);
    }
    for (auto it : registers) {
        if (it.first->layout) {
            auto type = types.get(it.first->layout);
            createType(type, false, &converted);
        }
    }

    // Headers next
    converted.clear();
    for (auto it : headers) {
        auto type = it.first->type;
        createType(type, true, &converted);
    }
    for (auto it : stacks) {
        auto type = it.first->type;
        createType(type, true, &converted);
    }
}

const IR::Type_Struct* ProgramStructure::createFieldListType(const IR::Expression* expression) {
    if (!expression->is<IR::PathExpression>())
        ::error("%1%: expected a field list", expression);
    auto nr = expression->to<IR::PathExpression>();
    auto fl = field_lists.get(nr->path->name);
    if (fl == nullptr)
        ::error("%1%: Expected a field list", expression);

    auto name = makeUniqueName(nr->path->name);
    auto annos = addNameAnnotation(nr->path->name);
    auto result = new IR::Type_Struct(expression->srcInfo, name, annos);
    std::set<cstring> fieldNames;
    for (auto f : fl->fields) {
        cstring name;
        if (f->is<IR::PathExpression>())
            name = f->to<IR::PathExpression>()->path->name;
        else if (f->is<IR::Member>())
            name = f->to<IR::Member>()->member;
        name = cstring::make_unique(fieldNames, name, '_');
        fieldNames.emplace(name);
        auto type = f->type;
        CHECK_NULL(type);
        BUG_CHECK(!type->is<IR::Type_Unknown>(), "%1%: Unknown field list member type", f);
        auto sf = new IR::StructField(f->srcInfo, name, type);
        result->fields.push_back(sf);
    }

    return result;
}

void ProgramStructure::createStructures() {
    auto metadata = new IR::Type_Struct(v1model.metadataType.Id());
    for (auto it : this->metadata) {
        if (it.first->name == v1model.standardMetadata.name)
            continue;
        IR::ID id = it.first->name;
        auto type = it.first->type;
        auto type_name = types.get(type);
        auto ht = type->to<IR::Type_Struct>();
        auto path = new IR::Path(type_name);
        auto tn = new IR::Type_Name(ht->name.srcInfo, path);
        auto annos = addNameAnnotation(id, it.first->annotations);
        auto field = new IR::StructField(id.srcInfo, id, annos, tn);
        metadata->fields.push_back(field);
    }
    declarations->push_back(metadata);

    auto headers = new IR::Type_Struct(IR::ID(v1model.headersType.name));
    for (auto it : this->headers) {
        IR::ID id = it.first->name;
        auto type = it.first->type;
        auto type_name = types.get(type);
        auto ht = type->to<IR::Type_Header>();
        auto path = new IR::Path(type_name);
        auto tn = new IR::Type_Name(ht->name.srcInfo, path);
        auto annos = addNameAnnotation(id, it.first->annotations);
        auto field = new IR::StructField(id.srcInfo, id, annos, tn);
        headers->fields.push_back(field);
    }

    for (auto it : stacks) {
        IR::ID id = it.first->name;
        auto type = it.first->type;
        auto type_name = types.get(type);
        int size = it.first->size;
        auto ht = type->to<IR::Type_Header>();
        auto path = new IR::Path(type_name);
        auto tn = new IR::Type_Name(ht->name.srcInfo, path);
        auto stack = new IR::Type_Stack(id.srcInfo, tn, new IR::Constant(size));
        auto annos = addNameAnnotation(id, it.first->annotations);
        auto field = new IR::StructField(id.srcInfo, id, annos, stack);
        headers->fields.push_back(field);
    }
    declarations->push_back(headers);
}

class ProgramStructure::FixupExtern : public Modifier {
    ProgramStructure            &self;
    cstring                     origname, extname;
    IR::TypeParameters          *typeParams = nullptr;

    bool preorder(IR::Type_Extern *type) override {
        BUG_CHECK(!origname, "Nested extern");
        origname = type->name;
        return true; }
    void postorder(IR::Type_Extern *type) override {
        if (extname != type->name) {
            type->annotations = self.addNameAnnotation(type->name.name, type->annotations);
            type->name = extname; }
        // FIXME -- should create ctors based on attributes?  For now just create a
        // FIXME -- 0-arg one if needed
        if (!type->lookupMethod(type->name, 0)) {
            type->methods.push_back(new IR::Method(type->name, new IR::Type_Method(
                                                new IR::ParameterList()))); } }
    void postorder(IR::Method *meth) override {
        if (meth->name == origname) meth->name = extname; }
    // Convert extern methods that take a field_list_calculation to take a type param instead
    bool preorder(IR::Type_MethodBase *mtype) override {
        BUG_CHECK(!typeParams, "recursion failure");
        typeParams = mtype->typeParameters->clone();
        return true; }
    bool preorder(IR::Parameter *param) override {
        BUG_CHECK(typeParams, "recursion failure");
        if (param->type->is<IR::Type_FieldListCalculation>()) {
            auto n = new IR::Type_Var(self.makeUniqueName("FL"));
            param->type = n;
            typeParams->push_back(n); }
        return false; }
    void postorder(IR::Type_MethodBase *mtype) override {
        BUG_CHECK(typeParams, "recursion failure");
        if (*typeParams != *mtype->typeParameters)
            mtype->typeParameters = typeParams;
        typeParams = nullptr; }

 public:
    FixupExtern(ProgramStructure &self, cstring n) : self(self), extname(n) {}
};

void ProgramStructure::createExterns() {
    for (auto it : extern_types)
        declarations->push_back(it.first->apply(FixupExtern(*this, it.second)));
}

const IR::Expression* ProgramStructure::paramReference(const IR::Parameter* param) {
    auto result = new IR::PathExpression(param->name);
    auto tn = param->type->to<IR::Type_Name>();
    cstring name = tn->path->toString();
    auto type = types.get(name);
    if (type != nullptr)
        result->type = type;
    else
        result->type = param->type;
    return result;
}

const IR::AssignmentStatement* ProgramStructure::assign(
    Util::SourceInfo srcInfo, const IR::Expression* left,
    const IR::Expression* right, const IR::Type* type) {
    const IR::Expression* cast;
    if (type != nullptr)
        cast = new IR::Cast(type, right);
    else
        cast = right;
    auto result = new IR::AssignmentStatement(srcInfo, left, cast);
    return result;
}

const IR::Statement* ProgramStructure::convertParserStatement(const IR::Expression* expr) {
    ExpressionConverter conv(this);

    if (expr->is<IR::Primitive>()) {
        auto primitive = expr->to<IR::Primitive>();
        if (primitive->name == "extract") {
            BUG_CHECK(primitive->operands.size() == 1, "Expected 1 operand for %1%", primitive);
            auto dest = primitive->operands.at(0);
            auto destType = dest->type;
            CHECK_NULL(destType);
            BUG_CHECK(destType->is<IR::Type_Header>(), "%1%: expected a header", destType);
            auto finalDestType = ::get(
                finalHeaderType, destType->to<IR::Type_Header>()->externalName());
            BUG_CHECK(finalDestType != nullptr, "%1%: could not find final type",
                      destType->to<IR::Type_Header>()->externalName());
            destType = finalDestType;
            BUG_CHECK(destType->is<IR::Type_Header>(), "%1%: expected a header", destType);

            auto current = conv.convert(dest);
            auto args = new IR::Vector<IR::Expression>();
            args->push_back(current);

            // A second conversion of dest is used to compute the
            // 'latest' (P4-14 keyword) value if referenced later.
            conv.replaceNextWithLast = true;
            this->latest = conv.convert(dest);
            conv.replaceNextWithLast = false;
            const IR::Expression* method = new IR::Member(
                paramReference(parserPacketIn),
                p4lib.packetIn.extract.Id());
            auto mce = new IR::MethodCallExpression(expr->srcInfo, method, args);

            LOG3("Inserted extract " << dbp(mce) << " " << dbp(destType));
            extractsSynthesized.emplace(mce, destType->to<IR::Type_Header>());
            auto result = new IR::MethodCallStatement(expr->srcInfo, mce);
            return result;
        } else if (primitive->name == "set_metadata") {
            BUG_CHECK(primitive->operands.size() == 2, "Expected 2 operands for %1%", primitive);

            auto dest = conv.convert(primitive->operands.at(0));
            auto src = conv.convert(primitive->operands.at(1));
            return assign(primitive->srcInfo, dest, src, primitive->operands.at(0)->type);
        }
    }
    BUG("Unhandled expression %1%", expr);
}

const IR::PathExpression* ProgramStructure::getState(IR::ID dest) {
    auto ps = parserStates.get(dest.name);
    if (ps != nullptr) {
        return new IR::PathExpression(dest);
    } else {
        auto ctrl = controls.get(dest);
        if (ctrl != nullptr) {
            if (ingress != nullptr) {
                    if (ingress != ctrl)
                        ::error("Parser exits to two different control blocks: %1% and %2%",
                                dest, ingressReference);
            } else {
                ingress = ctrl;
                ingressReference = dest;
            }
            return new IR::PathExpression(IR::ParserState::accept);
        }
    }
    BUG("Unexpected destination %1%", dest);
}

static const IR::Expression*
explodeLabel(const IR::Constant* value, const IR::Constant* mask,
             const std::vector<int> &sizes) {
    if (mask->value == 0)
        return new IR::DefaultExpression(value->srcInfo);
    bool useMask = mask->value != -1;

    mpz_class v = value->value;
    mpz_class m = mask->value;

    auto rv = new IR::ListExpression(value->srcInfo, {});
    for (auto it = sizes.rbegin(); it != sizes.rend(); ++it) {
        int s = *it;
        auto bits = Util::ripBits(v, s);
        auto type = IR::Type_Bits::get(s);
        const IR::Expression* expr = new IR::Constant(value->srcInfo, type, bits, value->base);
        if (useMask) {
            auto maskbits = Util::ripBits(m, s);
            auto maskcst = new IR::Constant(mask->srcInfo, type, maskbits, mask->base);
            expr = new IR::Mask(mask->srcInfo, expr, maskcst);
        }
        rv->components.insert(rv->components.begin(), expr);
    }
    if (rv->components.size() == 1)
        return rv->components.at(0);
    return rv;
}

const IR::ParserState* ProgramStructure::convertParser(const IR::V1Parser* parser) {
    ExpressionConverter conv(this);

    latest = nullptr;
    IR::IndexedVector<IR::StatOrDecl> components;
    for (auto e : parser->stmts) {
        auto stmt = convertParserStatement(e);
        components.push_back(stmt);
    }
    const IR::Expression* select = nullptr;
    if (parser->select != nullptr) {
        auto list = new IR::ListExpression(parser->select->srcInfo, {});
        std::vector<int> sizes;
        for (auto e : *parser->select) {
            auto c = conv.convert(e);
            list->components.push_back(c);
            int w = c->type->width_bits();
            BUG_CHECK(w > 0, "Unknown width for expression %1%", e);
            sizes.push_back(w);
        }
        BUG_CHECK(list->components.size() > 0, "No select expression in %1%", parser);
        // select always expects a ListExpression
        IR::Vector<IR::SelectCase> cases;
        for (auto c : *parser->cases) {
            IR::ID state = c->action;
            auto deststate = getState(state);
            for (auto v : c->values) {
                auto expr = explodeLabel(v.first, v.second, sizes);
                auto sc = new IR::SelectCase(c->srcInfo, expr, deststate);
                cases.push_back(sc);
            }
        }
        select = new IR::SelectExpression(parser->select->srcInfo, list, std::move(cases));
    } else if (!parser->default_return.name.isNullOrEmpty()) {
        IR::ID id = parser->default_return;
        select = getState(id);
    } else {
        BUG("No select or default_return %1%", parser);
    }
    latest = nullptr;
    auto annos = addNameAnnotation(parser->name, parser->annotations);
    auto result = new IR::ParserState(parser->srcInfo, parser->name, annos, components, select);
    return result;
}

void ProgramStructure::createParser() {
    auto paramList = new IR::ParameterList;
    auto pinpath = new IR::Path(p4lib.packetIn.Id());
    auto pintype = new IR::Type_Name(pinpath);
    parserPacketIn = new IR::Parameter(v1model.parser.packetParam.Id(),
                                       IR::Direction::None, pintype);
    paramList->push_back(parserPacketIn);

    auto headpath = new IR::Path(v1model.headersType.Id());
    auto headtype = new IR::Type_Name(headpath);
    parserHeadersOut = new IR::Parameter(v1model.parser.headersParam.Id(),
                                         IR::Direction::Out, headtype);
    paramList->push_back(parserHeadersOut);
    conversionContext.header = paramReference(parserHeadersOut);

    auto metapath = new IR::Path(v1model.metadataType.Id());
    auto metatype = new IR::Type_Name(metapath);
    auto meta = new IR::Parameter(v1model.parser.metadataParam.Id(),
                                  IR::Direction::InOut, metatype);
    paramList->push_back(meta);
    conversionContext.userMetadata = paramReference(meta);

    auto stdMetaPath = new IR::Path(v1model.standardMetadataType.Id());
    auto stdMetaType = new IR::Type_Name(stdMetaPath);
    auto stdmeta = new IR::Parameter(v1model.ingress.standardMetadataParam.Id(),
                                     IR::Direction::InOut, stdMetaType);
    paramList->push_back(stdmeta);
    conversionContext.standardMetadata = paramReference(stdmeta);

    auto type = new IR::Type_Parser(v1model.parser.Id(), new IR::TypeParameters(), paramList);
    IR::IndexedVector<IR::Declaration> stateful;
    IR::IndexedVector<IR::ParserState> states;
    for (auto p : parserStates) {
        auto ps = convertParser(p.first);
        states.push_back(ps);
    }

    if (states.empty())
        ::error("No parsers specified");
    auto result = new IR::P4Parser(v1model.parser.Id(), type, stateful, states);
    declarations->push_back(result);
    conversionContext.clear();
}

void ProgramStructure::include(cstring filename) {
    Util::PathName path(p4includePath);
    path = path.join(filename);

    CompilerOptions options;
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.file = path.toString();
    if (FILE* file = options.preprocess()) {
        if (!::errorCount()) {
            auto code = P4::P4ParserDriver::parse(options.file, file);
            if (code && !::errorCount())
                for (auto decl : code->declarations)
                    declarations->push_back(decl); }
        options.closeInput(file); }
}

void ProgramStructure::loadModel() {
    // This includes in turn stdlib.p4
    include("v1model.p4");
}

namespace {
// Must return a 'canonical' representation of each header.
// If a header appears twice this must return the SAME expression.
class HeaderRepresentation {
 private:
    const IR::Expression* hdrsParam;  // reference to headers parameter in deparser
    std::map<cstring, const IR::Expression*> header;
    std::map<cstring, const IR::Expression*> fakeHeader;
    std::map<const IR::Expression*, std::map<int, const IR::Expression*>> stackElement;

 public:
    explicit HeaderRepresentation(const IR::Expression* hdrsParam) :
            hdrsParam(hdrsParam) { CHECK_NULL(hdrsParam); }

    const IR::Expression* getHeader(const IR::Expression* expression) {
        CHECK_NULL(expression);
        // expression is a reference to a header in an 'extract' statement
        if (expression->is<IR::ConcreteHeaderRef>()) {
            cstring name = expression->to<IR::ConcreteHeaderRef>()->ref->name;
            if (header.find(name) == header.end())
                header[name] = new IR::Member(hdrsParam, name);
            return header[name];
        } else if (expression->is<IR::HeaderStackItemRef>()) {
            auto hsir = expression->to<IR::HeaderStackItemRef>();
            auto hdr = getHeader(hsir->base_);
            if (hsir->index_->is<IR::PathExpression>())
                // This is most certainly 'next'.
                return hdr;
            BUG_CHECK(hsir->index_->is<IR::Constant>(), "%1%: expected a constant", hsir->index_);
            int index = hsir->index_->to<IR::Constant>()->asInt();
            if (stackElement.find(hdr) != stackElement.end()) {
                if (stackElement[hdr].find(index) != stackElement[hdr].end())
                    return stackElement[hdr][index];
            }
            auto ai = new IR::ArrayIndex(hdr, hsir->index_);
            stackElement[hdr][index] = ai;
            return ai;
        }
        BUG("%1%: Unexpected expression in 'extract'", expression);
    }
    const IR::Expression* getFakeHeader(cstring state) {
        if (fakeHeader.find(state) == fakeHeader.end())
            fakeHeader[state] = new IR::StringLiteral(state);
        return fakeHeader[state];
    }
};
}  // namespace

void ProgramStructure::createDeparser() {
    auto headpath = new IR::Path(v1model.headersType.Id());
    auto headtype = new IR::Type_Name(headpath);
    auto headers = new IR::Parameter(v1model.deparser.headersParam.Id(),
                                     IR::Direction::In, headtype);
    auto hdrsParam = paramReference(headers);
    HeaderRepresentation hr(hdrsParam);

    P4::CallGraph<const IR::Expression*> headerOrder("headerOrder");
    for (auto it : parsers) {
        cstring caller = it.first;

        const IR::Expression* lastExtract = nullptr;
        if (extracts.count(caller) == 0) {
            lastExtract = hr.getFakeHeader(caller);
        } else {
            for (auto e : extracts[caller]) {
                auto h = hr.getHeader(e);
                if (lastExtract != nullptr)
                    headerOrder.calls(lastExtract, h);
                lastExtract = h;
            }
        }
        for (auto callee : *it.second) {
            const IR::Expression* firstExtract;
            if (extracts.count(callee) == 0) {
                firstExtract = hr.getFakeHeader(callee);
            } else {
                auto e = extracts[callee].at(0);
                auto h = hr.getHeader(e);
                firstExtract = h;
            }
            headerOrder.calls(lastExtract, firstExtract);
        }
    }

    cstring start = IR::ParserState::start;
    const IR::Expression* startHeader;
    // find starting point
    if (extracts.count(start) != 0) {
        auto e = extracts[start].at(0);
        startHeader = hr.getHeader(e);
    } else {
        startHeader = hr.getFakeHeader(start);
    }

    std::vector<const IR::Expression*> sortedHeaders;
    bool loop = headerOrder.sccSort(startHeader, sortedHeaders);
    if (loop)
        ::warning("The order of headers in deparser is not uniquely determined by parser!");

    auto params = new IR::ParameterList;
    auto poutpath = new IR::Path(p4lib.packetOut.Id());
    auto pouttype = new IR::Type_Name(poutpath);
    auto packetOut = new IR::Parameter(v1model.deparser.packetParam.Id(),
                                       IR::Direction::None, pouttype);
    params->push_back(packetOut);
    params->push_back(headers);
    conversionContext.header = paramReference(headers);

    auto type = new IR::Type_Control(v1model.deparser.Id(), params);

    ExpressionConverter conv(this);

    auto body = new IR::BlockStatement;
    for (auto it = sortedHeaders.rbegin(); it != sortedHeaders.rend(); it++) {
        auto exp = *it;
        if (exp->is<IR::StringLiteral>())
            // a "fake" header
            continue;
        auto args = new IR::Vector<IR::Expression>();
        args->push_back(exp);
        const IR::Expression* method = new IR::Member(
            paramReference(packetOut),
            p4lib.packetOut.emit.Id());
        auto mce = new IR::MethodCallExpression(method, args);
        auto stat = new IR::MethodCallStatement(mce);
        body->push_back(stat);
    }
    deparser = new IR::P4Control(v1model.deparser.Id(), type, body);
    declarations->push_back(deparser);
}

const IR::P4Table*
ProgramStructure::convertTable(const IR::V1Table* table, cstring newName,
                               IR::IndexedVector<IR::Declaration> &stateful,
                               std::map<cstring, cstring> &mapNames) {
    ExpressionConverter conv(this);
    auto props = new IR::TableProperties(table->properties.properties);
    auto actionList = new IR::ActionList({});

    cstring profile = table->action_profile.name;
    const IR::ActionProfile* action_profile = nullptr;
    const IR::ActionSelector* action_selector = nullptr;
    if (!profile.isNullOrEmpty()) {
        action_profile = action_profiles.get(profile);
        if (!action_profile->selector.name.isNullOrEmpty()) {
            action_selector = action_selectors.get(action_profile->selector.name);
            if (action_selector == nullptr)
                ::error("Cannot locate action selector %1%", action_profile->selector);
        }
    }

    auto mtr = get(directMeters, table->name);
    auto ctr = get(directCounters, table->name);
    const std::vector<IR::ID> &actionsToDo =
            action_profile ? action_profile->actions : table->actions;
    for (auto a : actionsToDo) {
        auto action = actions.get(a);
        cstring newname;
        if (mtr != nullptr || ctr != nullptr) {
            // we must synthesize a new action, which has a writeback to
            // the meter/counter
            newname = makeUniqueName(a);
            auto actCont = convertAction(action, newname, mtr, ctr);
            stateful.push_back(actCont);
            mapNames[table->name + '.' + a] = newname;
        } else {
            newname = actions.get(action);
        }
        auto ale = new IR::ActionListElement(a.srcInfo, new IR::PathExpression(newname));
        actionList->push_back(ale);
    }
    if (!table->default_action) {
        actionList->push_back(
            new IR::ActionListElement(
                new IR::Annotations({new IR::Annotation("default_only", {})}),
                new IR::PathExpression(p4lib.noAction.Id())));
    } else if (!actionList->getDeclaration(table->default_action)) {
        actionList->push_back(
            new IR::ActionListElement(
                new IR::Annotations({new IR::Annotation("default_only", {})}),
                new IR::PathExpression(table->default_action))); }
    props->push_back(new IR::Property(IR::ID(IR::TableProperties::actionsPropertyName),
                                      actionList, false));

    if (table->reads != nullptr) {
        auto key = new IR::Key({});
        for (size_t i = 0; i < table->reads->size(); i++) {
            auto e = table->reads->at(i);
            auto rt = table->reads_types.at(i);
            if (rt.name == "valid")
                rt.name = p4lib.exactMatch.Id();
            auto ce = conv.convert(e);

            // If the key has a P4-14 mask, we add here a @name annotation.
            // A mask generates a BAnd; a BAnd can only come from a mask.
            const IR::Annotations* annos = IR::Annotations::empty;
            if (ce->is<IR::BAnd>()) {
                auto mask = ce->to<IR::BAnd>();
                auto expr = mask->left;
                if (mask->left->is<IR::Constant>())
                    expr = mask->right;

                P4::KeyNameGenerator kng(nullptr);
                expr->apply(kng);
                cstring anno = kng.getName(expr);
                annos = annos->addAnnotation(IR::Annotation::nameAnnotation,
                                             new IR::StringLiteral(key->srcInfo, anno));
            }
            // Here we rely on the fact that the spelling of 'rt' is
            // the same in P4-14 and core.p4/v1model.p4.
            auto keyComp = new IR::KeyElement(annos, ce, new IR::PathExpression(rt));
            key->push_back(keyComp);
        }

        if (action_selector != nullptr) {
            auto flc = field_list_calculations.get(action_selector->key.name);
            if (flc == nullptr) {
                ::error("Cannot locate field list %1%", action_selector->key);
            } else {
                auto fl = getFieldLists(flc);
                if (fl != nullptr) {
                    for (auto f : fl->fields) {
                        auto ce = conv.convert(f);
                        auto keyComp = new IR::KeyElement(ce,
                            new IR::PathExpression(v1model.selectorMatchType.Id()));
                        key->push_back(keyComp);
                    }
                }
            }
        }

        props->push_back(new IR::Property(IR::ID(IR::TableProperties::keyPropertyName),
                                          key, false));
    }

    if (table->size != 0) {
        auto prop = new IR::Property(IR::ID("size"),
            new IR::ExpressionValue(new IR::Constant(table->size)), false);
        props->push_back(prop);
    }
    if (table->min_size != 0) {
        auto prop = new IR::Property(IR::ID("min_size"),
            new IR::ExpressionValue(new IR::Constant(table->min_size)), false);
        props->push_back(prop);
    }
    if (table->max_size != 0) {
        auto prop = new IR::Property(IR::ID("max_size"),
            new IR::ExpressionValue(new IR::Constant(table->max_size)), false);
        props->push_back(prop);
    }

    {
        const bool hasExplicitDefaultAction = !table->default_action.name.isNullOrEmpty();
        auto act = new IR::PathExpression(hasExplicitDefaultAction ? table->default_action
                                                                   : p4lib.noAction.Id());
        auto args = table->default_action_args != nullptr ?
                table->default_action_args : new IR::Vector<IR::Expression>();
        auto methodCall = new IR::MethodCallExpression(act, args);
        auto prop = new IR::Property(
            IR::ID(IR::TableProperties::defaultActionPropertyName),
            new IR::ExpressionValue(methodCall),
            /* isConstant = */ hasExplicitDefaultAction);
        props->push_back(prop);
    }

    if (action_selector != nullptr) {
        auto type = new IR::Type_Name(new IR::Path(v1model.action_selector.Id()));
        auto args = new IR::Vector<IR::Expression>();
        auto flc = field_list_calculations.get(action_selector->key.name);
        auto algorithm = convertHashAlgorithm(flc->algorithm);
        if (algorithm == nullptr)
            return nullptr;
        args->push_back(algorithm);
        auto size = new IR::Constant(v1model.action_selector.sizeType, action_profile->size);
        args->push_back(size);
        auto width = new IR::Constant(v1model.action_selector.widthType, flc->output_width);
        args->push_back(width);
        auto constructor = new IR::ConstructorCallExpression(type, args);
        auto propvalue = new IR::ExpressionValue(constructor);
        auto annos = addNameAnnotation(action_profile->name);
        if (action_selector->mode)
            annos = annos->addAnnotation("mode", new IR::StringLiteral(action_selector->mode));
        if (action_selector->type)
            annos = annos->addAnnotation("type", new IR::StringLiteral(action_selector->type));
        auto prop = new IR::Property(
            IR::ID(v1model.tableAttributes.tableImplementation.Id()),
            annos, propvalue, false);
        props->push_back(prop);
    } else if (action_profile != nullptr) {
        auto size = new IR::Constant(v1model.action_profile.sizeType, action_profile->size);
        auto type = new IR::Type_Name(new IR::Path(v1model.action_profile.Id()));
        auto args = new IR::Vector<IR::Expression>();
        args->push_back(size);
        auto constructor = new IR::ConstructorCallExpression(type, args);
        auto propvalue = new IR::ExpressionValue(constructor);
        auto annos = addNameAnnotation(action_profile->name);
        auto prop = new IR::Property(
            IR::ID(v1model.tableAttributes.tableImplementation.Id()),
            annos, propvalue, false);
        props->push_back(prop);
    }

    if (!ctr.isNullOrEmpty()) {
        auto counter = counters.get(ctr);
        auto kindarg = counterType(counter);
        auto type = new IR::Type_Name(new IR::Path(v1model.directCounter.Id()));
        auto args = new IR::Vector<IR::Expression>();
        args->push_back(kindarg);
        auto constructor = new IR::ConstructorCallExpression(type, args);
        auto propvalue = new IR::ExpressionValue(constructor);
        auto annos = addNameAnnotation(ctr);
        auto prop = new IR::Property(
            IR::ID(v1model.tableAttributes.directCounter.Id()),
            annos, propvalue, false);
        props->push_back(prop);
    }

    if (mtr != nullptr) {
        auto meter = new IR::PathExpression(mtr->name);
        auto propvalue = new IR::ExpressionValue(meter);
        auto prop = new IR::Property(
            IR::ID(v1model.tableAttributes.directMeter.Id()), propvalue, false);
        props->push_back(prop);
    }

    auto annos = addNameAnnotation(table->name, table->annotations);
    auto result = new IR::P4Table(table->srcInfo, newName, annos, props);
    return result;
}

const IR::Expression* ProgramStructure::convertHashAlgorithm(IR::ID algorithm) {
    IR::ID result;
    if (algorithm == "crc32") {
        result = v1model.algorithm.crc32.Id();
    } else if (algorithm == "crc32_custom") {
        result = v1model.algorithm.crc32_custom.Id();
    } else if (algorithm == "crc16") {
        result = v1model.algorithm.crc16.Id();
    } else if (algorithm == "crc16_custom") {
        result = v1model.algorithm.crc16_custom.Id();
    } else if (algorithm == "random") {
        result = v1model.algorithm.random.Id();
    } else if (algorithm == "identity") {
        result = v1model.algorithm.identity.Id();
    } else {
        ::warning("%1%: unexpected algorithm", algorithm);
        result = algorithm;
    }
    auto pe = new IR::TypeNameExpression(v1model.algorithm.Id());
    auto mem = new IR::Member(pe, result);
    return mem;
}

static bool sameBitsType(const IR::Type* left, const IR::Type* right) {
    if (typeid(*left) != typeid(*right))
        return false;
    if (left->is<IR::Type_Bits>())
        return left->to<IR::Type_Bits>()->operator==(* right->to<IR::Type_Bits>());
    BUG("%1%: Expected a bit/int type", right);
}

// Implement modify_field(left, right, mask)
const IR::Statement* ProgramStructure::sliceAssign(
    Util::SourceInfo srcInfo, const IR::Expression* left,
    const IR::Expression* right, const IR::Expression* mask) {
    if (mask->is<IR::Constant>()) {
        auto cst = mask->to<IR::Constant>();
        if (cst->value < 0) {
            ::error("%1%: Negative mask not supported", mask);
            return nullptr;
        }
        auto range = Util::findOnes(cst->value);
        if (cst->value == range.value) {
            auto h = new IR::Constant(range.highIndex);
            auto l = new IR::Constant(range.lowIndex);
            left = new IR::Slice(left->srcInfo, left, h, l);
            right = new IR::Slice(right->srcInfo, right, h, l);
            return assign(srcInfo, left, right, nullptr);
        }
        // else value is too complex for a slice
    }

    auto type = left->type;
    if (!sameBitsType(mask->type, left->type))
        mask = new IR::Cast(type, mask);
    if (!sameBitsType(right->type, left->type))
        right = new IR::Cast(type, right);
    auto op1 = new IR::BAnd(left->srcInfo, left, new IR::Cmpl(mask));
    auto op2 = new IR::BAnd(right->srcInfo, right, mask);
    auto result = new IR::BOr(mask->srcInfo, op1, op2);
    return assign(srcInfo, left, result, type);
}

#define OPS_CK(primitive, n) BUG_CHECK((primitive)->operands.size() == n, \
                                       "Expected " #n " operands for %1%", primitive)

const IR::Expression* ProgramStructure::convertFieldList(const IR::Expression* expression) {
    ExpressionConverter conv(this);

    if (!expression->is<IR::PathExpression>())
        ::error("%1%: expected a field list", expression);
    auto nr = expression->to<IR::PathExpression>();
    auto fl = field_lists.get(nr->path->name);
    if (fl == nullptr)
        ::error("%1%: Expected a field list", expression);
    auto result = conv.convert(fl);
    return result;
}


const IR::Statement* ProgramStructure::convertPrimitive(const IR::Primitive* primitive) {
    ExpressionConverter conv(this);
    const IR::GlobalRef *glob = nullptr;
    const IR::Declaration_Instance *extrn = nullptr;
    if (primitive->operands.size() >= 1)
        glob = primitive->operands[0]->to<IR::GlobalRef>();
    if (glob) extrn = glob->obj->to<IR::Declaration_Instance>();

    if (extrn) {
        auto extref = new IR::PathExpression(externs.get(extrn));
        auto method = new IR::Member(primitive->srcInfo, extref, primitive->name);
        auto args = new IR::Vector<IR::Expression>();
        for (unsigned i = 1; i < primitive->operands.size(); ++i)
            args->push_back(conv.convert(primitive->operands.at(i)));
        auto mc = new IR::MethodCallExpression(primitive->srcInfo, method, args);
        return new IR::MethodCallStatement(primitive->srcInfo, mc);
    } else if (primitive->name == "modify_field") {
        if (primitive->operands.size() == 2) {
            auto left = conv.convert(primitive->operands.at(0));
            auto right = conv.convert(primitive->operands.at(1));
            return assign(primitive->srcInfo, left, right, primitive->operands.at(0)->type);
        } else if (primitive->operands.size() == 3) {
            auto left = conv.convert(primitive->operands.at(0));
            auto right = conv.convert(primitive->operands.at(1));
            auto mask = conv.convert(primitive->operands.at(2));
            auto result = sliceAssign(primitive->srcInfo, left, right, mask);
            return result;
        }
    } else if (primitive->name == "bit_xor" || primitive->name == "add" ||
               primitive->name == "bit_nor" || primitive->name == "subtract" ||
               primitive->name == "bit_nand" || primitive->name == "shift_left" ||
               primitive->name == "shift_right" || primitive->name == "bit_and" ||
               primitive->name == "bit_orca" || primitive->name == "bit_orcb" ||
               primitive->name == "bit_andca" || primitive->name == "bit_andcb" ||
               primitive->name == "bit_xnor" ||
               primitive->name == "bit_or" || primitive->name == "min" ||
               primitive->name == "max") {
        OPS_CK(primitive, 3);
        auto dest = conv.convert(primitive->operands.at(0));
        auto left = conv.convert(primitive->operands.at(1));
        auto right = conv.convert(primitive->operands.at(2));
        IR::Expression* op = nullptr;

        if (!sameBitsType(left->type, right->type) && !primitive->name.startsWith("shift")) {
            auto r = new IR::Cast(right->srcInfo, left->type, right);
            r->type = left->type;
            right = r;
        }
        if (primitive->name == "bit_xor")
            op = new IR::BXor(dest->srcInfo, left, right);
        else if (primitive->name == "min")
            op = new IR::Mux(dest->srcInfo, new IR::Leq(dest->srcInfo, left, right),
                             left, right);
        else if (primitive->name == "max")
            op = new IR::Mux(dest->srcInfo, new IR::Geq(dest->srcInfo, left, right),
                             left, right);
        else if (primitive->name == "bit_or")
            op = new IR::BOr(dest->srcInfo, left, right);
        else if (primitive->name == "bit_xnor")
            op = new IR::Cmpl(dest->srcInfo, new IR::BXor(dest->srcInfo, left, right));
        else if (primitive->name == "add")
            op = new IR::Add(dest->srcInfo, left, right);
        else if (primitive->name == "bit_nor")
            op = new IR::Cmpl(dest->srcInfo, new IR::BOr(dest->srcInfo, left, right));
        else if (primitive->name == "bit_nand")
            op = new IR::Cmpl(dest->srcInfo, new IR::BAnd(dest->srcInfo, left, right));
        else if (primitive->name == "bit_orca")
            op = new IR::BOr(dest->srcInfo, new IR::Cmpl(dest->srcInfo, left), right);
        else if (primitive->name == "bit_orcb")
            op = new IR::BOr(dest->srcInfo, left, new IR::Cmpl(dest->srcInfo, right));
        else if (primitive->name == "bit_andca")
            op = new IR::BAnd(dest->srcInfo, new IR::Cmpl(dest->srcInfo, left), right);
        else if (primitive->name == "bit_andcb")
            op = new IR::BAnd(dest->srcInfo, left, new IR::Cmpl(dest->srcInfo, right));
        else if (primitive->name == "bit_and")
            op = new IR::BAnd(dest->srcInfo, left, right);
        else if (primitive->name == "subtract")
            op = new IR::Sub(dest->srcInfo, left, right);
        else if (primitive->name == "shift_left")
            op = new IR::Shl(dest->srcInfo, left, right);
        else if (primitive->name == "shift_right")
            op = new IR::Shr(dest->srcInfo, left, right);
        return assign(primitive->srcInfo, dest, op, primitive->operands.at(0)->type);
    } else if (primitive->name == "bit_not") {
        OPS_CK(primitive, 2);
        auto dest = conv.convert(primitive->operands.at(0));
        auto right = conv.convert(primitive->operands.at(1));
        IR::Expression* op = nullptr;
        if (primitive->name == "bit_not")
            op = new IR::Cmpl(right);
        return assign(primitive->srcInfo, dest, op, primitive->operands.at(0)->type);
    } else if (primitive->name == "no_op") {
        return new IR::EmptyStatement();
    } else if (primitive->name == "add_to_field" || primitive->name == "subtract_from_field") {
        OPS_CK(primitive, 2);
        auto left = conv.convert(primitive->operands.at(0));
        auto left2 = conv.convert(primitive->operands.at(0));
        // convert twice, so we have different expression trees on RHS and LHS
        auto right = conv.convert(primitive->operands.at(1));
        const IR::Expression* op;
        if (primitive->name == "add_to_field")
            op = new IR::Add(primitive->srcInfo, left, right);
        else
            op = new IR::Sub(primitive->srcInfo, left, right);
        return assign(primitive->srcInfo, left2, op, primitive->operands.at(0)->type);
    } else if (primitive->name == "remove_header") {
        OPS_CK(primitive, 1);
        auto hdr = conv.convert(primitive->operands.at(0));
        auto method = new IR::Member(hdr, IR::ID(IR::Type_Header::setInvalid));
        auto args = new IR::Vector<IR::Expression>();
        auto mc = new IR::MethodCallExpression(primitive->srcInfo, method, args);
        return new IR::MethodCallStatement(mc->srcInfo, mc);
    } else if (primitive->name == "add_header") {
        OPS_CK(primitive, 1);
        auto hdr = conv.convert(primitive->operands.at(0));
        auto method = new IR::Member(hdr, IR::ID(IR::Type_Header::setValid));
        auto args = new IR::Vector<IR::Expression>();
        auto mc = new IR::MethodCallExpression(primitive->srcInfo, method, args);
        return new IR::MethodCallStatement(mc->srcInfo, mc);
    } else if (primitive->name == "copy_header") {
        OPS_CK(primitive, 2);
        auto left = conv.convert(primitive->operands.at(0));
        auto right = conv.convert(primitive->operands.at(1));
        return new IR::AssignmentStatement(primitive->srcInfo, left, right);
    } else if (primitive->name == "drop" || primitive->name == "mark_for_drop") {
        auto method = new IR::PathExpression(v1model.drop.Id());
        auto mc = new IR::MethodCallExpression(primitive->srcInfo, method);
        return new IR::MethodCallStatement(mc->srcInfo, mc);
    } else if (primitive->name == "push" || primitive->name == "pop") {
        OPS_CK(primitive, 2);
        auto hdr = conv.convert(primitive->operands.at(0));
        auto count = conv.convert(primitive->operands.at(1));
        auto methodName = primitive->name == "push" ?
                IR::Type_Stack::push_front : IR::Type_Stack::pop_front;
        auto method = new IR::Member(hdr, IR::ID(methodName));
        auto args = new IR::Vector<IR::Expression>();
        args->push_back(count);
        auto mc = new IR::MethodCallExpression(primitive->srcInfo, method, args);
        return new IR::MethodCallStatement(mc->srcInfo, mc);
    } else if (primitive->name == "count") {
        OPS_CK(primitive, 2);
        auto ref = primitive->operands.at(0);
        const IR::Counter *counter = nullptr;
        if (auto gr = ref->to<IR::GlobalRef>())
            counter = gr->obj->to<IR::Counter>();
        else if (auto nr = ref->to<IR::PathExpression>())
            counter = counters.get(nr->path->name);
        if (counter == nullptr) {
            ::error("Expected a counter reference %1%", ref);
            return nullptr; }
        auto newname = counters.get(counter);
        auto counterref = new IR::PathExpression(newname);
        auto methodName = v1model.counter.increment.Id();
        auto method = new IR::Member(counterref, methodName);
        auto args = new IR::Vector<IR::Expression>();
        auto arg = new IR::Cast(v1model.counter.index_type,
                                conv.convert(primitive->operands.at(1)));
        args->push_back(arg);
        auto mc = new IR::MethodCallExpression(primitive->srcInfo, method, args);
        return new IR::MethodCallStatement(mc->srcInfo, mc);
    } else if (primitive->name == "modify_field_from_rng") {
        BUG_CHECK(primitive->operands.size() == 2 || primitive->operands.size() == 3,
                  "Expected 2 or 3 operands for %1%", primitive);
        auto field = conv.convert(primitive->operands.at(0));
        auto logRange = conv.convert(primitive->operands.at(1));
        const IR::Expression* mask = nullptr;
        if (primitive->operands.size() == 3)
            mask = conv.convert(primitive->operands.at(2));

        cstring tmpvar = makeUniqueName("tmp");
        auto decl = new IR::Declaration_Variable(tmpvar, v1model.random.resultType, nullptr);
        auto args = new IR::Vector<IR::Expression>();
        args->push_back(new IR::PathExpression(tmpvar));
        args->push_back(new IR::Constant(primitive->operands.at(1)->srcInfo,
                                         v1model.random.resultType, 0));
        auto one = new IR::Constant(primitive->operands.at(1)->srcInfo, 1);
        args->push_back(new IR::Cast(primitive->operands.at(1)->srcInfo, v1model.random.resultType,
                                     new IR::Sub(new IR::Shl(one, logRange), one)));
        auto random = new IR::PathExpression(v1model.random.Id());
        auto mc = new IR::MethodCallExpression(primitive->srcInfo, random, args);
        auto call = new IR::MethodCallStatement(primitive->srcInfo, mc);
        auto block = new IR::BlockStatement;
        const IR::Statement* assgn;
        if (mask != nullptr)
            assgn = sliceAssign(primitive->srcInfo, field,
                                 new IR::PathExpression(IR::ID(tmpvar)), mask);
        else
            assgn = new IR::AssignmentStatement(field, new IR::PathExpression(tmpvar));
        block->push_back(decl);
        block->push_back(call);
        block->push_back(assgn);
        return block;
    } else if (primitive->name == "modify_field_rng_uniform") {
        BUG_CHECK(primitive->operands.size() == 3, "Expected 3 operands for %1%", primitive);
        auto field = conv.convert(primitive->operands.at(0));
        auto lo = conv.convert(primitive->operands.at(1));
        auto hi = conv.convert(primitive->operands.at(2));
        auto args = new IR::Vector<IR::Expression>();
        args->push_back(field);
        args->push_back(new IR::Cast(primitive->operands.at(1)->srcInfo,
                                     v1model.random.resultType, lo));
        args->push_back(new IR::Cast(primitive->operands.at(1)->srcInfo,
                                     v1model.random.resultType, hi));
        auto random = new IR::PathExpression(v1model.random.Id());
        auto mc = new IR::MethodCallExpression(primitive->srcInfo, random, args);
        auto call = new IR::MethodCallStatement(primitive->srcInfo, mc);
        return call;
    } else if (primitive->name == "recirculate") {
        OPS_CK(primitive, 1);
        auto right = convertFieldList(primitive->operands.at(0));
        if (right == nullptr)
            return nullptr;
        auto args = new IR::Vector<IR::Expression>();
        args->push_back(right);
        auto path = new IR::PathExpression(v1model.recirculate.Id());
        auto mc = new IR::MethodCallExpression(primitive->srcInfo, path, args);
        return new IR::MethodCallStatement(mc->srcInfo, mc);
    } else if (primitive->name == "clone_egress_pkt_to_egress" ||
               primitive->name == "clone_ingress_pkt_to_egress" ||
               primitive->name == "clone_i2e" || primitive->name == "clone_e2e") {
        BUG_CHECK(primitive->operands.size() == 1 || primitive->operands.size() == 2,
                  "Expected 1 or 2 operands for %1%", primitive);
        auto kind = primitive->name ==
                "clone_egress_pkt_to_egress" || primitive->name == "clone_e2e" ?
                v1model.clone.cloneType.e2e : v1model.clone.cloneType.i2e;

        auto session = conv.convert(primitive->operands.at(0));

        auto args = new IR::Vector<IR::Expression>();
        auto enumref = new IR::TypeNameExpression(
            new IR::Type_Name(new IR::Path(v1model.clone.cloneType.Id())));
        auto kindarg = new IR::Member(enumref, kind.Id());
        args->push_back(kindarg);
        args->push_back(new IR::Cast(primitive->operands.at(0)->srcInfo,
                                     v1model.clone.sessionType, session));
        if (primitive->operands.size() == 2) {
            auto list = convertFieldList(primitive->operands.at(1));
            if (list != nullptr)
                args->push_back(list);
        }

        auto id = primitive->operands.size() == 2 ? v1model.clone.clone3.Id() : v1model.clone.Id();
        auto clone = new IR::PathExpression(id);
        auto mc = new IR::MethodCallExpression(primitive->srcInfo, clone, args);
        return new IR::MethodCallStatement(mc->srcInfo, mc);
    } else if (primitive->name == "resubmit") {
        OPS_CK(primitive, 1);
        auto args = new IR::Vector<IR::Expression>();
        auto list = convertFieldList(primitive->operands.at(0));
        if (list != nullptr)
            args->push_back(list);
        auto resubmit = new IR::PathExpression(v1model.resubmit.Id());
        auto mc = new IR::MethodCallExpression(primitive->srcInfo, resubmit, args);
        return new IR::MethodCallStatement(mc->srcInfo, mc);
    } else if (primitive->name == "execute_meter") {
        OPS_CK(primitive, 3);
        auto ref = primitive->operands.at(0);
        const IR::Meter *meter = nullptr;
        if (auto gr = ref->to<IR::GlobalRef>())
            meter = gr->obj->to<IR::Meter>();
        else if (auto nr = ref->to<IR::PathExpression>())
            meter = meters.get(nr->path->name);
        if (!meter) {
            ::error("Expected a meter reference %1%", ref);
            return nullptr; }
        if (!meter->implementation.name.isNullOrEmpty())
            ::warning("Ignoring `implementation' field of meter %1%", meter);
        auto newname = meters.get(meter);
        auto meterref = new IR::PathExpression(newname);
        auto methodName = v1model.meter.executeMeter.Id();
        auto method = new IR::Member(meterref, methodName);
        auto args = new IR::Vector<IR::Expression>();
        auto arg = new IR::Cast(v1model.meter.index_type,
                                conv.convert(primitive->operands.at(1)));
        args->push_back(arg);
        auto dest = conv.convert(primitive->operands.at(2));
        args->push_back(dest);
        auto mc = new IR::MethodCallExpression(primitive->srcInfo, method, args);
        return new IR::MethodCallStatement(primitive->srcInfo, mc);
    } else if (primitive->name == "modify_field_with_hash_based_offset") {
        OPS_CK(primitive, 4);

        auto dest = conv.convert(primitive->operands.at(0));
        auto base = conv.convert(primitive->operands.at(1));
        auto max = conv.convert(primitive->operands.at(3));
        auto args = new IR::Vector<IR::Expression>();

        auto nr = primitive->operands.at(2)->to<IR::PathExpression>();
        auto flc = field_list_calculations.get(nr->path->name);
        if (flc == nullptr)
            ::error("%1%: Expected a field_list_calculation", primitive->operands.at(1));
        auto ttype = IR::Type_Bits::get(flc->output_width);
        auto fl = getFieldLists(flc);
        if (fl == nullptr)
            return nullptr;
        auto list = conv.convert(fl);

        auto algorithm = convertHashAlgorithm(flc->algorithm);
        args->push_back(dest);
        args->push_back(algorithm);
        args->push_back(new IR::Cast(ttype, base));
        args->push_back(list);
        args->push_back(new IR::Cast(IR::Type_Bits::get(2 * flc->output_width), max));
        auto hash = new IR::PathExpression(v1model.hash.Id());
        auto mc = new IR::MethodCallExpression(primitive->srcInfo, hash, args);
        auto result = new IR::MethodCallStatement(primitive->srcInfo, mc);
        return result;
    } else if (primitive->name == "modify_field_conditionally") {
        OPS_CK(primitive, 3);
        auto dest = conv.convert(primitive->operands.at(0));
        auto cond = conv.convert(primitive->operands.at(1));
        auto src = conv.convert(primitive->operands.at(2));
        if (!cond->type->is<IR::Type::Boolean>())
            cond = new IR::Neq(cond, new IR::Constant(0));
        src = new IR::Mux(primitive->srcInfo, cond, src, dest);
        return assign(primitive->srcInfo, dest, src, primitive->operands.at(0)->type);
    } else if (primitive->name == "generate_digest") {
        OPS_CK(primitive, 2);
        auto args = new IR::Vector<IR::Expression>();
        auto receiver = conv.convert(primitive->operands.at(0));
        args->push_back(new IR::Cast(primitive->operands.at(1)->srcInfo,
                                     v1model.digest_receiver.receiverType, receiver));
        auto list = convertFieldList(primitive->operands.at(1));
        auto type = createFieldListType(primitive->operands.at(1));
        declarations->push_back(type);  // In P4 v1.0 programs we can have declarations out of order
        if (list != nullptr)
            args->push_back(list);
        auto typeArgs = new IR::Vector<IR::Type>();
        auto typeName = new IR::Type_Name(new IR::Path(type->name));
        typeArgs->push_back(typeName);
        auto random = new IR::PathExpression(v1model.digest_receiver.Id());
        auto mc = new IR::MethodCallExpression(primitive->srcInfo, random, typeArgs, args);
        auto result = new IR::MethodCallStatement(mc->srcInfo, mc);
        return result;
    } else if (primitive->name == "register_read") {
        OPS_CK(primitive, 3);
        auto left = conv.convert(primitive->operands.at(0));
        auto ref = primitive->operands.at(1);
        const IR::Register *reg = nullptr;
        if (auto gr = ref->to<IR::GlobalRef>())
            reg = gr->obj->to<IR::Register>();
        else if (auto nr = ref->to<IR::PathExpression>())
            reg = registers.get(nr->path->name);
        if (!reg) {
            ::error("Expected a register reference %1%", ref);
            return nullptr; }
        auto newname = registers.get(reg);
        auto registerref = new IR::PathExpression(newname);
        auto methodName = v1model.registers.read.Id();
        auto method = new IR::Member(registerref, methodName);
        auto args = new IR::Vector<IR::Expression>();
        auto arg = new IR::Cast(v1model.registers.index_type,
                                conv.convert(primitive->operands.at(2)));
        args->push_back(left);
        args->push_back(arg);
        auto mc = new IR::MethodCallExpression(primitive->srcInfo, method, args);
        return new IR::MethodCallStatement(mc->srcInfo, mc);
    } else if (primitive->name == "register_write") {
        OPS_CK(primitive, 3);
        auto ref = primitive->operands.at(0);
        const IR::Register *reg = nullptr;
        if (auto gr = ref->to<IR::GlobalRef>())
            reg = gr->obj->to<IR::Register>();
        else if (auto nr = ref->to<IR::PathExpression>())
            reg = registers.get(nr->path->name);
        if (!reg) {
            ::error("Expected a register reference %1%", ref);
            return nullptr; }
        int width = reg->width;
        if (width <= 0)
            width = defaultRegisterWidth;
        auto regElementType = IR::Type_Bits::get(width);

        auto newname = registers.get(reg);
        auto registerref = new IR::PathExpression(newname);
        auto methodName = v1model.registers.write.Id();
        auto method = new IR::Member(registerref, methodName);
        auto args = new IR::Vector<IR::Expression>();
        auto arg0 = new IR::Cast(primitive->operands.at(1)->srcInfo, v1model.registers.index_type,
                                 conv.convert(primitive->operands.at(1)));
        auto arg1 = new IR::Cast(primitive->operands.at(2)->srcInfo, regElementType,
                                 conv.convert(primitive->operands.at(2)));
        args->push_back(arg0);
        args->push_back(arg1);
        auto mc = new IR::MethodCallExpression(primitive->srcInfo, method, args);
        return new IR::MethodCallStatement(mc->srcInfo, mc);
    } else if (primitive->name == "truncate") {
        OPS_CK(primitive, 1);
        auto len = primitive->operands.at(0);
        auto methodName = v1model.truncate.Id();
        auto method = new IR::PathExpression(methodName);
        auto args = new IR::Vector<IR::Expression>();
        auto arg0 = new IR::Cast(len->srcInfo, v1model.truncate.length_type, conv.convert(len));
        args->push_back(arg0);
        auto mc = new IR::MethodCallExpression(primitive->srcInfo, method, args);
        return new IR::MethodCallStatement(mc->srcInfo, mc);
    } else if (primitive->name == "exit") {
        OPS_CK(primitive, 0);
        return new IR::ExitStatement(primitive->srcInfo);
    } else if (primitive->name == "funnel_shift_right") {
        OPS_CK(primitive, 4);
        auto dest = conv.convert(primitive->operands.at(0));
        auto hi = conv.convert(primitive->operands.at(1));
        auto lo = conv.convert(primitive->operands.at(2));
        auto shift = conv.convert(primitive->operands.at(3));
        auto src = new IR::Shr(new IR::Concat(hi, lo), shift);
        return assign(primitive->srcInfo, dest, src, primitive->operands.at(0)->type);
    }

    // If everything else failed maybe we are invoking an action
    auto action = actions.get(primitive->name);
    if (action != nullptr) {
        auto newname = actions.get(action);
        auto act = new IR::PathExpression(newname);
        auto args = new IR::Vector<IR::Expression>();
        for (auto a : primitive->operands) {
            auto e = conv.convert(a);
            args->push_back(e);
        }
        auto call = new IR::MethodCallExpression(primitive->srcInfo, act, args);
        auto stat = new IR::MethodCallStatement(primitive->srcInfo, call);
        return stat;
    }

    BUG("Unhandled primitive %1%", primitive);
    return nullptr;
}

const IR::P4Action*
ProgramStructure::convertAction(const IR::ActionFunction* action, cstring newName,
                                const IR::Meter* meterToAccess,
                                cstring counterToAccess) {
    LOG1("Converting action " << action->name);
    auto body = new IR::BlockStatement;
    auto params = new IR::ParameterList;
    bool isCalled = calledActions.isCallee(action->name.name);
    for (auto p : action->args) {
        IR::Direction direction;
        if (!isCalled)
            direction = IR::Direction::None;
        else
            direction = p->write ? IR::Direction::InOut : IR::Direction::None;
        auto type = p->type;
        if (type == IR::Type_Unknown::get()) {
            ::warning("Could not infer type for %1%, using bit<8>", p);
            type = IR::Type_Bits::get(8);
        } else if (type->is<IR::Type_StructLike>()) {
            auto path = new IR::Path(type->to<IR::Type_StructLike>()->name);
            type = new IR::Type_Name(path);
        }
        auto param = new IR::Parameter(p->srcInfo, p->name, direction, type);
        LOG2("  " << p << " is " << direction << " " << param);
        params->push_back(param); }

    if (meterToAccess != nullptr) {
        ExpressionConverter conv(this);
        // add a writeback to the meter field
        auto decl = get(meterMap, meterToAccess);
        CHECK_NULL(decl);
        auto extObj = new IR::PathExpression(decl->name);
        auto method = new IR::Member(extObj, v1model.directMeter.read.Id());
        auto args = new IR::Vector<IR::Expression>();
        auto arg = conv.convert(meterToAccess->result);
        if (arg != nullptr)
            args->push_back(arg);
        auto mc = new IR::MethodCallExpression(method, args);
        auto stat = new IR::MethodCallStatement(mc->srcInfo, mc);
        body->push_back(stat); }

    if (counterToAccess != nullptr) {
        ExpressionConverter conv(this);
        auto decl = get(counterMap, counterToAccess);
        CHECK_NULL(decl);
        auto extObj = new IR::PathExpression(decl->name);
        auto method = new IR::Member(extObj, v1model.directCounter.count.Id());
        auto args = new IR::Vector<IR::Expression>();
        auto mc = new IR::MethodCallExpression(method, args);
        auto stat = new IR::MethodCallStatement(mc->srcInfo, mc);
        body->push_back(stat); }

    for (auto p : action->action) {
        auto stat = convertPrimitive(p);
        if (stat != nullptr)
            body->push_back(stat); }

    // Save the original action name in an annotation
    // The leading dot indicates a global action
    auto annos = addNameAnnotation(cstring(".") + action->name, action->annotations);
    auto result = new IR::P4Action(action->srcInfo, newName, annos, params, body);
    return result;
}

const IR::Type_Control* ProgramStructure::controlType(IR::ID name) {
    auto params = new IR::ParameterList;

    auto headpath = new IR::Path(v1model.headersType.Id());
    auto headtype = new IR::Type_Name(headpath);
    // we can use ingress, since all control blocks have the same signature
    auto headers = new IR::Parameter(v1model.ingress.headersParam.Id(),
                                     IR::Direction::InOut, headtype);
    params->push_back(headers);
    conversionContext.header = paramReference(headers);

    auto metapath = new IR::Path(v1model.metadataType.Id());
    auto metatype = new IR::Type_Name(metapath);
    auto meta = new IR::Parameter(v1model.ingress.metadataParam.Id(),
                                  IR::Direction::InOut, metatype);
    params->push_back(meta);
    conversionContext.userMetadata = paramReference(meta);

    auto stdMetaPath = new IR::Path(v1model.standardMetadataType.Id());
    auto stdMetaType = new IR::Type_Name(stdMetaPath);
    auto stdmeta = new IR::Parameter(v1model.ingress.standardMetadataParam.Id(),
                                     IR::Direction::InOut, stdMetaType);
    params->push_back(stdmeta);
    conversionContext.standardMetadata = paramReference(stdmeta);

    auto type = new IR::Type_Control(name, new IR::TypeParameters(), params);
    return type;
}

const IR::Expression* ProgramStructure::counterType(const IR::CounterOrMeter* cm) const {
    IR::ID kind;
    IR::ID enumName;
    if (cm->is<IR::Counter>()) {
        enumName = v1model.counterOrMeter.counterType.Id();
        if (cm->type == IR::CounterType::PACKETS)
            kind = v1model.counterOrMeter.counterType.packets.Id();
        else if (cm->type == IR::CounterType::BYTES)
            kind = v1model.counterOrMeter.counterType.bytes.Id();
        else if (cm->type == IR::CounterType::BOTH)
            kind = v1model.counterOrMeter.counterType.both.Id();
        else
            BUG("%1%: unsupported", cm);
    } else {
        BUG_CHECK(cm->is<IR::Meter>(), "%1%: expected a meter", cm);
        enumName = v1model.counterOrMeter.meterType.Id();
        if (cm->type == IR::CounterType::PACKETS)
            kind = v1model.counterOrMeter.meterType.packets.Id();
        else if (cm->type == IR::CounterType::BYTES)
            kind = v1model.counterOrMeter.meterType.bytes.Id();
        else
            BUG("%1%: unsupported", cm);
    }
    auto enumref = new IR::TypeNameExpression(new IR::Type_Name(new IR::Path(enumName)));
    return new IR::Member(enumref, kind);
}

const IR::Declaration_Instance*
ProgramStructure::convert(const IR::Register* reg, cstring newName) {
    LOG1("Synthesizing " << reg);
    const IR::Type *regElementType = nullptr;
    if (reg->width > 0) {
        regElementType = IR::Type_Bits::get(reg->width);
    } else if (reg->layout) {
        regElementType = types.get(reg->layout);
    } else {
        ::warning("%1%: Register width unspecified; using %2%", reg, defaultRegisterWidth);
        regElementType = IR::Type_Bits::get(defaultRegisterWidth);
    }

    IR::ID ext = v1model.registers.Id();
    auto typepath = new IR::Path(ext);
    auto type = new IR::Type_Name(typepath);
    auto typeargs = new IR::Vector<IR::Type>();
    typeargs->push_back(regElementType);
    auto spectype = new IR::Type_Specialized(type, typeargs);
    auto args = new IR::Vector<IR::Expression>();
    if (reg->direct) {
        // FIXME -- do we need a direct_register extern in v1model?  For now
        // using size of 0 to specify a direct register
        args->push_back(new IR::Constant(v1model.registers.size_type, 0));
    } else {
        args->push_back(new IR::Constant(v1model.registers.size_type, reg->instance_count)); }
    auto annos = addNameAnnotation(reg->name, reg->annotations);
    auto decl = new IR::Declaration_Instance(newName, annos, spectype, args, nullptr);
    return decl;
}

const IR::Declaration_Instance*
ProgramStructure::convert(const IR::CounterOrMeter* cm, cstring newName) {
    LOG1("Synthesizing " << cm);
    IR::ID ext;
    if (cm->is<IR::Counter>())
        ext = v1model.counter.Id();
    else
        ext = v1model.meter.Id();
    auto typepath = new IR::Path(ext);
    auto type = new IR::Type_Name(typepath);
    auto args = new IR::Vector<IR::Expression>();
    args->push_back(new IR::Constant(v1model.counterOrMeter.size_type, cm->instance_count));
    auto kindarg = counterType(cm);
    args->push_back(kindarg);
    auto annos = addNameAnnotation(cm->name, cm->annotations);
    if (auto *c = cm->to<IR::Counter>()) {
        if (c->min_width >= 0)
            annos = annos->addAnnotation("min_width", new IR::Constant(c->min_width));
        if (c->max_width >= 0)
            annos = annos->addAnnotation("max_width", new IR::Constant(c->max_width));
    }
    auto decl = new IR::Declaration_Instance(
        newName, annos, type, args, nullptr);
    return decl;
}

const IR::Declaration_Instance*
ProgramStructure::convertExtern(const IR::Declaration_Instance *ext, cstring newName) {
    LOG1("Synthesizing " << ext);
    auto *rv = ext->clone();
    auto *et = rv->type->to<IR::Type_Extern>();
    BUG_CHECK(et, "Extern %s is not extern type, but %s", ext, ext->type);
    rv->name = newName;
    rv->type = new IR::Type_Name(new IR::Path(extern_types.get(et)));
    return rv->apply(TypeConverter(this))->to<IR::Declaration_Instance>();;
}

const IR::Declaration_Instance*
ProgramStructure::convertDirectMeter(const IR::Meter* m, cstring newName) {
    LOG1("Synthesizing " << m);
    auto meterOutput = m->result;
    if (meterOutput == nullptr) {
        ::error("%1%: direct meter with no result", m);
        return nullptr;
    }

    auto outputType = meterOutput->type;
    CHECK_NULL(outputType);
    IR::ID ext = v1model.directMeter.Id();
    auto typepath = new IR::Path(ext);
    auto type = new IR::Type_Name(typepath);
    auto vec = new IR::Vector<IR::Type>();
    vec->push_back(outputType);
    auto specType = new IR::Type_Specialized(type, vec);
    auto args = new IR::Vector<IR::Expression>();
    auto kindarg = counterType(m);
    args->push_back(kindarg);
    auto annos = addNameAnnotation(m->name, m->annotations);
    auto decl = new IR::Declaration_Instance(newName, annos, specType, args, nullptr);
    return decl;
}

const IR::Declaration_Instance*
ProgramStructure::convertDirectCounter(const IR::Counter* c, cstring newName) {
    LOG1("Synthesizing " << c);

    IR::ID ext = v1model.directCounter.Id();
    auto typepath = new IR::Path(ext);
    auto type = new IR::Type_Name(typepath);
    auto args = new IR::Vector<IR::Expression>();
    auto kindarg = counterType(c);
    args->push_back(kindarg);
    auto annos = addNameAnnotation(c->name, c->annotations);
    auto decl = new IR::Declaration_Instance(newName, annos, type, args, nullptr);
    return decl;
}

const IR::P4Control*
ProgramStructure::convertControl(const IR::V1Control* control, cstring newName) {
    IR::ID name = newName;
    auto type = controlType(name);
    std::vector<cstring> actionsInTables;
    IR::IndexedVector<IR::Declaration> stateful;

    std::vector<const IR::V1Table*> usedTables;
    tablesReferred(control, usedTables);
    for (auto t : usedTables) {
        for (auto a : t->actions)
            actionsInTables.push_back(a.name);
        if (!t->default_action.name.isNullOrEmpty())
            actionsInTables.push_back(t->default_action.name);
        if (!t->action_profile.name.isNullOrEmpty()) {
            auto ap = action_profiles.get(t->action_profile.name);
            for (auto a : ap->actions)
                actionsInTables.push_back(a.name);
        }
    }

    std::vector<cstring> actionsToDo;
    calledActions.sort(actionsInTables, actionsToDo);

    std::set<cstring> countersToDo;
    for (auto a : actionsToDo)
        calledCounters.getCallees(a, countersToDo);
    for (auto c : counters) {
        if (c.first->direct) {
            if (c.first->table.name.isNullOrEmpty()) {
                ::error("%1%: Direct counter with no table", c.first);
                return nullptr;
            }
            auto tbl = tables.get(c.first->table.name);
            if (tbl == nullptr) {
                ::error("Cannot locate table %1%", c.first->table.name);
                return nullptr;
            }
            if (std::find(usedTables.begin(), usedTables.end(), tbl) != usedTables.end()) {
                auto counter = counters.get(c.second);
                auto extcounter = convertDirectCounter(counter, c.second);
                if (extcounter != nullptr) {
                    stateful.push_back(extcounter);
                    directCounters.emplace(c.first->table.name, c.first->name);
                    counterMap.emplace(c.first->name, extcounter);
                }
            }
        }
    }
    for (auto c : countersToDo) {
        auto ctr = counters.get(c);
        auto counter = convert(ctr, counters.get(ctr));
        stateful.push_back(counter);
    }

    for (auto m : meters) {
        if (m.first->direct) {
            if (m.first->table.name.isNullOrEmpty()) {
                ::error("%1%: Direct meter with no table", m.first);
                return nullptr;
            }
            auto tbl = tables.get(m.first->table.name);
            if (tbl == nullptr) {
                ::error("Cannot locate table %1%", m.first->table.name);
                return nullptr;
            }
            if (std::find(usedTables.begin(), usedTables.end(), tbl) != usedTables.end()) {
                auto meter = meters.get(m.second);
                auto extmeter = convertDirectMeter(meter, m.second);
                if (extmeter != nullptr) {
                    stateful.push_back(extmeter);
                    directMeters.emplace(m.first->table.name, meter);
                    meterMap.emplace(meter, extmeter);
                }
            }
        }
    }

    std::set<cstring> metersToDo;
    std::set<cstring> registersToDo;
    std::set<cstring> externsToDo;
    for (auto a : actionsToDo) {
        calledMeters.getCallees(a, metersToDo);
        calledRegisters.getCallees(a, registersToDo);
        calledExterns.getCallees(a, externsToDo);
    }
    for (auto c : externsToDo) {
        calledRegisters.getCallees(c, registersToDo);
    }
    for (auto c : metersToDo) {
        auto mtr = meters.get(c);
        auto meter = convert(mtr, meters.get(mtr));
        stateful.push_back(meter);
    }

    for (auto c : registersToDo) {
        auto reg = registers.get(c);
        auto r = convert(reg, registers.get(reg));
        stateful.push_back(r);
    }

    for (auto c : externsToDo) {
        auto ext = externs.get(c);
        ext = convertExtern(ext, externs.get(ext));
        stateful.push_back(ext);
    }

    for (auto a : actionsToDo) {
        auto act = actions.get(a);
        if (act == nullptr) {
            ::error("Cannot locate action %1%", a);
            return nullptr;
        }
        auto action = convertAction(act, actions.get(act), nullptr, nullptr);
        stateful.push_back(action);
    }

    std::set<cstring> tablesDone;
    std::map<cstring, cstring> instanceNames;
    for (auto t : usedTables) {
        if (tablesDone.find(t->name.name) != tablesDone.end())
            continue;
        auto tbl = convertTable(t, tables.get(t), stateful, instanceNames);
        if (tbl != nullptr)
            stateful.push_back(tbl);
        tablesDone.emplace(t->name.name);
    }

    if (calledControls.isCaller(name.name)) {
        for (auto cc : *calledControls.getCallees(name.name)) {
            if (instanceNames.find(cc) != instanceNames.end()) continue;
            cstring iname = makeUniqueName(cc);
            instanceNames.emplace(cc, iname);
            auto control = controls.get(cc);
            cstring newname = controls.get(control);
            auto typepath = new IR::Path(IR::ID(newname));
            auto type = new IR::Type_Name(typepath);
            auto annos = addNameAnnotation(cc);
            auto decl = new IR::Declaration_Instance(IR::ID(iname), annos, type,
                new IR::Vector<IR::Expression>(), nullptr);
            stateful.push_back(decl);
        }
    }

    StatementConverter conv(this, &instanceNames);
    auto body = new IR::BlockStatement(control->annotations);
    for (auto e : *control->code) {
        auto s = conv.convert(e);
        body->push_back(s);
    }

    auto result = new IR::P4Control(name, type, stateful, body);
    conversionContext.clear();
    return result;
}

void ProgramStructure::createControls() {
    std::vector<cstring> controlsToDo;
    std::vector<cstring> knownControls;

    for (auto it : controls)
        knownControls.push_back(it.second);
    bool cycles = calledControls.sort(knownControls, controlsToDo);
    if (cycles) {
        // TODO: give a better error message
        ::error("Program contains recursive control blocks");
        return;
    }

    for (auto c : controlsToDo) {
        auto ct = controls.get(c);
        auto ctrl = convertControl(ct, controls.get(ct));
        if (ctrl == nullptr)
            return;
        declarations->push_back(ctrl);
    }

    auto egress = controls.get(v1model.egress.name);
    if (egress == nullptr) {
        auto name = v1model.egress.Id();
        auto type = controlType(name);
        auto body = new IR::BlockStatement;
        auto egressControl = new IR::P4Control(name, type, body);
        declarations->push_back(egressControl);
    }
}

void ProgramStructure::createMain() {
    auto name = IR::ID(IR::P4Program::main);
    auto typepath = new IR::Path(v1model.sw.Id());
    auto type = new IR::Type_Name(typepath);
    auto args = new IR::Vector<IR::Expression>();

    auto emptyArgs = new IR::Vector<IR::Expression>();

    auto parserPath = new IR::Path(v1model.parser.Id());
    auto parserType = new IR::Type_Name(parserPath);
    auto parserConstruct = new IR::ConstructorCallExpression(parserType, emptyArgs);
    args->push_back(parserConstruct);

    auto verifyPath = new IR::Path(verifyChecksums->name);
    auto verifyType = new IR::Type_Name(verifyPath);
    auto verifyConstruct = new IR::ConstructorCallExpression(verifyType, emptyArgs);
    args->push_back(verifyConstruct);

    auto ingressPath = new IR::Path(ingressReference);
    auto ingressType = new IR::Type_Name(ingressPath);
    auto ingressConstruct = new IR::ConstructorCallExpression(ingressType, emptyArgs);
    args->push_back(ingressConstruct);

    auto egressPath = new IR::Path(v1model.egress.Id());
    auto egressType = new IR::Type_Name(egressPath);
    auto egressConstruct = new IR::ConstructorCallExpression(egressType, emptyArgs);
    args->push_back(egressConstruct);

    auto updatePath = new IR::Path(updateChecksums->name);
    auto updateType = new IR::Type_Name(updatePath);
    auto updateConstruct = new IR::ConstructorCallExpression(updateType, emptyArgs);
    args->push_back(updateConstruct);

    auto deparserPath = new IR::Path(deparser->name);
    auto deparserType = new IR::Type_Name(deparserPath);
    auto deparserConstruct = new IR::ConstructorCallExpression(deparserType, emptyArgs);
    args->push_back(deparserConstruct);

    auto result = new IR::Declaration_Instance(name, type, args, nullptr);
    declarations->push_back(result);
}

cstring ProgramStructure::mapAlgorithm(IR::ID algorithm) const {
    if (algorithm.name == "csum16")
        return v1model.ck16.name;
    ::error("Unsupported algorithm %1%", algorithm);
    return nullptr;
}

const IR::Declaration_Instance*
ProgramStructure::checksumUnit(const IR::FieldListCalculation* flc) {
    auto ext = mapAlgorithm(flc->algorithm);
    if (ext == nullptr)
        return nullptr;
    auto typePath = new IR::Path(IR::ID(ext));
    auto extType = new IR::Type_Name(typePath);
    auto newName = field_list_calculations.get(flc);
    auto inst = new IR::Declaration_Instance(
        flc->srcInfo, newName, flc->annotations, extType,
        new IR::Vector<IR::Expression>(), nullptr);
    return inst;
}

// if a FieldListCalculation contains multiple field lists concatenate them all
// into a temporary field list
const IR::FieldList* ProgramStructure::getFieldLists(const IR::FieldListCalculation* flc) {
    // FIXME -- this duplicates P4_14::TypeCheck.  Why not just use flc->input_fields?
    if (flc->input->names.size() == 0)
        ::error("%1%: field_list_calculation with zero inputs", flc);
    if (flc->input->names.size() == 1) {
        auto name = flc->input->names.at(0);
        auto result = field_lists.get(name);
        if (result == nullptr)
            ::error("Could not find field_list %1%", name);
        return result;
    }

    IR::FieldList* result = new IR::FieldList();
    result->payload = false;
    for (auto name : flc->input->names) {
        auto fl = field_lists.get(name);
        if (fl == nullptr) {
            ::error("Could not find field_list %1%", name);
            return nullptr;
        }
        result->fields.insert(result->fields.end(), fl->fields.begin(), fl->fields.end());
        /* FIXME -- do something with fl->annotations? */
    }
    return result;
}

void ProgramStructure::createChecksumVerifications() {
    ExpressionConverter conv(this);

    // verify
    auto params = new IR::ParameterList;
    auto headpath = new IR::Path(v1model.headersType.Id());
    auto headtype = new IR::Type_Name(headpath);
    auto headers = new IR::Parameter(v1model.verify.headersParam.Id(),
                                     IR::Direction::In, headtype);
    params->push_back(headers);
    conversionContext.header = paramReference(headers);

    auto metapath = new IR::Path(v1model.metadataType.Id());
    auto metatype = new IR::Type_Name(metapath);
    auto meta = new IR::Parameter(v1model.parser.metadataParam.Id(),
                                  IR::Direction::InOut, metatype);
    params->push_back(meta);
    conversionContext.userMetadata = paramReference(meta);

    conversionContext.standardMetadata = nullptr;

    auto type = new IR::Type_Control(v1model.verify.Id(), params);

    std::map<const IR::FieldListCalculation*, const IR::Declaration_Instance*> map;
    IR::IndexedVector<IR::Declaration> stateful;
    for (auto cf : calculated_fields) {
        // FIXME -- do something with cf->annotations?
        for (auto uov : cf->specs) {
            if (uov.update) continue;
            auto flc = field_list_calculations.get(uov.name.name);
            if (flc == nullptr) {
                ::error("Cannot find field_list_calculation %1%", uov.name);
                continue;
            }
            if (map.find(flc) != map.end())
                continue;
            auto inst = checksumUnit(flc);
            if (inst == nullptr)
                continue;
            stateful.push_back(inst);
            map.emplace(flc, inst);
        }
    }

    auto body = new IR::BlockStatement;
    for (auto cf : calculated_fields) {
        LOG1("Converting " << cf);
        auto dest = conv.convert(cf->field);

        for (auto uov : cf->specs) {
            if (uov.update) continue;
            auto flc = field_list_calculations.get(uov.name.name);
            auto inst = get(map, flc);

            auto fl = getFieldLists(flc);
            if (fl == nullptr) continue;
            auto le = conv.convert(fl);
            auto extObj = new IR::PathExpression(inst->name);
            // The only method we currently know about is v1model.ck16.get
            auto method = new IR::Member(extObj, v1model.ck16.get.Id());
            auto args = new IR::Vector<IR::Expression>();
            args->push_back(le);
            auto mc = new IR::MethodCallExpression(method, args);
            const IR::Expression* cond = new IR::Equ(uov.srcInfo, dest, mc);
            if (uov.cond != nullptr) {
                auto cond2 = conv.convert(uov.cond);
                // cond2 is evaluated first
                cond = new IR::LAnd(cond2, cond);
            }
            auto dropmethod = new IR::PathExpression(v1model.drop.Id());
            auto dropmc = new IR::MethodCallExpression(dropmethod);
            auto drop = new IR::MethodCallStatement(mc->srcInfo, dropmc);
            auto ifstate = new IR::IfStatement(cond, drop, nullptr);
            body->push_back(ifstate);
            LOG1("Converted " << flc);
        }
    }
    verifyChecksums = new IR::P4Control(v1model.verify.Id(), type, stateful, body);
    declarations->push_back(verifyChecksums);
    conversionContext.clear();
}

void ProgramStructure::createChecksumUpdates() {
    ExpressionConverter conv(this);
    auto params = new IR::ParameterList;
    auto headpath = new IR::Path(v1model.headersType.Id());
    auto headtype = new IR::Type_Name(headpath);
    auto headers = new IR::Parameter(v1model.update.headersParam.Id(),
                                     IR::Direction::InOut, headtype);
    params->push_back(headers);
    conversionContext.header = paramReference(headers);

    auto metapath = new IR::Path(v1model.metadataType.Id());
    auto metatype = new IR::Type_Name(metapath);
    auto meta = new IR::Parameter(v1model.parser.metadataParam.Id(),
                                  IR::Direction::InOut, metatype);
    params->push_back(meta);
    conversionContext.userMetadata = paramReference(meta);

    conversionContext.standardMetadata = nullptr;

    auto type = new IR::Type_Control(v1model.update.Id(), params);

    IR::IndexedVector<IR::Declaration> stateful;
    auto body = new IR::BlockStatement;

    std::map<const IR::FieldListCalculation*, const IR::Declaration_Instance*> map;
    for (auto cf : calculated_fields) {
        for (auto uov : cf->specs) {
            if (!uov.update) continue;
            auto flc = field_list_calculations.get(uov.name.name);
            if (flc == nullptr) {
                ::error("Cannot find field_list_calculation %1%", uov.name);
                continue;
            }
            if (map.find(flc) != map.end())
                continue;
            auto inst = checksumUnit(flc);
            if (inst == nullptr)
                continue;
            stateful.push_back(inst);
            map.emplace(flc, inst);
        }
    }

    for (auto cf : calculated_fields) {
        LOG1("Converting " << cf);
        auto dest = conv.convert(cf->field);

        for (auto uov : cf->specs) {
            if (!uov.update) continue;
            auto flc = field_list_calculations.get(uov.name.name);
            auto inst = get(map, flc);

            auto fl = getFieldLists(flc);
            if (fl == nullptr) continue;
            auto le = conv.convert(fl);

            auto extObj = new IR::PathExpression(inst->name);
            // The only method we currently know about is v1model.ck16.get
            auto method = new IR::Member(extObj, v1model.ck16.get.Id());
            auto args = new IR::Vector<IR::Expression>();
            args->push_back(le);
            auto mc = new IR::MethodCallExpression(method, args);
            const IR::Statement* set = new IR::AssignmentStatement(flc->srcInfo, dest, mc);
            if (uov.cond != nullptr) {
                auto cond = conv.convert(uov.cond);
                set = new IR::IfStatement(cond, set, nullptr);
            }
            body->push_back(set);
            LOG1("Converted " << flc);
        }
    }
    updateChecksums = new IR::P4Control(v1model.update.Id(), type, stateful, body);
    declarations->push_back(updateChecksums);
    conversionContext.clear();
}

const IR::P4Program* ProgramStructure::create(Util::SourceInfo info) {
    createTypes();
    createStructures();
    createExterns();
    createParser();
    if (::errorCount())
        return nullptr;
    createControls();
    if (::errorCount())
        return nullptr;
    createDeparser();
    createChecksumVerifications();
    createChecksumUpdates();
    createMain();
    if (::errorCount())
        return nullptr;
    auto result = new IR::P4Program(info, *declarations);
    return result;
}

void
ProgramStructure::tablesReferred(const IR::V1Control* control,
                                 std::vector<const IR::V1Table*> &out) {
    LOG1("Inspecting " << control->name);
    for (auto it : tableMapping) {
        if (it.second == control)
            out.push_back(it.first);
    }
    // sort alphabetically to have a deterministic order
    std::sort(out.begin(), out.end(),
              [](const IR::V1Table* left, const IR::V1Table* right) {
                  return left->name.name < right->name.name; });
}

}  // namespace P4V1
