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
#include "JsonObjects.h"

namespace BMV2 {

// TODO(hanw): move to header.cpp
void Backend::createJsonType(const IR::Type_StructLike *st) {
    cstring name = extVisibleName(st);
    auto fields = new Util::JsonArray();
    pushFields(st, fields);
    auto id = bm->add_header_type(name, fields);
}

// TODO(hanw): move to header.cpp
void Backend::pushFields(const IR::Type_StructLike *st,
                                Util::JsonArray *fields) {
    for (auto f : st->fields) {
        auto ftype = typeMap->getType(f, true);
        if (ftype->to<IR::Type_StructLike>()) {
            BUG("%1%: nested structure", st);
        } else if (ftype->is<IR::Type_Boolean>()) {
            auto field = pushNewArray(fields);
            field->append(f->name.name);
            field->append(boolWidth);
            field->append(0);
        } else if (auto type = ftype->to<IR::Type_Bits>()) {
            auto field = pushNewArray(fields);
            field->append(f->name.name);
            field->append(type->size);
            field->append(type->isSigned);
        } else if (auto type = ftype->to<IR::Type_Varbits>()) {
            auto field = pushNewArray(fields);
            field->append(f->name.name);
            field->append(type->size);  // FIXME -- where does length go?
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
        cstring name = refMap->newName("_padding");
        auto field = pushNewArray(fields);
        field->append(name);
        field->append(8 - padding);
        field->append(false);
    }
}


/**
 * We synthesize a "header_type" for each local which has a struct type
 * and we pack all the scalar-typed locals into a scalarsStruct
 */
// TODO(hanw): move to header.cpp
void Backend::addLocals() {
    // TODO(hanw): avoid modifying refMap
    scalarsName = refMap->newName("scalars");
    scalarsStruct = new Util::JsonObject();
    scalarsStruct->emplace("name", scalarsName);
    scalarsStruct->emplace("id", nextId("header_types"));
    scalarFields = mkArrayField(scalarsStruct, "fields");

    LOG1("... structure " << structure.variables);
    for (auto v : structure.variables) {
        LOG1("Creating local " << v);
        auto type = typeMap->getType(v, true);
        if (auto st = type->to<IR::Type_StructLike>()) {
            auto metadata_type = extVisibleName(st);
            createJsonType(st);
            bm->add_metadata(metadata_type, v->name);
        } else if (auto stack = type->to<IR::Type_Stack>()) {
            auto type = typeMap->getTypeType(stack->elementType, true);
            BUG_CHECK(type->is<IR::Type_Header>(), "%1% not a header type", stack->elementType);
            auto ht = type->to<IR::Type_Header>();
            createJsonType(ht);
            cstring header_type = extVisibleName(stack->elementType->to<IR::Type_Header>());
            std::vector<unsigned> header_ids;
            for (unsigned i=0; i < stack->getSize(); i++) {
                cstring name = v->name + "[" + Util::toString(i) + "]";
                auto header_id = bm->add_header(header_type, name);
                header_ids.push_back(header_id);
            }
            bm->add_header_stack(header_type, v->name, stack->getSize(), header_ids);
        } else if (type->is<IR::Type_Bits>()) {
            auto tb = type->to<IR::Type_Bits>();
            auto field = pushNewArray(scalarFields);
            field->append(v->name.name);
            field->append(tb->size);
            field->append(tb->isSigned);
            scalars_width += tb->size;
            // bm->add_header_field(header_type, header_name, fields);
        } else if (type->is<IR::Type_Boolean>()) {
            auto field = pushNewArray(scalarFields);
            field->append(v->name.name);
            field->append(boolWidth);
            field->append(0);
            scalars_width += boolWidth;
            // bm->add_header_field(header_type, header_name, fields);
        } else if (type->is<IR::Type_Error>()) {
            auto field = pushNewArray(scalarFields);
            field->append(v->name.name);
            field->append(32);  // using 32-bit fields for errors
            field->append(0);
            scalars_width += 32;
            // bm->add_header_field(header_type, header_name, fields);
        } else {
            BUG("%1%: type not yet handled on this target", type);
        }
    }

    // FIXME: do we need to put scalarsStruct in set?
    bm->header_types->append(scalarsStruct);
    bm->add_metadata(scalarsName, scalarsName);
}

// TODO(hanw): merge with add_locals
void Backend::padScalars() {
    unsigned padding = scalars_width % 8;
    auto scalarFields = (*scalarsStruct)["fields"]->to<Util::JsonArray>();
    if (padding != 0) {
        cstring name = refMap->newName("_padding");
        auto field = pushNewArray(scalarFields);
        field->append(name);
        field->append(8 - padding);
        field->append(false);
    }
}

#ifdef PSA
void Backend::genExternMethod(Util::JsonArray* result, P4::ExternMethod *em) {
    auto name = "_" + em->actualExternType->name + "_" + em->method->name.name;
    auto primitive = mkPrimitive(name, result);
    auto parameters = mkParameters(primitive);

    auto ext = new Util::JsonObject();
    ext->emplace("type", "extern");

    // FIXME: PSA have extern pass building a map and lookup here.
    if (em->object->is<IR::Parameter>()) {
        auto param = em->object->to<IR::Parameter>();
        // auto packageObject = resolveParameter(param);
        // ext->emplace("value", packageObject->getName());
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

// TODO(hanw): return JsonArray*
void Backend::convertActionBody(const IR::Vector<IR::StatOrDecl>* body, Util::JsonArray* result) {
    // FIXME: conv->createFieldList = true??
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
            auto type = typeMap->getType(l, true);
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
            auto mi = P4::MethodInstance::resolve(mc, refMap, typeMap);
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

// TODO(hanw): return JsonArray*
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
            if (!refMap->isUsed(p))
                ::warning("Unused action parameter %1%", p);

            auto param = new Util::JsonObject();
            param->emplace("name", p->name);
            auto type = typeMap->getType(p, true);
            if (!type->is<IR::Type_Bits>())
                ::error("%1%: Action parameters can only be bit<> or int<> on this target", p);
            param->emplace("bitwidth", type->width_bits());
            params->append(param);
        }
        auto body = mkArrayField(jact, "primitives");
        convertActionBody(&action->body->components, body);
        actions->append(jact);

        // auto params = new Util::JsonArray();
        // for (auto p : *action->parameters->getEnumerator()) {
        //   auto param = new Util::JsonObject();
        //   param->emplace("name", p->name);
        //   params->append(param);
        // }
        //
        // TODO(hanw): function to convertActionParams
        // auto params = convertActionParams(&action->parameters);
        // auto body = convertActionBody(&action->body->components);
        // bm->add_action(name, params, body);
    }
}

// TODO(hanw): remove
void Backend::createMetadata() {
    bm->add_metadata("standard_metadata", "standard_metadata");
}

// TODO(hanw): clean up
void Backend::createFieldAliases(const char *remapFile) {
    Arch::MetadataRemapT *remap = Arch::readMap(remapFile);
    LOG1("Metadata alias map of size = " << remap->size());
    for (auto r : *remap) {
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

// TODO(hanw): remove
void Backend::addErrors(Util::JsonArray* errors) {
    for (const auto &p : errorCodesMap) {
        auto name = p.first->toString();
        auto entry = pushNewArray(errors);
        entry->append(name);
        entry->append(p.second);
        // auto type = p.second;
        // bm->json->add_error(name, type);
    }
}

void Backend::process(const IR::ToplevelBlock* tb) {
    setName("BackEnd");
    addPasses({
        new DiscoverStructure(&structure),
        new ErrorCodesVisitor(&errorCodesMap),
        // new MetadataRemap(this, Arch::readMap(mapfile)),
    });
    tb->getProgram()->apply(*this);
}

/// BMV2 Backend that takes the top level block and converts it to a JsonObject.
void Backend::convert(const IR::ToplevelBlock* tb, CompilerOptions& options) {
    toplevel.emplace("program", options.file);
    toplevel.emplace("__meta__", bm->meta);
    toplevel.emplace("header_types", bm->header_types);
    toplevel.emplace("headers", bm->headers);
    toplevel.emplace("header_stacks", bm->header_stacks);
    field_lists = mkArrayField(&toplevel, "field_lists");
    errors = mkArrayField(&toplevel, "errors");
    toplevel.emplace("enums", bm->enums);
    toplevel.emplace("parsers", bm->parsers);
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
        new CopyAnnotations(refMap, &blockTypeMap),
        new VisitFunctor([this](){ addLocals(); }),
        new VisitFunctor([this](){ createMetadata(); }),
        new ConvertHeaders(this),
        new VisitFunctor([this](){ padScalars(); }),
        new VisitFunctor([this](){ addErrors(errors); }),
#ifdef PSA
        new ConvertExterns(this),
#endif
        new ConvertParser(this),
        // createAction must be called before convertControl
        new VisitFunctor([this](){ createActions(actions); }),
        new ConvertControl(this),
        new ConvertDeparser(this),
        // new VisitFunctor([this, metadata_remap_file](){
        //     createFieldAliases(metadata_remap_file); }),
    };
    codegen_passes.setName("CodeGen");
    tb->getMain()->apply(codegen_passes);
}

}  // namespace BMV2
