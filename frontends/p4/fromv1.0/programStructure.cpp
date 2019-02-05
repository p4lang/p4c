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
        value_sets(&allNames),
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
    return annos->addAnnotationIfNew(IR::Annotation::nameAnnotation,
                                     new IR::StringLiteral(name));
}

const IR::Annotations*
ProgramStructure::addGlobalNameAnnotation(cstring name,
                                          const IR::Annotations* annos) {
    return addNameAnnotation(cstring(".") + name, annos);
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
    LOG3("Checking " << hdr << " " << (metadata ? "M" : "H"));
    for (auto f : hdr->fields) {
        if (f->type->is<IR::Type_Varbits>()) {
            if (metadata)
                ::error("%1%: varbit types illegal in metadata", f);
        } else if (!f->type->is<IR::Type_Bits>()) {
            // These come from P4-14, so they cannot be anything else
            BUG("%1%: unexpected type", f); }
    }
}

cstring ProgramStructure::createType(const IR::Type_StructLike* type, bool header,
                                  std::unordered_set<const IR::Type*> *converted) {
    if (converted->count(type))
        return type->name;
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
    return type_name;
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
            cstring layoutTypeName = it.first->layout;
            auto type = types.get(layoutTypeName);
            if (converted.count(type) || !type->is<IR::Type_StructLike>())
                continue;
            auto st = type->to<IR::Type_StructLike>();
            if (type->is<IR::Type_Struct>()) {
                cstring newName = createType(type, false, &converted);
                registerLayoutType.emplace(layoutTypeName, newName);
                continue;
            }

            BUG_CHECK(type->is<IR::Type_Header>(), "%1%: unexpected type", type);
            // Must convert to a struct type
            cstring type_name = makeUniqueName(st->name);
            // Registers always use struct types
            auto annos = addNameAnnotation(layoutTypeName, type->annotations);
            auto newType = new IR::Type_Struct(type->srcInfo, type_name, annos, st->fields);
            checkHeaderType(newType, false);
            LOG3("Added type " << dbp(newType) << " named " << type_name << " from " << dbp(type));
            declarations->push_back(newType);
            registerLayoutType.emplace(layoutTypeName, newType->name);
        }
    }

    // Now headers
    converted.clear();
    for (auto it : headers) {
        auto type = it.first->type;
        CHECK_NULL(type);
        createType(type, true, &converted);
    }
    for (auto it : stacks) {
        auto type = it.first->type;
        CHECK_NULL(type);
        createType(type, true, &converted);
    }
}

const IR::Type_Struct* ProgramStructure::createFieldListType(const IR::Expression* expression) {
    if (!expression->is<IR::PathExpression>()) {
      ::error("%1%: expected a field list", expression);
      return nullptr;
    }
    auto nr = expression->to<IR::PathExpression>();
    auto fl = field_lists.get(nr->path->name);
    if (fl == nullptr) {
      ::error("%1%: Expected a field list", expression);
      return nullptr;
    }

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
        else if (f->is<IR::ConcreteHeaderRef>())
            name = f->to<IR::ConcreteHeaderRef>()->ref->name;
        else
            BUG("%1%: unexpected field list member", f);
        name = cstring::make_unique(fieldNames, name, '_');
        fieldNames.emplace(name);
        auto type = f->type->getP4Type();
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
        auto annos = addGlobalNameAnnotation(id, it.first->annotations);
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
        auto annos = addGlobalNameAnnotation(id, it.first->annotations);
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
        auto annos = addGlobalNameAnnotation(id, it.first->annotations);
        auto field = new IR::StructField(id.srcInfo, id, annos, stack);
        headers->fields.push_back(field);
    }
    declarations->push_back(headers);
}

void ProgramStructure::createExterns() {
    for (auto it : extern_types) {
        if (auto et = ExternConverter::cvtExternType(this, it.first, it.second)) {
            if (et != it.first)
                extern_remap[it.first] = et;
            if (et != declarations->getDeclaration(et->name))
                declarations->push_back(et); } }
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
    if (type != nullptr && type != right->type) {
        auto t1 = right->type->to<IR::Type::Bits>();
        auto t2 = type->to<IR::Type::Bits>();
        if (t1 && t2 && t1->size != t2->size && t1->isSigned != t2->isSigned) {
            /* P4_16 does not allow changing both size and signedness with a single cast,
             * so we need two, changing size first, then signedness */
            right = new IR::Cast(IR::Type::Bits::get(t2->size, t1->isSigned), right); }
        right = new IR::Cast(type, right); }
    return new IR::AssignmentStatement(srcInfo, left, right);
}

const IR::Statement* ProgramStructure::convertParserStatement(const IR::Expression* expr) {
    ExpressionConverter conv(this);

    if (expr->is<IR::Primitive>()) {
        auto primitive = expr->to<IR::Primitive>();
        if (primitive->name == "extract") {
            BUG_CHECK(primitive->operands.size() == 1, "Expected 1 operand for %1%", primitive);
            auto dest = conv.convert(primitive->operands.at(0));
            auto destType = dest->type;
            CHECK_NULL(destType);
            BUG_CHECK(destType->is<IR::Type_Header>(), "%1%: expected a header", destType);
            auto finalDestType = ::get(
                finalHeaderType, destType->to<IR::Type_Header>()->externalName());
            BUG_CHECK(finalDestType != nullptr, "%1%: could not find final type",
                      destType->to<IR::Type_Header>()->externalName());
            destType = finalDestType;
            BUG_CHECK(destType->is<IR::Type_Header>(), "%1%: expected a header", destType);

            // A second conversion of dest is used to compute the
            // 'latest' (P4-14 keyword) value if referenced later.
            // Some passes somewhere are apparently broken and can't deal with dags
            conv.replaceNextWithLast = true;
            this->latest = conv.convert(primitive->operands.at(0));
            conv.replaceNextWithLast = false;
            const IR::Expression* method = new IR::Member(
                paramReference(parserPacketIn),
                p4lib.packetIn.extract.Id());
            auto result = new IR::MethodCallStatement(
                expr->srcInfo, method, { new IR::Argument(dest) });

            LOG3("Inserted extract " << dbp(result->methodCall) << " " << dbp(destType));
            extractsSynthesized.emplace(result->methodCall, destType->to<IR::Type_Header>());
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
        } else {
            ::error("%1%: unknown state", dest);
            return nullptr;
        }
    }
}

static const IR::Expression*
explodeLabel(const IR::Constant* value, const IR::Constant* mask,
             const std::vector<const IR::Type::Bits *> &fieldTypes) {
    if (mask->value == 0)
        return new IR::DefaultExpression(value->srcInfo);
    bool useMask = mask->value != -1;

    mpz_class v = value->value;
    mpz_class m = mask->value;

    auto rv = new IR::ListExpression(value->srcInfo, {});
    for (auto it = fieldTypes.rbegin(); it != fieldTypes.rend(); ++it) {
        int sz = (*it)->width_bits();
        auto bits = Util::ripBits(v, sz);
        const IR::Expression* expr = new IR::Constant(value->srcInfo, *it, bits, value->base);
        if (useMask) {
            auto maskbits = Util::ripBits(m, sz);
            auto maskcst = new IR::Constant(mask->srcInfo, *it, maskbits, mask->base);
            expr = new IR::Mask(mask->srcInfo, expr, maskcst);
        }
        rv->components.insert(rv->components.begin(), expr);
    }
    if (rv->components.size() == 1)
        return rv->components.at(0);
    return rv;
}

static const IR::Type*
explodeType(const std::vector<const IR::Type::Bits *> &fieldTypes) {
    auto rv = new IR::Vector<IR::Type>();
    for (auto it = fieldTypes.begin(); it != fieldTypes.end(); ++it) {
        rv->push_back(*it);
    }
    if (rv->size() == 1)
        return rv->at(0);
    return new IR::Type_Tuple(*rv);
}

/**
 * convert a P4-14 parser to P4-16 parser state.
 * @param parser     The P4-14 parser IR node to be converted
 * @param stateful   If any declaration is created during the conversion, save to 'stateful'
 * @returns          The P4-16 parser state corresponding to the P4-14 parser
 */
const IR::ParserState* ProgramStructure::convertParser(const IR::V1Parser* parser,
                                    IR::IndexedVector<IR::Declaration>* stateful) {
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
        std::vector<const IR::Type::Bits *> fieldTypes;
        for (auto e : *parser->select) {
            auto c = conv.convert(e);
            list->components.push_back(c);
            if (auto *t = c->type->to<IR::Type::Bits>()) {
                fieldTypes.push_back(t);
            } else {
                auto w = c->type->width_bits();
                BUG_CHECK(w > 0, "Unknown width for expression %1%", e);
                fieldTypes.push_back(IR::Type::Bits::get(w));
            }
        }
        BUG_CHECK(list->components.size() > 0, "No select expression in %1%", parser);
        // select always expects a ListExpression
        IR::Vector<IR::SelectCase> cases;
        for (auto c : *parser->cases) {
            IR::ID state = c->action;
            auto deststate = getState(state);
            if (deststate == nullptr)
                return nullptr;

            // discover all parser value_sets in the current parser state.
            for (auto v : c->values) {
                auto first = v.first->to<IR::PathExpression>();
                if (!first) continue;
                auto value_set = value_sets.get(first->path->name);
                if (!value_set) {
                    ::error("Unable to find declaration for parser_value_set %s",
                            first->path->name);
                    return nullptr;
                }

                auto type = explodeType(fieldTypes);
                auto sizeAnnotation = value_set->annotations->getSingle("parser_value_set_size");
                const IR::Constant* sizeConstant;
                if (sizeAnnotation) {
                    if (sizeAnnotation->expr.size() != 1) {
                        ::error("@size should be an integer for declaration %1%", value_set);
                        return nullptr;
                    }
                    sizeConstant = sizeAnnotation->expr[0]->to<IR::Constant>();
                    if (sizeConstant == nullptr || !sizeConstant->fitsInt()) {
                        ::error("@size should be an integer for declaration %1%", value_set);
                        return nullptr;
                    }
                } else {
                    ::warning(ErrorType::WARN_MISSING,
                              "%1%: parser_value_set has no @parser_value_set_size annotation."
                              "Using default size 4.", c);
                    sizeConstant = new IR::Constant(4);
                }
                auto annos = addGlobalNameAnnotation(value_set->name, value_set->annotations);
                auto decl = new IR::P4ValueSet(value_set->name, annos, type, sizeConstant);
                LOG1(decl);
                stateful->push_back(decl);
            }
            for (auto v : c->values) {
                if (auto first = v.first->to<IR::Constant>()) {
                    auto expr = explodeLabel(first, v.second, fieldTypes);
                    auto sc = new IR::SelectCase(c->srcInfo, expr, deststate);
                    cases.push_back(sc);
                } else {
                    auto sc = new IR::SelectCase(c->srcInfo, v.first, deststate);
                    cases.push_back(sc);
                }
            }
        }
        select = new IR::SelectExpression(parser->select->srcInfo, list, std::move(cases));
    } else if (!parser->default_return.name.isNullOrEmpty()) {
        IR::ID id = parser->default_return;
        select = getState(id);
        if (select == nullptr)
            return nullptr;
    } else {
        BUG("No select or default_return %1%", parser);
    }
    latest = nullptr;
    auto annos = addGlobalNameAnnotation(parser->name, parser->annotations);
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
        auto ps = convertParser(p.first, &stateful);
        if (ps == nullptr)
            return;
        states.push_back(ps);
    }

    if (states.empty())
        ::error("No parsers specified");
    auto result = new IR::P4Parser(v1model.parser.Id(), type, stateful, states);
    declarations->push_back(result);
    conversionContext.clear();

    if (ingressReference.name.isNullOrEmpty())
        ::error("No transition from a parser to ingress pipeline found");
}

void ProgramStructure::include(cstring filename, cstring ppoptions) {
    if (included_files.count(filename)) return;
    included_files.insert(filename);
    // the p4c driver sets environment variables for include
    // paths.  check the environment and add these to the command
    // line for the preporicessor
    char * drvP4IncludePath = getenv("P4C_16_INCLUDE_PATH");
    Util::PathName path(drvP4IncludePath ? drvP4IncludePath : p4includePath);
    path = path.join(filename);

    CompilerOptions options;
    if (ppoptions) {
        options.preprocessor_options += " ";
        options.preprocessor_options += ppoptions; }
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.file = path.toString();
    if (!::errorCount()) {
        if (FILE* file = options.preprocess()) {
            auto code = P4::P4ParserDriver::parse(file, options.file);
            if (code && !::errorCount())
                for (auto decl : code->objects)
                    declarations->push_back(decl);
            options.closeInput(file);
        }
    }
}

void ProgramStructure::loadModel() {
    // This includes in turn core.p4
    include("v1model.p4");
}

namespace {
// Must return a 'canonical' representation of each header.
// If a header appears twice this must return the SAME expression.
class HeaderRepresentation {
 private:
    const IR::Expression* hdrsParam;  // reference to headers parameter in deparser
    std::map<cstring, const IR::Expression*> header;
    /// This is for parsers that have no extracts
    std::map<cstring, const IR::Expression*> fakeHeader;
    /// One expression for each constant stack index
    std::map<const IR::Expression*, std::map<int, const IR::Expression*>> stackElement;
    /// One expression for each each stack[next]
    std::map<const IR::Expression*, const IR::Expression*> stackNextElement;

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
            if (auto pe = hsir->index_->to<IR::PathExpression>()) {
                if (pe->path->name.name == "next") {
                    if (stackNextElement.find(hdr) == stackNextElement.end())
                        stackNextElement[hdr] = new IR::Member(hdr, IR::ID("next"));
                    return stackNextElement[hdr];
                } else {
                    ::error("%1%: Illegal extract stack expression", pe);
                    return hdr;
                }
            }
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
                if (lastExtract != nullptr) {
                    if (!lastExtract->is<IR::Member>() || lastExtract != h)
                        // Do not add self-edges between h[next] and h[next]
                        headerOrder.calls(lastExtract, h);
                }
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
            if (!lastExtract->is<IR::Member>() || lastExtract != firstExtract)
                // Do not add self-edges between h[next] and h[next]
                // TODO: this is not 100% precise, since it ignores
                // edges h[next]->h[next] that go through other
                // headers.  We will still get warnings for these cases.
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
        ::warning(ErrorType::WARN_ORDERING,
                  "%1%: the order of headers in deparser is not uniquely determined by parser!",
                  startHeader);

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
        if (auto mem = exp->to<IR::Member>())
            if (mem->expr->is<IR::Member>())
                // This is a 'hdr.h.next'; use hdr.h instead
                exp = mem->expr;

        auto args = new IR::Vector<IR::Argument>();
        args->push_back(new IR::Argument(exp));
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

const IR::Declaration_Instance*
ProgramStructure::convertActionProfile(const IR::ActionProfile* action_profile, cstring newName) {
    auto *action_selector = action_selectors.get(action_profile->selector.name);
    if (!action_profile->selector.name.isNullOrEmpty() && !action_selector)
        ::error("Cannot locate action selector %1%", action_profile->selector);
    const IR::Type *type = nullptr;
    auto args = new IR::Vector<IR::Argument>();
    auto annos = addGlobalNameAnnotation(action_profile->name);
    if (action_selector) {
        type = new IR::Type_Name(new IR::Path(v1model.action_selector.Id()));
        auto flc = field_list_calculations.get(action_selector->key.name);
        auto algorithm = convertHashAlgorithms(flc->algorithm);
        if (algorithm == nullptr)
            return nullptr;
        args->push_back(new IR::Argument(algorithm));
        auto size = new IR::Constant(
            action_profile->srcInfo, v1model.action_selector.sizeType, action_profile->size);
        args->push_back(new IR::Argument(size));
        auto width = new IR::Constant(v1model.action_selector.widthType, flc->output_width);
        args->push_back(new IR::Argument(width));
        if (action_selector->mode)
            annos = annos->addAnnotation("mode", new IR::StringLiteral(action_selector->mode));
        if (action_selector->type)
            annos = annos->addAnnotation("type", new IR::StringLiteral(action_selector->type));
    } else {
        type = new IR::Type_Name(new IR::Path(v1model.action_profile.Id()));
        auto size = new IR::Constant(
            action_profile->srcInfo, v1model.action_profile.sizeType, action_profile->size);
        args->push_back(new IR::Argument(size)); }
    auto decl = new IR::Declaration_Instance(newName, annos, type, args, nullptr);
    return decl;
}

const IR::P4Table*
ProgramStructure::convertTable(const IR::V1Table* table, cstring newName,
                               IR::IndexedVector<IR::Declaration> &stateful,
                               std::map<cstring, cstring> &mapNames) {
    ExpressionConverter conv(this);
    auto props = new IR::TableProperties(table->properties.properties);
    auto actionList = new IR::ActionList({});

    cstring profile = table->action_profile.name;
    auto *action_profile = action_profiles.get(profile);
    auto *action_selector = action_profile ? action_selectors.get(action_profile->selector.name)
                                           : nullptr;
    auto mtr = get(directMeters, table->name);
    auto ctr = get(directCounters, table->name);
    const std::vector<IR::ID> &actionsToDo =
            action_profile ? action_profile->actions : table->actions;
    cstring default_action = table->default_action;
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
        if (table->default_action.name == action->name.name) {
            default_action = newname;
        }
    }
    if (!table->default_action.name.isNullOrEmpty() &&
        !actionList->getDeclaration(default_action)) {
        actionList->push_back(
            new IR::ActionListElement(
                new IR::Annotations(
                    {new IR::Annotation(IR::Annotation::defaultOnlyAnnotation, {})}),
                new IR::PathExpression(default_action))); }
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

            const IR::Annotations* annos = IR::Annotations::empty;
            // Generate a name annotation for the key if it contains a mask.
            if (auto bin = e->to<IR::Mask>()) {
                // This is a bit heuristic, but P4-14 does not allow arbitrary expressions for keys
                cstring name;
                if (bin->left->is<IR::Constant>())
                    name = bin->right->toString();
                else if (bin->right->is<IR::Constant>())
                    name = bin->left->toString();
                if (!name.isNullOrEmpty())
                    annos = addNameAnnotation(name, annos);
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

    if (!table->default_action.name.isNullOrEmpty()) {
        auto act = new IR::PathExpression(default_action);
        auto args = new IR::Vector<IR::Argument>();
        if (table->default_action_args != nullptr) {
            for (auto e : *table->default_action_args) {
                auto arg = new IR::Argument(e);
                args->push_back(arg);
            }
        } else {
            // Since no default action arguments are added in the p4 program we
            // add zero initialized arguments
            for (auto a : actions) {
                if (a.second == table->default_action.name) {
                    for (auto aa : a.first->args) {
                        args->push_back(new IR::Argument(aa->name, new IR::Constant(0)));
                    }
                    break;
                }
            }
        }
        auto methodCall = new IR::MethodCallExpression(act, args);
        auto prop = new IR::Property(
            IR::ID(IR::TableProperties::defaultActionPropertyName),
            new IR::ExpressionValue(methodCall),
            table->default_action_is_const);
        props->push_back(prop);
    }

    if (action_profile != nullptr) {
        auto sel = new IR::PathExpression(action_profiles.get(action_profile));
        auto propvalue = new IR::ExpressionValue(sel);
        auto prop = new IR::Property(
            IR::ID(v1model.tableAttributes.tableImplementation.Id()),
            propvalue, false);
        props->push_back(prop); }

    if (!ctr.isNullOrEmpty()) {
        auto counter = counters.get(ctr);
        cstring name = counters.get(counter);
        auto path = new IR::PathExpression(name);
        auto propvalue = new IR::ExpressionValue(path);
        auto prop = new IR::Property(
            IR::ID(v1model.tableAttributes.counters.Id()),
            IR::Annotations::empty, propvalue, false);
        props->push_back(prop);
    }

    if (mtr != nullptr) {
        auto meter = new IR::PathExpression(mtr->name);
        auto propvalue = new IR::ExpressionValue(meter);
        auto prop = new IR::Property(
            IR::ID(v1model.tableAttributes.meters.Id()), propvalue, false);
        props->push_back(prop);
    }

    auto annos = addGlobalNameAnnotation(table->name, table->annotations);
    auto result = new IR::P4Table(table->srcInfo, newName, annos, props);
    return result;
}

const IR::Expression* ProgramStructure::convertHashAlgorithm(
    Util::SourceInfo srcInfo, IR::ID algorithm) {
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
    } else if (algorithm == "csum16") {
        result = v1model.algorithm.csum16.Id();
    } else if (algorithm == "xor16") {
        result = v1model.algorithm.xor16.Id();
    } else {
        ::warning(ErrorType::WARN_UNSUPPORTED, "%1%: unexpected algorithm", algorithm);
        result = algorithm;
    }
    auto pe = new IR::TypeNameExpression(v1model.algorithm.Id());
    auto mem = new IR::Member(srcInfo, pe, result);
    return mem;
}

const IR::Expression* ProgramStructure::convertHashAlgorithms(const IR::NameList *algorithm) {
    if (!algorithm || algorithm->names.empty()) return nullptr;
    if (algorithm->names.size() > 1) {
#if 1
        ::warning(ErrorType::WARN_UNSUPPORTED,
                  "%s: Multiple algorithms in a field list not supported in P4_16 -- using "
                  "only the first", algorithm->names[0].srcInfo);
#else
        auto rv = new IR::ListExpression({});
        for (auto &alg : algorithm->names)
            rv->push_back(convertHashAlgorithm(alg));
        return rv;
#endif
    }
    return convertHashAlgorithm(algorithm->srcInfo, algorithm->names[0]);
}

static bool sameBitsType(
    const IR::Node* errorPosition, const IR::Type* left, const IR::Type* right) {
    if (typeid(*left) != typeid(*right))
        return false;
    if (left->is<IR::Type_Bits>())
        return left->to<IR::Type_Bits>()->operator==(* right->to<IR::Type_Bits>());
    ::error("%1%: operation only defined for bit/int types", errorPosition);
    return true;  // to prevent inserting a cast
}

static bool isSaturatedField(const IR::Expression *expr) {
    auto member = expr->to<IR::Member>();
    if (!member)
        return false;
    auto header_type = dynamic_cast<const IR::Type_StructLike *>(member->expr->type);
    if (!header_type)
        return false;
    auto field = header_type->getField(member->member.name);
    if (field && field->getAnnotation("saturating")) {
        return true;
    }
    return false;
}

// Implement modify_field(left, right, mask)
const IR::Statement* ProgramStructure::sliceAssign(
    const IR::Primitive* primitive, const IR::Expression* left,
    const IR::Expression* right, const IR::Expression* mask) {
    if (mask->is<IR::Constant>()) {
        auto cst = mask->to<IR::Constant>();
        if (cst->value < 0) {
            ::error("%1%: Negative mask not supported", mask);
            return nullptr;
        }
        if (cst->value != 0) {
            auto range = Util::findOnes(cst->value);
            if (cst->value == range.value) {
                auto h = new IR::Constant(range.highIndex);
                auto l = new IR::Constant(range.lowIndex);
                left = new IR::Slice(left->srcInfo, left, h, l);
                right = new IR::Slice(right->srcInfo, right, h, l);
                return assign(primitive->srcInfo, left, right, nullptr);
            }
        }
        // else value is too complex for a slice
    }

    auto type = left->type;
    if (!sameBitsType(primitive, mask->type, left->type))
        mask = new IR::Cast(type, mask);
    if (!sameBitsType(primitive, right->type, left->type))
        right = new IR::Cast(type, right);
    auto op1 = new IR::BAnd(left->srcInfo, left, new IR::Cmpl(mask));
    auto op2 = new IR::BAnd(right->srcInfo, right, mask);
    auto result = new IR::BOr(mask->srcInfo, op1, op2);
    return assign(primitive->srcInfo, left, result, type);
}

const IR::Expression* ProgramStructure::convertFieldList(const IR::Expression* expression) {
    ExpressionConverter conv(this);

    if (!expression->is<IR::PathExpression>()) {
      ::error("%1%: expected a field list", expression);
      return expression;
    }
    auto nr = expression->to<IR::PathExpression>();
    auto fl = field_lists.get(nr->path->name);
    if (fl == nullptr) {
      ::error("%1%: Expected a field list", expression);
      return expression;
    }
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

    if (extrn)
        return ExternConverter::cvtExternCall(this, extrn, primitive);
    if (auto *st = PrimitiveConverter::cvtPrimitive(this, primitive))
        return st;
    // If everything else failed maybe we are invoking an action
    auto action = actions.get(primitive->name);
    if (action != nullptr) {
        auto newname = actions.get(action);
        auto act = new IR::PathExpression(newname);
        auto args = new IR::Vector<IR::Argument>();
        for (auto a : primitive->operands) {
            auto e = conv.convert(a);
            args->push_back(new IR::Argument(e));
        }
        auto call = new IR::MethodCallExpression(primitive->srcInfo, act, args);
        auto stat = new IR::MethodCallStatement(primitive->srcInfo, call);
        return stat;
    }
    error("Unsupported primitive %1%", primitive);
    return nullptr;
}

#define OPS_CK(primitive, n) BUG_CHECK((primitive)->operands.size() == n, \
                                       "Expected " #n " operands for %1%", primitive)

CONVERT_PRIMITIVE(modify_field) {
    auto args = convertArgs(structure, primitive);
    if (args.size() == 2) {
        return structure->assign(primitive->srcInfo, args[0], args[1],
                                 primitive->operands.at(0)->type);
    } else if (args.size() == 3) {
        return structure->sliceAssign(primitive, args[0], args[1], args[2]); }
    return nullptr;
}

CONVERT_PRIMITIVE(bit_xor) {
    OPS_CK(primitive, 3);
    auto args = convertArgs(structure, primitive);
    if (!sameBitsType(primitive, args[1]->type, args[2]->type))
        args[2] = new IR::Cast(args[2]->srcInfo, args[1]->type, args[2]);
    auto op = new IR::BXor(primitive->srcInfo, args[1], args[2]);
    return structure->assign(primitive->srcInfo, args[0], op, primitive->operands.at(0)->type);
}

CONVERT_PRIMITIVE(min) {
    OPS_CK(primitive, 3);
    auto args = convertArgs(structure, primitive);
    if (!sameBitsType(primitive, args[1]->type, args[2]->type))
        args[2] = new IR::Cast(args[2]->srcInfo, args[1]->type, args[2]);
    auto op = new IR::Mux(primitive->srcInfo,
                          new IR::Leq(primitive->srcInfo, args[1], args[2]), args[1], args[2]);
    return structure->assign(primitive->srcInfo, args[0], op, primitive->operands.at(0)->type);
}

CONVERT_PRIMITIVE(max) {
    OPS_CK(primitive, 3);
    auto args = convertArgs(structure, primitive);
    if (!sameBitsType(primitive, args[1]->type, args[2]->type))
        args[2] = new IR::Cast(args[2]->srcInfo, args[1]->type, args[2]);
    auto op = new IR::Mux(primitive->srcInfo,
                          new IR::Geq(primitive->srcInfo, args[1], args[2]), args[1], args[2]);
    return structure->assign(primitive->srcInfo, args[0], op, primitive->operands.at(0)->type);
}

CONVERT_PRIMITIVE(bit_or) {
    OPS_CK(primitive, 3);
    auto args = convertArgs(structure, primitive);
    if (!sameBitsType(primitive, args[1]->type, args[2]->type))
        args[2] = new IR::Cast(args[2]->srcInfo, args[1]->type, args[2]);
    auto op = new IR::BOr(primitive->srcInfo, args[1], args[2]);
    return structure->assign(primitive->srcInfo, args[0], op, primitive->operands.at(0)->type);
}

CONVERT_PRIMITIVE(bit_xnor) {
    OPS_CK(primitive, 3);
    auto args = convertArgs(structure, primitive);
    if (!sameBitsType(primitive, args[1]->type, args[2]->type))
        args[2] = new IR::Cast(args[2]->srcInfo, args[1]->type, args[2]);
    auto op = new IR::Cmpl(primitive->srcInfo, new IR::BXor(primitive->srcInfo, args[1], args[2]));
    return structure->assign(primitive->srcInfo, args[0], op, primitive->operands.at(0)->type);
}

CONVERT_PRIMITIVE(add) {
    OPS_CK(primitive, 3);
    auto args = convertArgs(structure, primitive);
    bool isSaturated = false;
    for (auto a : args)
        isSaturated = isSaturated || isSaturatedField(a);  // any of the fields
    if (!sameBitsType(primitive, args[1]->type, args[2]->type))
        args[2] = new IR::Cast(args[2]->srcInfo, args[1]->type, args[2]);
    IR::Operation_Binary *op = nullptr;
    if (isSaturated)
        op = new IR::AddSat(primitive->srcInfo, args[1], args[2]);
    else
        op = new IR::Add(primitive->srcInfo, args[1], args[2]);
    LOG3("add: isSaturated " << isSaturated << ", op: " << op);
    return structure->assign(primitive->srcInfo, args[0], op, primitive->operands.at(0)->type);
}

CONVERT_PRIMITIVE(bit_nor) {
    OPS_CK(primitive, 3);
    auto args = convertArgs(structure, primitive);
    if (!sameBitsType(primitive, args[1]->type, args[2]->type))
        args[2] = new IR::Cast(args[2]->srcInfo, args[1]->type, args[2]);
    auto op = new IR::Cmpl(primitive->srcInfo, new IR::BOr(primitive->srcInfo, args[1], args[2]));
    return structure->assign(primitive->srcInfo, args[0], op, primitive->operands.at(0)->type);
}

CONVERT_PRIMITIVE(bit_nand) {
    OPS_CK(primitive, 3);
    auto args = convertArgs(structure, primitive);
    if (!sameBitsType(primitive, args[1]->type, args[2]->type))
        args[2] = new IR::Cast(args[2]->srcInfo, args[1]->type, args[2]);
    auto op = new IR::Cmpl(primitive->srcInfo, new IR::BAnd(primitive->srcInfo, args[1], args[2]));
    return structure->assign(primitive->srcInfo, args[0], op, primitive->operands.at(0)->type);
}

CONVERT_PRIMITIVE(bit_orca) {
    OPS_CK(primitive, 3);
    auto args = convertArgs(structure, primitive);
    if (!sameBitsType(primitive, args[1]->type, args[2]->type))
        args[2] = new IR::Cast(args[2]->srcInfo, args[1]->type, args[2]);
    auto op = new IR::BOr(primitive->srcInfo, new IR::Cmpl(primitive->srcInfo, args[1]), args[2]);
    return structure->assign(primitive->srcInfo, args[0], op, primitive->operands.at(0)->type);
}

CONVERT_PRIMITIVE(bit_orcb) {
    OPS_CK(primitive, 3);
    auto args = convertArgs(structure, primitive);
    if (!sameBitsType(primitive, args[1]->type, args[2]->type))
        args[2] = new IR::Cast(args[2]->srcInfo, args[1]->type, args[2]);
    auto op = new IR::BOr(primitive->srcInfo, args[1], new IR::Cmpl(primitive->srcInfo, args[2]));
    return structure->assign(primitive->srcInfo, args[0], op, primitive->operands.at(0)->type);
}

CONVERT_PRIMITIVE(bit_andca) {
    OPS_CK(primitive, 3);
    auto args = convertArgs(structure, primitive);
    if (!sameBitsType(primitive, args[1]->type, args[2]->type))
        args[2] = new IR::Cast(args[2]->srcInfo, args[1]->type, args[2]);
    auto op = new IR::BAnd(primitive->srcInfo, new IR::Cmpl(primitive->srcInfo, args[1]), args[2]);
    return structure->assign(primitive->srcInfo, args[0], op, primitive->operands.at(0)->type);
}

CONVERT_PRIMITIVE(bit_andcb) {
    OPS_CK(primitive, 3);
    auto args = convertArgs(structure, primitive);
    if (!sameBitsType(primitive, args[1]->type, args[2]->type))
        args[2] = new IR::Cast(args[2]->srcInfo, args[1]->type, args[2]);
    auto op = new IR::BAnd(primitive->srcInfo, args[1], new IR::Cmpl(primitive->srcInfo, args[2]));
    return structure->assign(primitive->srcInfo, args[0], op, primitive->operands.at(0)->type);
}

CONVERT_PRIMITIVE(bit_and) {
    OPS_CK(primitive, 3);
    auto args = convertArgs(structure, primitive);
    if (!sameBitsType(primitive, args[1]->type, args[2]->type))
        args[2] = new IR::Cast(args[2]->srcInfo, args[1]->type, args[2]);
    auto op = new IR::BAnd(primitive->srcInfo, args[1], args[2]);
    return structure->assign(primitive->srcInfo, args[0], op, primitive->operands.at(0)->type);
}

CONVERT_PRIMITIVE(subtract) {
    OPS_CK(primitive, 3);
    auto args = convertArgs(structure, primitive);
    bool isSaturated = false;
    for (auto a : args)
        isSaturated = isSaturated || isSaturatedField(a);
    if (!sameBitsType(primitive, args[1]->type, args[2]->type))
        args[2] = new IR::Cast(args[2]->srcInfo, args[1]->type, args[2]);
    IR::Operation_Binary *op = nullptr;
    if (isSaturated)
        op = new IR::SubSat(primitive->srcInfo, args[1], args[2]);
    else
        op = new IR::Sub(primitive->srcInfo, args[1], args[2]);
    LOG3("subtract: isSaturated " << isSaturated << ", op: " << op);
    return structure->assign(primitive->srcInfo, args[0], op, primitive->operands.at(0)->type);
}

CONVERT_PRIMITIVE(shift_left) {
    OPS_CK(primitive, 3);
    auto args = convertArgs(structure, primitive);
    auto op = new IR::Shl(primitive->srcInfo, args[1], args[2]);
    return structure->assign(primitive->srcInfo, args[0], op, primitive->operands.at(0)->type);
}

CONVERT_PRIMITIVE(shift_right) {
    OPS_CK(primitive, 3);
    auto args = convertArgs(structure, primitive);
    auto op = new IR::Shr(primitive->srcInfo, args[1], args[2]);
    return structure->assign(primitive->srcInfo, args[0], op, primitive->operands.at(0)->type);
}

CONVERT_PRIMITIVE(bit_not) {
    OPS_CK(primitive, 2);
    auto args = convertArgs(structure, primitive);
    auto op = new IR::Cmpl(primitive->srcInfo, args[1]);
    return structure->assign(primitive->srcInfo, args[0], op, primitive->operands.at(0)->type);
}

CONVERT_PRIMITIVE(no_op) {
    (void)structure;
    (void)primitive;
    return new IR::EmptyStatement();
}

CONVERT_PRIMITIVE(add_to_field) {
    ExpressionConverter conv(structure);
    OPS_CK(primitive, 2);
    auto left = conv.convert(primitive->operands.at(0));
    auto left2 = conv.convert(primitive->operands.at(0));
    // convert twice, so we have different expression trees on RHS and LHS
    auto right = primitive->operands.at(1);
    bool isSaturated = isSaturatedField(left) || isSaturatedField(right);
    IR::Operation_Binary *op = nullptr;
    if (auto k = primitive->operands.at(1)->to<IR::Constant>()) {
        if (k->value < 0 && !k->type->to<IR::Type::Bits>()->isSigned) {
            // adding a negative value to an unsigned field -- want to turn this into a
            // subtract to avoid the warning about 'negative value with unsigned type'
            right = new IR::Constant(k->srcInfo, k->type, -k->value);
            if (isSaturated)
                op = new IR::SubSat(primitive->srcInfo, left, right);
            else
                op = new IR::Sub(primitive->srcInfo, left, right); } }
    if (!op) {
        right = conv.convert(right);
        if (isSaturated)
            op = new IR::AddSat(primitive->srcInfo, left, right);
        else
            op = new IR::Add(primitive->srcInfo, left, right); }
    LOG3("add_to_field: isSaturated " << isSaturated << ", op: " << op);
    return structure->assign(primitive->srcInfo, left2, op, primitive->operands.at(0)->type);
}

CONVERT_PRIMITIVE(subtract_from_field) {
    ExpressionConverter conv(structure);
    OPS_CK(primitive, 2);
    auto left = conv.convert(primitive->operands.at(0));
    auto left2 = conv.convert(primitive->operands.at(0));
    // convert twice, so we have different expression trees on RHS and LHS
    auto right = conv.convert(primitive->operands.at(1));
    bool isSaturated = isSaturatedField(left) || isSaturatedField(right);
    IR::Operation_Binary *op = nullptr;
    if (isSaturated)
        op = new IR::SubSat(primitive->srcInfo, left, right);
    else
        op = new IR::Sub(primitive->srcInfo, left, right);
    LOG3("subtract_form_field: isSaturated " << isSaturated << ", op: " << op);
    return structure->assign(primitive->srcInfo, left2, op, primitive->operands.at(0)->type);
}

CONVERT_PRIMITIVE(remove_header) {
    ExpressionConverter conv(structure);
    OPS_CK(primitive, 1);
    auto hdr = conv.convert(primitive->operands.at(0));
    auto method = new IR::Member(hdr, IR::ID(IR::Type_Header::setInvalid));
    return new IR::MethodCallStatement(primitive->srcInfo, method, {});
}

CONVERT_PRIMITIVE(add_header) {
    ExpressionConverter conv(structure);
    OPS_CK(primitive, 1);
    auto hdr = conv.convert(primitive->operands.at(0));
    auto method = new IR::Member(hdr, IR::ID(IR::Type_Header::setValid));
    return new IR::MethodCallStatement(primitive->srcInfo, method, {});
}

CONVERT_PRIMITIVE(copy_header) {
    ExpressionConverter conv(structure);
    OPS_CK(primitive, 2);
    auto left = conv.convert(primitive->operands.at(0));
    auto right = conv.convert(primitive->operands.at(1));
    return new IR::AssignmentStatement(primitive->srcInfo, left, right);
}

CONVERT_PRIMITIVE(drop) {
    return new IR::MethodCallStatement(
        primitive->srcInfo, structure->v1model.drop.Id(), {});
}

CONVERT_PRIMITIVE(mark_for_drop) {
    return new IR::MethodCallStatement(
        primitive->srcInfo, structure->v1model.drop.Id(), {});
}

static const IR::Constant *push_pop_size(ExpressionConverter &conv, const IR::Primitive *prim) {
    if (prim->operands.size() == 1)
        return new IR::Constant(1);
    auto op1 = prim->operands.at(1);
    auto count = conv.convert(op1);
    if (!count->is<IR::Constant>()) {
        ::error("%1%: Only %2% with a constant value is supported", op1, prim->name);
        return new IR::Constant(1); }
    auto cst = count->to<IR::Constant>();
    auto number = cst->asInt();
    if (number < 0) {
        ::error("%1%: %2% requires a positive amount", op1, prim->name);
        return new IR::Constant(1); }
    if (number > 0xFFFF) {
        ::error("%1%: %2% amount is too large", op1, prim->name);
        return new IR::Constant(1); }
    return cst;
}
CONVERT_PRIMITIVE(push) {
    ExpressionConverter conv(structure);
    BUG_CHECK(primitive->operands.size() >= 1 || primitive->operands.size() <= 2,
              "Expected 1 or 2 operands for %1%", primitive);
    auto hdr = conv.convert(primitive->operands.at(0));
    auto count = push_pop_size(conv, primitive);
    IR::IndexedVector<IR::StatOrDecl> block;
    auto methodName = IR::Type_Stack::push_front;
    auto method = new IR::Member(hdr, IR::ID(methodName));
    block.push_back(new IR::MethodCallStatement(primitive->srcInfo, method,
                                                { new IR::Argument(count) }));
    for (int i = 0; i < count->asInt(); i++) {
        auto elemi = new IR::ArrayIndex(primitive->srcInfo, hdr, new IR::Constant(i));
        auto setValid = new IR::Member(elemi, IR::ID(IR::Type_Header::setValid));
        block.push_back(new IR::MethodCallStatement(primitive->srcInfo, setValid, {}));
    }
    return new IR::BlockStatement(primitive->srcInfo, std::move(block));
}
CONVERT_PRIMITIVE(pop) {
    ExpressionConverter conv(structure);
    BUG_CHECK(primitive->operands.size() >= 1 || primitive->operands.size() <= 2,
              "Expected 1 or 2 operands for %1%", primitive);
    auto hdr = conv.convert(primitive->operands.at(0));
    auto count = push_pop_size(conv, primitive);
    auto methodName = IR::Type_Stack::pop_front;
    auto method = new IR::Member(hdr, IR::ID(methodName));
    return new IR::MethodCallStatement(primitive->srcInfo, method,
                                       { new IR::Argument(count) });
}

CONVERT_PRIMITIVE(count) {
    ExpressionConverter conv(structure);
    OPS_CK(primitive, 2);
    auto ref = primitive->operands.at(0);
    const IR::Counter *counter = nullptr;
    if (auto gr = ref->to<IR::GlobalRef>())
        counter = gr->obj->to<IR::Counter>();
    else if (auto nr = ref->to<IR::PathExpression>())
        counter = structure->counters.get(nr->path->name);
    if (counter == nullptr) {
        ::error("Expected a counter reference %1%", ref);
        return nullptr; }
    auto newname = structure->counters.get(counter);
    auto counterref = new IR::PathExpression(newname);
    auto methodName = structure->v1model.counter.increment.Id();
    auto method = new IR::Member(counterref, methodName);
    auto arg = new IR::Cast(structure->v1model.counter.index_type,
                            conv.convert(primitive->operands.at(1)));
    return new IR::MethodCallStatement(
        primitive->srcInfo, method, { new IR::Argument(arg) });
}

CONVERT_PRIMITIVE(modify_field_from_rng) {
    ExpressionConverter conv(structure);
    BUG_CHECK(primitive->operands.size() == 2 || primitive->operands.size() == 3,
              "Expected 2 or 3 operands for %1%", primitive);
    auto field = conv.convert(primitive->operands.at(0));
    auto dest = field;
    auto logRange = conv.convert(primitive->operands.at(1));
    const IR::Expression* mask = nullptr;
    auto block = new IR::BlockStatement;
    if (primitive->operands.size() == 3) {
        mask = conv.convert(primitive->operands.at(2));
        cstring tmpvar = structure->makeUniqueName("tmp");
        auto decl = new IR::Declaration_Variable(tmpvar, field->type);
        block->push_back(decl);
        dest = new IR::PathExpression(field->type, new IR::Path(tmpvar)); }
    auto args = new IR::Vector<IR::Argument>();
    args->push_back(new IR::Argument(dest));
    args->push_back(
        new IR::Argument(new IR::Constant(primitive->operands.at(1)->srcInfo, field->type, 0)));
    auto one = new IR::Constant(primitive->operands.at(1)->srcInfo, 1);
    args->push_back(
        new IR::Argument(new IR::Cast(primitive->operands.at(1)->srcInfo, field->type,
                                      new IR::Sub(new IR::Shl(one, logRange), one))));
    auto random = new IR::PathExpression(structure->v1model.random.Id());
    auto mc = new IR::MethodCallExpression(primitive->srcInfo, random, args);
    auto call = new IR::MethodCallStatement(primitive->srcInfo, mc);
    block->push_back(call);
    if (mask != nullptr) {
        auto assgn = structure->sliceAssign(primitive, field, dest->clone(), mask);
        block->push_back(assgn); }
    return block;
}

CONVERT_PRIMITIVE(modify_field_rng_uniform) {
    ExpressionConverter conv(structure);
    BUG_CHECK(primitive->operands.size() == 3, "Expected 3 operands for %1%", primitive);
    auto field = conv.convert(primitive->operands.at(0));
    auto lo = conv.convert(primitive->operands.at(1));
    auto hi = conv.convert(primitive->operands.at(2));
    if (lo->type != field->type)
        lo = new IR::Cast(primitive->operands.at(1)->srcInfo, field->type, lo);
    if (hi->type != field->type)
        hi = new IR::Cast(primitive->operands.at(2)->srcInfo, field->type, hi);
    auto random = new IR::PathExpression(structure->v1model.random.Id());
    auto mc = new IR::MethodCallExpression(
        primitive->srcInfo, random, {
            new IR::Argument(field), new IR::Argument(lo), new IR::Argument(hi) });
    auto call = new IR::MethodCallStatement(primitive->srcInfo, mc);
    return call;
}

CONVERT_PRIMITIVE(recirculate) {
    ExpressionConverter conv(structure);
    OPS_CK(primitive, 1);
    auto right = structure->convertFieldList(primitive->operands.at(0));
    if (right == nullptr)
        return nullptr;
    auto args = new IR::Vector<IR::Argument>();
    args->push_back(new IR::Argument(right));
    auto path = new IR::PathExpression(structure->v1model.recirculate.Id());
    auto mc = new IR::MethodCallExpression(primitive->srcInfo, path, args);
    return new IR::MethodCallStatement(mc->srcInfo, mc);
}

static const IR::Statement *
convertClone(ProgramStructure *structure, const IR::Primitive *primitive, Model::Elem kind) {
    BUG_CHECK(primitive->operands.size() == 1 || primitive->operands.size() == 2,
              "Expected 1 or 2 operands for %1%", primitive);
    ExpressionConverter conv(structure);
    auto session = conv.convert(primitive->operands.at(0));

    auto args = new IR::Vector<IR::Argument>();
    auto enumref = new IR::TypeNameExpression(
        new IR::Type_Name(new IR::Path(structure->v1model.clone.cloneType.Id())));
    auto kindarg = new IR::Member(enumref, kind.Id());
    args->push_back(new IR::Argument(kindarg));
    args->push_back(
        new IR::Argument(new IR::Cast(primitive->operands.at(0)->srcInfo,
                                      structure->v1model.clone.sessionType, session)));
    if (primitive->operands.size() == 2) {
        auto list = structure->convertFieldList(primitive->operands.at(1));
        if (list != nullptr)
            args->push_back(new IR::Argument(list));
    }

    auto id = primitive->operands.size() == 2 ? structure->v1model.clone.clone3.Id()
                                               : structure->v1model.clone.Id();
    auto clone = new IR::PathExpression(id);
    auto mc = new IR::MethodCallExpression(primitive->srcInfo, clone, args);
    return new IR::MethodCallStatement(mc->srcInfo, mc);
}

CONVERT_PRIMITIVE(clone_egress_pkt_to_egress) {
    return convertClone(structure, primitive, structure->v1model.clone.cloneType.e2e); }
CONVERT_PRIMITIVE(clone_e2e) {
    return convertClone(structure, primitive, structure->v1model.clone.cloneType.e2e); }
CONVERT_PRIMITIVE(clone_ingress_pkt_to_egress) {
    return convertClone(structure, primitive, structure->v1model.clone.cloneType.i2e); }
CONVERT_PRIMITIVE(clone_i2e) {
    return convertClone(structure, primitive, structure->v1model.clone.cloneType.i2e); }

CONVERT_PRIMITIVE(resubmit) {
    ExpressionConverter conv(structure);
    BUG_CHECK(primitive->operands.size() <= 1, "Expected 0 or 1 operands for %1%", primitive);
    const IR::Expression *list;
    if (primitive->operands.size() > 0)
        list = structure->convertFieldList(primitive->operands.at(0));
    else
        list = new IR::ListExpression({});
    return new IR::MethodCallStatement(primitive->srcInfo, structure->v1model.resubmit.Id(),
                                       { new IR::Argument(list) });
}

CONVERT_PRIMITIVE(execute_meter) {
    ExpressionConverter conv(structure);
    OPS_CK(primitive, 3);
    auto ref = primitive->operands.at(0);
    const IR::Meter *meter = nullptr;
    if (auto gr = ref->to<IR::GlobalRef>())
        meter = gr->obj->to<IR::Meter>();
    else if (auto nr = ref->to<IR::PathExpression>())
        meter = structure->meters.get(nr->path->name);
    if (!meter) {
        ::error("Expected a meter reference %1%", ref);
        return nullptr; }
    if (!meter->implementation.name.isNullOrEmpty())
        ::warning(ErrorType::WARN_IGNORE_PROPERTY, "Ignoring `implementation' field of meter %1%",
                  meter);
    auto newname = structure->meters.get(meter);
    auto meterref = new IR::PathExpression(newname);
    auto methodName = structure->v1model.meter.executeMeter.Id();
    auto method = new IR::Member(meterref, methodName);
    auto args = new IR::Vector<IR::Argument>();
    auto arg = new IR::Cast(structure->v1model.meter.index_type,
                            conv.convert(primitive->operands.at(1)));
    args->push_back(new IR::Argument(arg));
    auto dest = conv.convert(primitive->operands.at(2));
    args->push_back(new IR::Argument(dest));
    auto mc = new IR::MethodCallExpression(primitive->srcInfo, method, args);
    return new IR::MethodCallStatement(primitive->srcInfo, mc);
}

CONVERT_PRIMITIVE(modify_field_with_hash_based_offset) {
    ExpressionConverter conv(structure);
    OPS_CK(primitive, 4);

    auto dest = conv.convert(primitive->operands.at(0));
    auto base = conv.convert(primitive->operands.at(1));
    auto max = conv.convert(primitive->operands.at(3));
    auto args = new IR::Vector<IR::Argument>();

    auto flc = structure->getFieldListCalculation(primitive->operands.at(2));
    if (flc == nullptr) {
        ::error("%1%: Expected a field_list_calculation", primitive->operands.at(2));
        return nullptr;
    }
    auto ttype = IR::Type_Bits::get(flc->output_width);
    auto fl = structure->getFieldLists(flc);
    if (fl == nullptr)
        return nullptr;
    auto list = conv.convert(fl);

    auto algorithm = structure->convertHashAlgorithms(flc->algorithm);
    args->push_back(new IR::Argument(dest));
    args->push_back(new IR::Argument(algorithm));
    args->push_back(new IR::Argument(new IR::Cast(ttype, base)));
    args->push_back(new IR::Argument(list));
    args->push_back(new IR::Argument(
        new IR::Cast(max->srcInfo, IR::Type_Bits::get(2 * flc->output_width), max)));
    auto hash = new IR::PathExpression(structure->v1model.hash.Id());
    auto mc = new IR::MethodCallExpression(primitive->srcInfo, hash, args);
    auto result = new IR::MethodCallStatement(primitive->srcInfo, mc);
    return result;
}

CONVERT_PRIMITIVE(modify_field_conditionally) {
    ExpressionConverter conv(structure);
    OPS_CK(primitive, 3);
    auto dest = conv.convert(primitive->operands.at(0));
    auto cond = conv.convert(primitive->operands.at(1));
    auto src = conv.convert(primitive->operands.at(2));
    if (!cond->type->is<IR::Type::Boolean>())
        cond = new IR::Neq(cond, new IR::Constant(0));
    src = new IR::Mux(primitive->srcInfo, cond, src, dest);
    return structure->assign(primitive->srcInfo, dest, src, primitive->operands.at(0)->type);
}

CONVERT_PRIMITIVE(modify_field_with_shift) {
    ExpressionConverter conv(structure);
    OPS_CK(primitive, 4);
    auto dest = conv.convert(primitive->operands.at(0));
    auto src = conv.convert(primitive->operands.at(1));
    auto shift = conv.convert(primitive->operands.at(2));
    auto mask = conv.convert(primitive->operands.at(3));
    auto result = structure->sliceAssign(primitive, dest, new IR::Shr(src, shift), mask);
    return result;
}

CONVERT_PRIMITIVE(generate_digest) {
    ExpressionConverter conv(structure);
    OPS_CK(primitive, 2);
    auto args = new IR::Vector<IR::Argument>();
    auto receiver = conv.convert(primitive->operands.at(0));
    args->push_back(
        new IR::Argument(new IR::Cast(primitive->operands.at(0)->srcInfo,
                                      structure->v1model.digest_receiver.receiverType, receiver)));
    auto list = structure->convertFieldList(primitive->operands.at(1));
    auto type = structure->createFieldListType(primitive->operands.at(1));
    // In P4 v1.0 programs we can have declarations out of order
    structure->declarations->push_back(type);
    if (list != nullptr)
        args->push_back(new IR::Argument(list));
    auto typeArgs = new IR::Vector<IR::Type>();
    auto typeName = new IR::Type_Name(new IR::Path(type->name));
    typeArgs->push_back(typeName);
    auto random = new IR::PathExpression(structure->v1model.digest_receiver.Id());
    auto mc = new IR::MethodCallExpression(primitive->srcInfo, random, typeArgs, args);
    auto result = new IR::MethodCallStatement(mc->srcInfo, mc);
    return result;
}

CONVERT_PRIMITIVE(register_read) {
    ExpressionConverter conv(structure);
    OPS_CK(primitive, 3);
    auto left = conv.convert(primitive->operands.at(0));
    auto ref = primitive->operands.at(1);
    const IR::Register *reg = nullptr;
    if (auto gr = ref->to<IR::GlobalRef>())
        reg = gr->obj->to<IR::Register>();
    else if (auto nr = ref->to<IR::PathExpression>())
        reg = structure->registers.get(nr->path->name);
    if (!reg) {
        ::error("Expected a register reference %1%", ref);
        return nullptr; }
    auto newname = structure->registers.get(reg);
    auto registerref = new IR::PathExpression(newname);
    auto methodName = structure->v1model.registers.read.Id();
    auto method = new IR::Member(registerref, methodName);
    auto args = new IR::Vector<IR::Argument>();
    auto arg = new IR::Cast(structure->v1model.registers.index_type,
                            conv.convert(primitive->operands.at(2)));
    args->push_back(new IR::Argument(left));
    args->push_back(new IR::Argument(arg));
    auto mc = new IR::MethodCallExpression(primitive->srcInfo, method, args);
    return new IR::MethodCallStatement(mc->srcInfo, mc);
}

CONVERT_PRIMITIVE(register_write) {
    ExpressionConverter conv(structure);
    OPS_CK(primitive, 3);
    auto ref = primitive->operands.at(0);
    const IR::Register *reg = nullptr;
    if (auto gr = ref->to<IR::GlobalRef>())
        reg = gr->obj->to<IR::Register>();
    else if (auto nr = ref->to<IR::PathExpression>())
        reg = structure->registers.get(nr->path->name);
    if (!reg) {
        ::error("Expected a register reference %1%", ref);
        return nullptr; }

    const IR::Type* castType = nullptr;
    int width = reg->width;
    if (width > 0)
        castType = IR::Type_Bits::get(width);
    // Else this is a structured type, so no cast is inserted below.

    auto newname = structure->registers.get(reg);
    auto registerref = new IR::PathExpression(newname);
    auto methodName = structure->v1model.registers.write.Id();
    auto method = new IR::Member(registerref, methodName);
    auto args = new IR::Vector<IR::Argument>();
    auto arg0 = new IR::Cast(primitive->operands.at(1)->srcInfo,
                             structure->v1model.registers.index_type,
                             conv.convert(primitive->operands.at(1)));
    const IR::Expression* arg1 = conv.convert(primitive->operands.at(2));
    if (castType != nullptr)
        arg1 = new IR::Cast(primitive->operands.at(2)->srcInfo, castType, arg1);
    args->push_back(new IR::Argument(arg0));
    args->push_back(new IR::Argument(arg1));
    auto mc = new IR::MethodCallExpression(primitive->srcInfo, method, args);
    return new IR::MethodCallStatement(mc->srcInfo, mc);
}

CONVERT_PRIMITIVE(truncate) {
    ExpressionConverter conv(structure);
    OPS_CK(primitive, 1);
    auto len = primitive->operands.at(0);
    auto methodName = structure->v1model.truncate.Id();
    auto method = new IR::PathExpression(methodName);
    auto args = new IR::Vector<IR::Argument>();
    auto arg0 = new IR::Cast(len->srcInfo, structure->v1model.truncate.length_type,
                             conv.convert(len));
    args->push_back(new IR::Argument(arg0));
    auto mc = new IR::MethodCallExpression(primitive->srcInfo, method, args);
    return new IR::MethodCallStatement(mc->srcInfo, mc);
}

CONVERT_PRIMITIVE(exit) {
    ExpressionConverter conv(structure);
    OPS_CK(primitive, 0);
    return new IR::ExitStatement(primitive->srcInfo);
}

CONVERT_PRIMITIVE(funnel_shift_right) {
    ExpressionConverter conv(structure);
    OPS_CK(primitive, 4);
    auto dest = conv.convert(primitive->operands.at(0));
    auto hi = conv.convert(primitive->operands.at(1));
    auto lo = conv.convert(primitive->operands.at(2));
    auto shift = conv.convert(primitive->operands.at(3));
    auto src = new IR::Shr(new IR::Concat(hi, lo), shift);
    return structure->assign(primitive->srcInfo, dest, src, primitive->operands.at(0)->type);
}

const IR::P4Action*
ProgramStructure::convertAction(const IR::ActionFunction* action, cstring newName,
                                const IR::Meter* meterToAccess,
                                cstring counterToAccess) {
    LOG3("Converting action " << action->name);
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
            ::warning(ErrorType::WARN_TYPE_INFERENCE,
                      "Could not infer type for %1%, using bit<8>", p);
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
        auto args = new IR::Vector<IR::Argument>();
        auto arg = conv.convert(meterToAccess->result);
        if (arg != nullptr)
            args->push_back(new IR::Argument(arg));
        auto mc = new IR::MethodCallExpression(method, args);
        auto stat = new IR::MethodCallStatement(mc->srcInfo, mc);
        body->push_back(stat); }

    if (counterToAccess != nullptr) {
        ExpressionConverter conv(this);
        auto decl = get(counterMap, counterToAccess);
        CHECK_NULL(decl);
        auto extObj = new IR::PathExpression(decl->name);
        auto method = new IR::Member(extObj, v1model.directCounter.count.Id());
        auto mc = new IR::MethodCallExpression(method, new IR::Vector<IR::Argument>());
        auto stat = new IR::MethodCallStatement(mc->srcInfo, mc);
        body->push_back(stat); }

    for (auto p : action->action) {
        auto stat = convertPrimitive(p);
        if (stat != nullptr)
            body->push_back(stat); }

    // Save the original action name in an annotation
    auto annos = addGlobalNameAnnotation(action->name, action->annotations);
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
    return new IR::Member(cm->srcInfo, enumref, kind);
}

const IR::Declaration_Instance*
ProgramStructure::convert(const IR::Register* reg, cstring newName,
                          const IR::Type *regElementType) {
    LOG3("Synthesizing " << reg);
    if (regElementType) {
        if (auto str = regElementType->to<IR::Type_StructLike>())
            regElementType = new IR::Type_Name(new IR::Path(str->name));
        // use provided type
    } else if (reg->width > 0) {
        regElementType = IR::Type_Bits::get(reg->width);
    } else if (reg->layout) {
        cstring newName = ::get(registerLayoutType, reg->layout);
        if (newName.isNullOrEmpty())
            newName = reg->layout;
        regElementType = new IR::Type_Name(new IR::Path(newName));
    } else {
        ::warning(ErrorType::WARN_MISSING,
                  "%1%: Register width unspecified; using %2%", reg, defaultRegisterWidth);
        regElementType = IR::Type_Bits::get(defaultRegisterWidth);
    }

    IR::ID ext = v1model.registers.Id();
    auto typepath = new IR::Path(ext);
    auto type = new IR::Type_Name(typepath);
    auto typeargs = new IR::Vector<IR::Type>();
    typeargs->push_back(regElementType);
    auto spectype = new IR::Type_Specialized(type, typeargs);
    auto args = new IR::Vector<IR::Argument>();
    if (reg->direct) {
        // FIXME -- do we need a direct_register extern in v1model?  For now
        // using size of 0 to specify a direct register
        args->push_back(new IR::Argument(new IR::Constant(v1model.registers.size_type, 0)));
    } else {
        args->push_back(new IR::Argument(
            new IR::Constant(v1model.registers.size_type, reg->instance_count))); }
    auto annos = addGlobalNameAnnotation(reg->name, reg->annotations);
    auto decl = new IR::Declaration_Instance(newName, annos, spectype, args, nullptr);
    return decl;
}

const IR::Declaration_Instance*
ProgramStructure::convert(const IR::CounterOrMeter* cm, cstring newName) {
    LOG3("Synthesizing " << cm);
    IR::ID ext;
    if (cm->is<IR::Counter>())
        ext = v1model.counter.Id();
    else
        ext = v1model.meter.Id();
    auto typepath = new IR::Path(ext);
    auto type = new IR::Type_Name(typepath);
    auto args = new IR::Vector<IR::Argument>();
    args->push_back(
        new IR::Argument(cm->srcInfo,
            new IR::Constant(v1model.counterOrMeter.size_type, cm->instance_count)));
    auto kindarg = counterType(cm);
    args->push_back(new IR::Argument(kindarg));
    auto annos = addGlobalNameAnnotation(cm->name, cm->annotations);
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
ProgramStructure::convertDirectMeter(const IR::Meter* m, cstring newName) {
    LOG3("Synthesizing " << m);
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
    auto args = new IR::Vector<IR::Argument>();
    auto kindarg = counterType(m);
    args->push_back(new IR::Argument(kindarg));
    auto annos = addGlobalNameAnnotation(m->name, m->annotations);
    if (m->pre_color != nullptr) {
        auto meterPreColor = ExpressionConverter(this).convert(m->pre_color);
        if (meterPreColor != nullptr)
            annos = annos->addAnnotation("pre_color", meterPreColor);
    }
    auto decl = new IR::Declaration_Instance(newName, annos, specType, args, nullptr);
    return decl;
}

const IR::Declaration_Instance*
ProgramStructure::convertDirectCounter(const IR::Counter* c, cstring newName) {
    LOG3("Synthesizing " << c);

    IR::ID ext = v1model.directCounter.Id();
    auto typepath = new IR::Path(ext);
    auto type = new IR::Type_Name(typepath);
    auto args = new IR::Vector<IR::Argument>();
    auto kindarg = counterType(c);
    args->push_back(new IR::Argument(kindarg));
    auto annos = addGlobalNameAnnotation(c->name, c->annotations);
    if (c->min_width >= 0)
        annos = annos->addAnnotation("min_width", new IR::Constant(c->min_width));
    if (c->max_width >= 0)
        annos = annos->addAnnotation("max_width", new IR::Constant(c->max_width));
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
                auto extcounter = convertDirectCounter(c.first, c.second);
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
        if (!declarations->getDeclaration(r->name)) {
            declarations->push_back(r);
        }
    }

    for (auto c : externsToDo) {
        auto ext = externs.get(c);
        if (!ExternConverter::cvtAsGlobal(this, ext)) {
            ext = ExternConverter::cvtExternInstance(this, ext, externs.get(ext), &stateful);
            stateful.push_back(ext);
        }
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
            auto annos = addGlobalNameAnnotation(cc);
            auto decl = new IR::Declaration_Instance(
                IR::ID(iname), annos, type, new IR::Vector<IR::Argument>(), nullptr);
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
        knownControls.push_back(it.first->name);
    bool cycles = calledControls.sort(knownControls, controlsToDo);
    if (cycles) {
        // TODO: give a better error message
        ::error("Program contains recursive control blocks");
        return;
    }

    for (auto ap : action_profiles)
        if (auto action_profile = convertActionProfile(ap.first, ap.second))
            declarations->push_back(action_profile);

    for (auto ext : externs) {
        if (ExternConverter::cvtAsGlobal(this, ext.first)) {
            // FIXME -- 'declarations' is a vector Nodes instead of Declarations.  Should
            // FIXME -- fix the IR to use a NameMap for scopes anyways.
            IR::IndexedVector<IR::Declaration> tmpscope;
            auto e = ExternConverter::cvtExternInstance(this, ext.first, ext.second, &tmpscope);
            for (auto d : tmpscope)
                declarations->push_back(d);
            if (e)
                declarations->push_back(e);
        }
    }

    for (auto c : controlsToDo) {
        auto ct = controls.get(c);
        /// do not convert control block if it is not invoked
        /// by other control block and it is not ingress or egress.
        if (!calledControls.isCallee(c) &&
            ct != controls.get(v1model.ingress.name) &&
            ct != controls.get(v1model.egress.name))
            continue;
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
    auto args = new IR::Vector<IR::Argument>();
    auto emptyArgs = new IR::Vector<IR::Argument>();

    auto parserPath = new IR::Path(v1model.parser.Id());
    auto parserType = new IR::Type_Name(parserPath);
    auto parserConstruct = new IR::ConstructorCallExpression(parserType, emptyArgs);
    args->push_back(new IR::Argument(parserConstruct));

    auto verifyPath = new IR::Path(verifyChecksums->name);
    auto verifyType = new IR::Type_Name(verifyPath);
    auto verifyConstruct = new IR::ConstructorCallExpression(verifyType, emptyArgs);
    args->push_back(new IR::Argument(verifyConstruct));

    auto ingressPath = new IR::Path(ingressReference);
    auto ingressType = new IR::Type_Name(ingressPath);
    auto ingressConstruct = new IR::ConstructorCallExpression(ingressType, emptyArgs);
    args->push_back(new IR::Argument(ingressConstruct));

    auto egressPath = new IR::Path(v1model.egress.Id());
    auto egressType = new IR::Type_Name(egressPath);
    auto egressConstruct = new IR::ConstructorCallExpression(egressType, emptyArgs);
    args->push_back(new IR::Argument(egressConstruct));

    auto updatePath = new IR::Path(updateChecksums->name);
    auto updateType = new IR::Type_Name(updatePath);
    auto updateConstruct = new IR::ConstructorCallExpression(updateType, emptyArgs);
    args->push_back(new IR::Argument(updateConstruct));

    auto deparserPath = new IR::Path(deparser->name);
    auto deparserType = new IR::Type_Name(deparserPath);
    auto deparserConstruct = new IR::ConstructorCallExpression(deparserType, emptyArgs);
    args->push_back(new IR::Argument(deparserConstruct));

    auto result = new IR::Declaration_Instance(name, type, args, nullptr);
    declarations->push_back(result);
}

// Get a field_list_calculation from a name
const IR::FieldListCalculation* ProgramStructure::getFieldListCalculation(const IR::Expression *e) {
    if (auto *pe = e->to<IR::PathExpression>()) {
        return field_list_calculations.get(pe->path->name); }
    if (auto *gref = e->to<IR::GlobalRef>()) {
        // an instance of a global object, but in P4_14, there might be a field_list_calculation
        // with the same name
        return field_list_calculations.get(gref->toString()); }
    return nullptr;
}

// if a FieldListCalculation contains multiple field lists concatenate them all
// into a temporary field list
const IR::FieldList* ProgramStructure::getFieldLists(const IR::FieldListCalculation* flc) {
    // FIXME -- this duplicates P4_14::TypeCheck.  Why not just use flc->input_fields?
    if (flc->input->names.size() == 0) {
        ::error("%1%: field_list_calculation with zero inputs", flc);
        return nullptr;
    }
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
        result->payload = result->payload || fl->payload;
    }
    return result;
}

void ProgramStructure::createChecksumVerifications() {
    ExpressionConverter conv(this);

    auto params = new IR::ParameterList;
    auto headpath = new IR::Path(v1model.headersType.Id());
    auto headtype = new IR::Type_Name(headpath);
    auto headers = new IR::Parameter(v1model.verify.headersParam.Id(),
                                     IR::Direction::InOut, headtype);
    params->push_back(headers);
    conversionContext.header = paramReference(headers);

    auto metapath = new IR::Path(v1model.metadataType.Id());
    auto metatype = new IR::Type_Name(metapath);
    auto meta = new IR::Parameter(v1model.parser.metadataParam.Id(),
                                  IR::Direction::InOut, metatype);
    auto body = new IR::BlockStatement();

    params->push_back(meta);
    conversionContext.userMetadata = paramReference(meta);
    conversionContext.standardMetadata = nullptr;
    auto type = new IR::Type_Control(v1model.verify.Id(), params);

    for (auto cf : calculated_fields) {
        LOG3("Converting " << cf);
        auto dest = conv.convert(cf->field);

        for (auto uov : cf->specs) {
            if (uov.update) continue;
            auto flc = field_list_calculations.get(uov.name.name);

            auto fl = getFieldLists(flc);
            if (fl == nullptr) continue;
            auto le = conv.convert(fl);
            IR::ID methodName = fl->payload ? v1model.verify_checksum_with_payload.Id() :
                                v1model.verify_checksum.Id();
            auto method = new IR::PathExpression(methodName);
            auto args = new IR::Vector<IR::Argument>();
            const IR::Expression* condition;
            if (uov.cond != nullptr)
                condition = conv.convert(uov.cond);
            else
                condition = new IR::BoolLiteral(true);
            auto algo = convertHashAlgorithms(flc->algorithm);
            args->push_back(new IR::Argument(condition));
            args->push_back(new IR::Argument(le));
            args->push_back(new IR::Argument(dest));
            args->push_back(new IR::Argument(algo));

            auto mc = new IR::MethodCallStatement(new IR::MethodCallExpression(method, args));
            body->push_back(mc);

            for (auto annot : cf->annotations->annotations)
                body->annotations = body->annotations->add(annot);

            for (auto annot : flc->annotations->annotations)
                body->annotations = body->annotations->add(annot);

            LOG3("Converted " << flc);
        }
    }
    verifyChecksums = new IR::P4Control(
        v1model.verify.Id(), type, *new IR::IndexedVector<IR::Declaration>(), body);
    declarations->push_back(verifyChecksums);
    conversionContext.clear();
}

void ProgramStructure::createChecksumUpdates() {
    ExpressionConverter conv(this);
    auto params = new IR::ParameterList;
    auto headpath = new IR::Path(v1model.headersType.Id());
    auto headtype = new IR::Type_Name(headpath);
    auto headers = new IR::Parameter(v1model.compute.headersParam.Id(),
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

    auto type = new IR::Type_Control(v1model.compute.Id(), params);
    auto body = new IR::BlockStatement;
    for (auto cf : calculated_fields) {
        LOG3("Conveting " << cf);
        auto dest = conv.convert(cf->field);

        for (auto uov : cf->specs) {
            if (!uov.update) continue;
            auto flc = field_list_calculations.get(uov.name.name);
            auto fl = getFieldLists(flc);
            if (fl == nullptr) continue;
            auto le = conv.convert(fl);

            IR::ID methodName = fl->payload ? v1model.update_checksum_with_payload.Id() :
                                v1model.update_checksum.Id();
            auto method = new IR::PathExpression(methodName);
            auto args = new IR::Vector<IR::Argument>();
            const IR::Expression* condition;
            if (uov.cond != nullptr)
                condition = conv.convert(uov.cond);
            else
                condition = new IR::BoolLiteral(true);
            auto algo = convertHashAlgorithms(flc->algorithm);

            args->push_back(new IR::Argument(condition));
            args->push_back(new IR::Argument(le));
            args->push_back(new IR::Argument(dest));
            args->push_back(new IR::Argument(algo));
            auto mc = new IR::MethodCallStatement(new IR::MethodCallExpression(method, args));
            body->push_back(mc);

            for (auto annot : cf->annotations->annotations)
                body->annotations = body->annotations->add(annot);

            for (auto annot : flc->annotations->annotations)
                body->annotations = body->annotations->add(annot);

            LOG3("Converted " << flc);
        }
    }
    updateChecksums = new IR::P4Control(
        v1model.compute.Id(), type, IR::IndexedVector<IR::Declaration>(), body);
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
    LOG3("Inspecting " << control->name);
    for (auto it : tableMapping) {
        if (it.second == control)
            out.push_back(it.first);
    }
    // sort alphabetically to have a deterministic order
    std::sort(out.begin(), out.end(),
              [](const IR::V1Table* left, const IR::V1Table* right) {
                  return left->name.name < right->name.name; });
}

void ProgramStructure::populateOutputNames() {
    static const char* used_names[] = {
        // core.p4
        "packet_in",
        "packet_out",
        "NoAction",
        "exact",
        "ternary",
        "lpm",
        // v1model.p4
        "range",
        "selector",
        // v1model externs
        "verify_checksum",
        "verify_checksum_with_payload",
        "update_checksum",
        "update_checksum_with_payload",
        "CounterType",
        "MeterType",
        "HashAlgorithm",
        "CloneType",
        "counter",
        "direct_counter",
        "meter",
        "direct_meter",
        "register",
        "action_profile",
        "action_selector",
        "random",
        "digest",
        "mark_to_drop",
        "hash",
        "resubmit",
        "recirculate",
        "clone",
        "clone3",
        "truncate",
        "V1Switch",
        // other v1model names
        "main",
        "headers",
        "metadata",
        "computeChecksum",
        "verifyChecksum",
        "ParserImpl",
        "DeparserImpl",
        // parameters
        "packet",
        "hdr",
        "meta",
        nullptr
    };
    for (const char** c = used_names; *c != nullptr; ++c)
        allNames.emplace(*c);
}

}  // namespace P4V1
