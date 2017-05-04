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

#include "backend.h"
#include "control.h"
#include "copyAnnotations.h"
#include "deparser.h"
#include "errorcode.h"
#include "expression.h"
#include "extern.h"
#include "frontends/p4/methodInstance.h"
#include "header.h"
#include "metadata.h"
#include "parser.h"

namespace BMV2 {

void Backend::addMetaInformation() {
  auto info = new Util::JsonObject();

  static constexpr int version_major = 2;
  static constexpr int version_minor = 7;
  auto version = mkArrayField(info, "version");
  version->append(version_major);
  version->append(version_minor);

  info->emplace("compiler", "https://github.com/p4lang/p4c");
  toplevel.emplace("__meta__", info);
}

void Backend::addEnums(Util::JsonArray* enums) {
    CHECK_NULL(enumMap);
    for (const auto &pEnum : *enumMap) {
        auto enumName = pEnum.first->getName().name.c_str();
        auto enumObj = new Util::JsonObject();
        enumObj->emplace("name", enumName);
        enumObj->emplace_non_null("source_info", pEnum.first->sourceInfoJsonObj());
        auto entries = mkArrayField(enumObj, "entries");
        for (const auto &pEntry : *pEnum.second) {
            auto entry = new Util::JsonArray();
            entry->append(pEntry.first);
            entry->append(pEntry.second);
            entries->append(entry);
        }
        enums->append(enumObj);
    }
}

void Backend::createScalars() {
    scalarsName = refMap.newName("scalars");
    scalarsStruct = new Util::JsonObject();
    scalarsStruct->emplace("name", scalarsName);
    scalarsStruct->emplace("id", nextId("header_types"));
    scalarFields = mkArrayField(scalarsStruct, "fields");
}

void Backend::createJsonType(const IR::Type_StructLike *st) {
    auto isCreated = headerTypesCreated.find(st) != headerTypesCreated.end();
    if (!isCreated) {
        auto typeJson = new Util::JsonObject();
        cstring name = extVisibleName(st);
        typeJson->emplace("name", name);
        typeJson->emplace("id", nextId("header_types"));
        typeJson->emplace_non_null("source_info", st->sourceInfoJsonObj());
        headerTypes->append(typeJson);
        auto fields = mkArrayField(typeJson, "fields");
        pushFields(st, fields);
        headerTypesCreated.insert(st);
    }
}

void Backend::pushFields(const IR::Type_StructLike *st,
                         Util::JsonArray *fields) {
    for (auto f : st->fields) {
        auto ftype = typeMap.getType(f, true);
        if (ftype->to<IR::Type_StructLike>()) {
            BUG("%1%: nested structure", st);
        } else if (ftype->is<IR::Type_Boolean>()) {
            auto field = pushNewArray(fields);
            field->append(f->name.name);
            field->append(1); // boolWidth
            field->append(0);
        } else if (auto type = ftype->to<IR::Type_Bits>()) {
            auto field = pushNewArray(fields);
            field->append(f->name.name);
            field->append(type->size);
            field->append(type->isSigned);
        } else if (auto type = ftype->to<IR::Type_Varbits>()) {
            auto field = pushNewArray(fields);
            field->append(f->name.name);
            field->append(type->size); // FIXME -- where does length go?
        } else if (ftype->to<IR::Type_Stack>()) {
            BUG("%1%: nested stack", st);
        } else {
            BUG("%1%: unexpected type for %2%.%3%", ftype, st, f->name);
        }
    }
    // must add padding
    unsigned width = st->width_bits();
    unsigned padding = width % 8;
    if (padding != 0) {
        cstring name = refMap.newName("_padding");
        auto field = pushNewArray(fields);
        field->append(name);
        field->append(8 - padding);
        field->append(false);
    }
}

void Backend::addLocals() {
    // We synthesize a "header_type" for each local which has a struct type
    // and we pack all the scalar-typed locals into a scalarsStruct
    auto scalarFields = scalarsStruct->get("fields")->to<Util::JsonArray>();
    CHECK_NULL(scalarFields);

    LOG3("... structure " << structure.variables);
    for (auto v : structure.variables) {
        LOG3("Creating local " << v);
        auto type = typeMap.getType(v, true);
        if (auto st = type->to<IR::Type_StructLike>()) {
            createJsonType(st);
            auto name = st->name;
            // create header instance?
            auto json = new Util::JsonObject();
            json->emplace("name", v->name);
            json->emplace("id", nextId("headers"));
            // TODO(jafingerhut) - add line/col here?
            json->emplace("header_type", name);
            json->emplace("metadata", true);
            json->emplace("pi_omit", true);  // Don't expose in PI.
            headerInstances->append(json);
        } else if (auto stack = type->to<IR::Type_Stack>()) {
            auto json = new Util::JsonObject();
            json->emplace("name", v->name);
            json->emplace("id", nextId("stack"));
            // TODO(jafingerhut) - add line/col here?
            json->emplace("size", stack->getSize());
            auto type = typeMap.getTypeType(stack->elementType, true);
            BUG_CHECK(type->is<IR::Type_Header>(), "%1% not a header type", stack->elementType);
            auto ht = type->to<IR::Type_Header>();
            createJsonType(ht);

            cstring header_type = extVisibleName(stack->elementType->to<IR::Type_Header>());
            json->emplace("header_type", header_type);
            auto stackMembers = mkArrayField(json, "header_ids");
            for (unsigned i=0; i < stack->getSize(); i++) {
                unsigned id = nextId("headers");
                stackMembers->append(id);
                auto header = new Util::JsonObject();
                cstring name = v->name + "[" + Util::toString(i) + "]";
                header->emplace("name", name);
                header->emplace("id", id);
                // TODO(jafingerhut) - add line/col here?
                header->emplace("header_type", header_type);
                header->emplace("metadata", false);
                header->emplace("pi_omit", true);  // Don't expose in PI.
                headerInstances->append(header);
            }
            headerStacks->append(json);
        } else if (type->is<IR::Type_Bits>()) {
            auto tb = type->to<IR::Type_Bits>();
            auto field = pushNewArray(scalarFields);
            field->append(v->name.name);
            field->append(tb->size);
            field->append(tb->isSigned);
            scalars_width += tb->size;
        } else if (type->is<IR::Type_Boolean>()) {
            auto field = pushNewArray(scalarFields);
            field->append(v->name.name);
            field->append(boolWidth);
            field->append(0);
            scalars_width += boolWidth;
        } else if (type->is<IR::Type_Error>()) {
            auto field = pushNewArray(scalarFields);
            field->append(v->name.name);
            field->append(32);  // using 32-bit fields for errors
            field->append(0);
            scalars_width += 32;
        } else {
            BUG("%1%: type not yet handled on this target", type);
        }
    }

    //FIXME: do we need to put scalarsStruct in set?
    //headerTypesCreated.insert(scalarsName);
    headerTypes->append(scalarsStruct);

    // insert the scalars instance
    auto json = new Util::JsonObject();
    json->emplace("name", scalarsName);
    json->emplace("id", nextId("headers"));
    // TODO(jafingerhut) - add line/col here?
    json->emplace("header_type", scalarsName);
    json->emplace("metadata", true);
    json->emplace("pi_omit", true);  // Don't expose in PI.
    headerInstances->append(json);
}

void Backend::padScalars() {
    unsigned padding = scalars_width % 8;
    auto scalarFields = (*scalarsStruct)["fields"]->to<Util::JsonArray>();
    if (padding != 0) {
        cstring name = refMap.newName("_padding");
        auto field = pushNewArray(scalarFields);
        field->append(name);
        field->append(8 - padding);
        field->append(false);
    }
}

#ifdef PSA
void Backend::genExternMethod(Util::JsonArray* result, P4::ExternMethod *em) {
    auto name = "_" + em->actualExternType->name + "_" + em->method->name.name;
    // if (em->originalExternType->name.name == corelib.packetOut.name &&
    //         em->method->name.name == corelib.packetOut.emit.name) {
    //     LOG1("print emit method");
    // }
    auto primitive = mkPrimitive(name, result);
    auto parameters = mkParameters(primitive);

    auto ext = new Util::JsonObject();
    ext->emplace("type", "extern");

    //FIXME: have extern pass building a map and lookup here.
    if (em->object->is<IR::Parameter>()) {
        auto param = em->object->to<IR::Parameter>();
        //auto packageObject = resolveParameter(param);
        //ext->emplace("value", packageObject->getName());
        ext->emplace("value", "FIXME");
    } else {
        ext->emplace("value", em->object->getName());
    }
    parameters->append(ext);

    for (auto a : *mc->arguments) {
        auto arg = conv->convert(a);
        parameters->append(arg);
    }
}
#endif

void Backend::convertActionBody(const IR::Vector<IR::StatOrDecl>* body, Util::JsonArray* result) {
    //FIXME: conv->createFieldList = true??
    for (auto s : *body) {
        // TODO(jafingerhut) - add line/col at all individual cases below,
        // or perhaps it can be done as a common case above or below
        // for all of them?
        if (!s->is<IR::Statement>()) {
            continue;
        } else if (auto block = s->to<IR::BlockStatement>()) {
            convertActionBody(&block->components, result);
            continue;
        } else if (s->is<IR::ReturnStatement>()) {
            break;
        } else if (s->is<IR::ExitStatement>()) {
            auto primitive = mkPrimitive("exit", result);
            (void)mkParameters(primitive);
            primitive->emplace_non_null("source_info", s->sourceInfoJsonObj());
            break;
        } else if (s->is<IR::AssignmentStatement>()) {
            const IR::Expression* l, *r;
            auto assign = s->to<IR::AssignmentStatement>();
            l = assign->left;
            r = assign->right;

            cstring operation;
            auto type = typeMap.getType(l, true);
            if (type->is<IR::Type_StructLike>())
                operation = "copy_header";
            else
                operation = "modify_field";
            auto primitive = mkPrimitive(operation, result);
            auto parameters = mkParameters(primitive);
            primitive->emplace_non_null("source_info", assign->sourceInfoJsonObj());
            auto left = conv->convertLeftValue(l);
            parameters->append(left);
            bool convertBool = type->is<IR::Type_Boolean>();
            auto right = conv->convert(r, true, true, convertBool);
            parameters->append(right);
            continue;
        } else if (s->is<IR::EmptyStatement>()) {
            continue;
        } else if (s->is<IR::MethodCallStatement>()) {
            LOG1("Visit " << dbp(s));
            auto mc = s->to<IR::MethodCallStatement>()->methodCall;
            auto mi = P4::MethodInstance::resolve(mc, &refMap, &typeMap);
            if (mi->is<P4::ActionCall>()) {
                BUG("%1%: action call should have been inlined", mc);
                continue;
            } else if (mi->is<P4::BuiltInMethod>()) {
                auto builtin = mi->to<P4::BuiltInMethod>();

                cstring prim;
                auto parameters = new Util::JsonArray();
                auto obj = conv->convert(builtin->appliedTo);
                parameters->append(obj);

                if (builtin->name == IR::Type_Header::setValid) {
                    prim = "add_header";
                } else if (builtin->name == IR::Type_Header::setInvalid) {
                    prim = "remove_header";
                } else if (builtin->name == IR::Type_Stack::push_front) {
                    BUG_CHECK(mc->arguments->size() == 1, "Expected 1 argument for %1%", mc);
                    auto arg = conv->convert(mc->arguments->at(0));
                    prim = "push";
                    parameters->append(arg);
                } else if (builtin->name == IR::Type_Stack::pop_front) {
                    BUG_CHECK(mc->arguments->size() == 1, "Expected 1 argument for %1%", mc);
                    auto arg = conv->convert(mc->arguments->at(0));
                    prim = "pop";
                    parameters->append(arg);
                } else {
                    BUG("%1%: Unexpected built-in method", s);
                }
                auto primitive = mkPrimitive(prim, result);
                primitive->emplace("parameters", parameters);
                primitive->emplace_non_null("source_info", s->sourceInfoJsonObj());
                continue;
            } else if (mi->is<P4::ExternMethod>()) {
                auto em = mi->to<P4::ExternMethod>();
#ifdef PSA
                    genExternMethod(result, em);
#else
                    LOG1("P4V1:: convert " << s);
                    P4V1::V1Model::convertExternObjects(result, this, em, mc, s);
#endif
                continue;
            } else if (mi->is<P4::ExternFunction>()) {
                auto ef = mi->to<P4::ExternFunction>();
#ifdef PSA
                auto primitive = mkPrimitive(ef->method->name.name, result);
                auto parameters = mkParameters(primitive);
                primitive->emplace_non_null("source_info", s->sourceInfoJsonObj());
                for (auto a : *mc->arguments) {
                    parameters->append(conv->convert(a));
                }
#else
                P4V1::V1Model::convertExternFunctions(result, this, ef, mc, s);
#endif
                continue;
            }
        }
        ::error("%1%: not yet supported on this target", s);
    }
}

void Backend::createActions(Util::JsonArray* actions) {
    for (auto it : structure.actions) {
        auto action = it.first;
        cstring name = extVisibleName(action);
        auto jact = new Util::JsonObject();
        jact->emplace("name", name);
        unsigned id = nextId("actions");
        LOG1("emplace " << action << " " << id);
        structure.ids.emplace(action, id);
        jact->emplace("id", id);
        jact->emplace_non_null("source_info", action->sourceInfoJsonObj());
        auto params = mkArrayField(jact, "runtime_data");
        for (auto p : *action->parameters->getEnumerator()) {
            if (!refMap.isUsed(p))
                ::warning("Unused action parameter %1%", p);

            auto param = new Util::JsonObject();
            param->emplace("name", p->name);
            auto type = typeMap.getType(p, true);
            if (!type->is<IR::Type_Bits>())
                ::error("%1%: Action parameters can only be bit<> or int<> on this target", p);
            param->emplace("bitwidth", type->width_bits());
            params->append(param);
        }
        auto body = mkArrayField(jact, "primitives");
        convertActionBody(&action->body->components, body);
        actions->append(jact);
    }
}

void Backend::createMetadata() {
    auto json = new Util::JsonObject();
    json->emplace("name", "standard_metadata"); //FIXME: from arch
    json->emplace("id", nextId("headers"));
    json->emplace("header_type", "standard_metadata");
    json->emplace("metadata", true);
    headerInstances->append(json);
}

void Backend::createFieldAliases(const char *remapFile) {
    Arch::MetadataRemapT *remap = Arch::readMap(remapFile);
    LOG1("Metadata alias map of size = " << remap->size());
    for ( auto r : *remap) {
        auto container = new Util::JsonArray();
        auto alias = new Util::JsonArray();
        container->append(r.second);
        // break down the alias into meta . field
        auto meta = r.first.before(r.first.find('.'));
        alias->append(meta);
        alias->append(r.first.substr(meta.size()+1));
        container->append(alias);
        field_aliases->append(container);
    }
}

void Backend::addErrors(Util::JsonArray* errors) {
    for (const auto &p : errorCodesMap) {
        auto name = p.first->getName().name.c_str();
        auto entry = pushNewArray(errors);
        entry->append(name);
        entry->append(p.second);
    }
}

void Backend::process(const IR::ToplevelBlock* tb) {
    setName("BackEnd");
    // constexpr char mapfile[] = "p4include/p4d2model_bmss_meta.map";
    addPasses({
        new P4::TypeChecking(&refMap, &typeMap),
        new DiscoverStructure(&structure),
        new ErrorCodesVisitor(&errorCodesMap),
        // new MetadataRemap(this, Arch::readMap(mapfile)),
    });
    tb->getProgram()->apply(*this);
}

/// BMV2 Backend that takes the top level block and converts it to a JsonObject.
void Backend::convert(const IR::ToplevelBlock* tb, CompilerOptions& options) {
    toplevel.emplace("program", options.file);
    addMetaInformation();
    headerTypes = mkArrayField(&toplevel, "header_types");
    headerInstances = mkArrayField(&toplevel, "headers");
    headerStacks = mkArrayField(&toplevel, "header_stacks");
    field_lists = mkArrayField(&toplevel, "field_lists");
    errors = mkArrayField(&toplevel, "errors");
    enums = mkArrayField(&toplevel, "enums");
    parsers = mkArrayField(&toplevel, "parsers");
    deparsers = mkArrayField(&toplevel, "deparsers");
    meter_arrays = mkArrayField(&toplevel, "meter_arrays");
    counters = mkArrayField(&toplevel, "counter_arrays");
    register_arrays = mkArrayField(&toplevel, "register_arrays");
    calculations = mkArrayField(&toplevel, "calculations");
    learn_lists = mkArrayField(&toplevel, "learn_lists");
    actions = mkArrayField(&toplevel, "actions");
    pipelines = mkArrayField(&toplevel, "pipelines");
    checksums = mkArrayField(&toplevel, "checksums");
    force_arith = mkArrayField(&toplevel, "force_arith");
    externs = mkArrayField(&toplevel, "extern_instances");
    constexpr char metadata_remap_file[] = "p4include/p4d2model_bmss_meta.map";
    field_aliases = mkArrayField(&toplevel, "field_aliases");

    // This visitor is used in multiple passes to convert expression to json
    conv = new ExpressionConverter(this);

    PassManager codegen_passes = {
        new CopyAnnotations(&refMap, &blockTypeMap),
        new VisitFunctor([this](){ addEnums(enums); }),
        new VisitFunctor([this](){ createScalars(); }),
        new VisitFunctor([this](){ addLocals(); }),
        new VisitFunctor([this](){ createMetadata(); }),
        new ConvertHeaders(this),
        new VisitFunctor([this](){ padScalars(); }),
        new VisitFunctor([this](){ addErrors(errors); }),
        new ConvertExterns(this),
        new ConvertParser(&refMap, &typeMap, conv, parsers),
        // createAction must be called before convertControl
        new VisitFunctor([this](){ createActions(actions); }),
        new ConvertControl(this),
        new ConvertDeparser(this),
        // new VisitFunctor([this, metadata_remap_file](){
        //     createFieldAliases(metadata_remap_file); }),
    };
    tb->getMain()->apply(codegen_passes);
}

} // namespace BMV2
