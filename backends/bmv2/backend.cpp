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
#include "extern.h"
#include "header.h"
#include "parser.h"
#include "deparser.h"
#include "errorcode.h"
#include "expression.h"
#include "frontends/p4/methodInstance.h"

namespace BMV2 {

/**
    Return direct meter information from the direct meter map
*/
DirectMeterMap::DirectMeterInfo* DirectMeterMap::createInfo(const IR::IDeclaration* meter) {
    auto prev = ::get(directMeter, meter);
    BUG_CHECK(prev == nullptr, "Already created");
    auto result = new DirectMeterMap::DirectMeterInfo();
    directMeter.emplace(meter, result);
    return result;
}

DirectMeterMap::DirectMeterInfo* DirectMeterMap::getInfo(const IR::IDeclaration* meter) {
    return ::get(directMeter, meter);
}

/**
    Set the table that a direct meter is attached to.
*/
void DirectMeterMap::setTable(const IR::IDeclaration* meter, const IR::P4Table* table) {
    auto info = getInfo(meter);
    CHECK_NULL(info);
    if (info->table != nullptr)
        ::error("%1%: Direct meters cannot be attached to multiple tables %2% and %3%",
                meter, table, info->table);
    info->table = table;
}

/**
    Helper function to check if two expressions are the same
*/
static bool checkSame(const IR::Expression* expr0, const IR::Expression* expr1) {
    if (expr0->node_type_name() != expr1->node_type_name())
        return false;
    if (auto pe0 = expr0->to<IR::PathExpression>()) {
        auto pe1 = expr1->to<IR::PathExpression>();
        return pe0->path->name == pe1->path->name &&
               pe0->path->absolute == pe1->path->absolute;
    } else if (auto mem0 = expr0->to<IR::Member>()) {
        auto mem1 = expr1->to<IR::Member>();
        return checkSame(mem0->expr, mem1->expr) && mem0->member == mem1->member;
    }
    BUG("%1%: unexpected expression for meter destination", expr0);
}

/**
    Set the destination that a meter is attached to??
*/
void DirectMeterMap::setDestination(const IR::IDeclaration* meter,
                                    const IR::Expression* destination) {
    auto info = getInfo(meter);
    if (info == nullptr)
        info = createInfo(meter);
    if (info->destinationField == nullptr) {
        info->destinationField = destination;
    } else {
        bool same = checkSame(destination, info->destinationField);
        if (!same)
            ::error("On this target all meter operations must write to the same destination "
                    "but %1% and %2% are different", destination, info->destinationField);
    }
}

/**
    Set the size of the table that a meter is attached to.

    @param meter
    @param size
*/
void DirectMeterMap::setSize(const IR::IDeclaration* meter, unsigned size) {
    auto info = getInfo(meter);
    CHECK_NULL(info);
    info->tableSize = size;
}

void Backend::addMetaInformation() {
  auto meta = new Util::JsonObject();

  static constexpr int version_major = 2;
  static constexpr int version_minor = 7;
  auto version = mkArrayField(meta, "version");
  version->append(version_major);
  version->append(version_minor);

  meta->emplace("compiler", "https://github.com/p4lang/p4c");

  toplevel.emplace("__meta__", meta);
}

void Backend::addEnums(Util::JsonArray* enums) {
    CHECK_NULL(enumMap);
    for (const auto &pEnum : *enumMap) {
        auto enumName = pEnum.first->getName().name.c_str();
        auto enumObj = new Util::JsonObject();
        enumObj->emplace("name", enumName);
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

void Backend::addLocals() {
    // We synthesize a "header_type" for each local which has a struct type
    // and we pack all the scalar-typed locals into a scalarsStruct
    auto scalarFields = scalarsStruct->get("fields")->to<Util::JsonArray>();
    CHECK_NULL(scalarFields);

    LOG3("... structure " << structure.variables);
    for (auto v : structure.variables) {
        LOG3("Creating local " << v);
        auto type = backend->getTypeMap()->getType(v, true);
        if (auto st = type->to<IR::Type_StructLike>()) {
            createJsonType(st);
            auto name = st->name;
            // create header instance?
            auto json = new Util::JsonObject();
            json->emplace("name", v->name);
            json->emplace("id", nextId("headers"));
            json->emplace("header_type", name);
            json->emplace("metadata", true);
            json->emplace("pi_omit", true);  // Don't expose in PI.
            headerInstances->append(json);
        } else if (auto stack = type->to<IR::Type_Stack>()) {
            auto json = new Util::JsonObject();
            json->emplace("name", v->name);
            json->emplace("id", nextId("stack"));
            json->emplace("size", stack->getSize());
            auto type = backend->getTypeMap()->getTypeType(stack->elementType, true);
            BUG_CHECK(type->is<IR::Type_Header>(), "%1% not a header type", stack->elementType);
            auto ht = type->to<IR::Type_Header>();
            createJsonType(ht);

            cstring header_type = stack->elementType->to<IR::Type_Header>()->name;
            json->emplace("header_type", header_type);
            auto stackMembers = mkArrayField(json, "header_ids");
            for (unsigned i=0; i < stack->getSize(); i++) {
                unsigned id = nextId("headers");
                stackMembers->append(id);
                auto header = new Util::JsonObject();
                cstring name = v->name + "[" + Util::toString(i) + "]";
                header->emplace("name", name);
                header->emplace("id", id);
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
                continue;
            } else if (mi->is<P4::ExternMethod>()) {
                auto em = mi->to<P4::ExternMethod>();
#ifdef PSA
                    genExternMethod(result, em);
#else
                    LOG1("P4V1:: convert " << s);
                    P4V1::V1Model::convertExternObjects(result, this, em, mc);
#endif
                continue;
            } else if (mi->is<P4::ExternFunction>()) {
                auto ef = mi->to<P4::ExternFunction>();
                auto primitive = mkPrimitive(ef->method->name.name, result);
                auto parameters = mkParameters(primitive);
                for (auto a : *mc->arguments) {
                    parameters->append(conv->convert(a));
                }
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
        structure.ids.emplace(action, id);
        jact->emplace("id", id);
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

/// BMV2 Backend that takes the top level block and converts it to a JsonObject.
void Backend::run(const IR::ToplevelBlock* tb) {
    headerTypes = mkArrayField(&toplevel, "header_types");
    headerInstances = mkArrayField(&toplevel, "headers");
    headerStacks = mkArrayField(&toplevel, "header_stacks");
    parsers = mkArrayField(&toplevel, "parsers");
    pipelines = mkArrayField(&toplevel, "pipelines");
    deparsers = mkArrayField(&toplevel, "deparsers");
    actions = mkArrayField(&toplevel, "actions");
    externs = mkArrayField(&toplevel, "extern_instances");
    errors = mkArrayField(&toplevel, "errors");
    enums = mkArrayField(&toplevel, "enums");

    /// backward compatible with bm2-ss
    calculations = mkArrayField(&toplevel, "calculations");
    checksums = mkArrayField(&toplevel, "checksums");
    counters = mkArrayField(&toplevel, "counter_arrays");
    field_lists = mkArrayField(&toplevel, "field_lists");
    learn_lists = mkArrayField(&toplevel, "learn_lists");
    meter_arrays = mkArrayField(&toplevel, "meter_arrays");
    register_arrays = mkArrayField(&toplevel, "register_arrays");

    PassManager processing_passes = {
        new P4::TypeChecking(&refMap, &typeMap),
        new DiscoverStructure(&structure),
        new ErrorCodesVisitor(errors, &errorCodesMap),
    };
    tb->getProgram()->apply(processing_passes);

    // This visitor is used in multiple passes to convert expression to json
    conv = new ExpressionConverter(&refMap, &typeMap, &structure, &errorCodesMap);

    PassManager codegen_passes = {
        new VisitFunctor([this]() { addMetaInformation(); }),
        new VisitFunctor([this]() { addEnums(enums); }),
        new VisitFunctor([this](){ createScalars(); }),
        new ConvertHeaders(this),
        new VisitFunctor([this](){ addLocals(); }),
        new VisitFunctor([this](){ padScalars(); }),
        new ConvertExterns(&refMap, &typeMap, conv, externs),
        new ConvertParser(&refMap, &typeMap, conv, parsers),
        new ConvertControl(&refMap, &typeMap, conv, &structure, pipelines, counters),
        new ConvertDeparser(&refMap, &typeMap, conv, deparsers),
        new VisitFunctor([this]() { createActions(actions); }),
    };
    //dump(tb->getProgram());
    //dump(tb->getMain());
    tb->getMain()->apply(codegen_passes);
}

} // namespace BMV2
