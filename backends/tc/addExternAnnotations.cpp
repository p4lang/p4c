#include "addExternAnnotations.h"

namespace TC {

void AddExternAnnotations::GetExternStructures() {
    // Add structure definition for Register control Path
    safe_vector<struct Extern_field *> fields = {
        new struct Extern_field("index", "tc_type", 32),
        new struct Extern_field("value", "tc_data", 32),
        };
    struct PNA_extern_struct* estruct = new struct PNA_extern_struct("tc_ControlPath_Register", fields);
    new_pna_extern_structs.push_back(estruct);
    /* TODO : Add structure definition for other PNA externs*/
}

/* Get and add new structures for extern control path in P4 Program */
const IR::Node* AddExternAnnotations::postorder(IR::P4Program *program) {
    auto new_objs = new IR::Vector<IR::Node>;
    GetExternStructures();
    for (auto st : new_pna_extern_structs) {
        IR::IndexedVector<IR::StructField> fields;
        for (auto field : st->fields) {
            auto *annotations = new IR::Annotations({new IR::Annotation(IR::ID(field->annotation), {})});
            auto new_field = new IR::StructField(IR::ID(field->field_name), annotations, IR::Type_Bits::get(field->size));
            fields.push_back(new_field);
        }
        auto new_struct = new IR::Type_Struct(IR::ID(st->struct_name), fields);
        new_objs->push_back(new_struct);
    }
    for (auto obj : program->objects) {
        new_objs->push_back(obj);
    }
    program->objects = *new_objs;
    return program;
}

/* Annotate all PNA externs for P4TC*/
const IR::Node* AddExternAnnotations::postorder(IR::Type_Extern *te) {
    auto type_extern_name = te->name;
    /*
    extern Register<T, S> {
        Register(@tc_numel bit<32> size);
        Register(@tc_numel bit<32> size, T initial_value);
        @tc_md_read T read(@tc_key S index);
        @tc_md_write void write(@tc_key S index, @tc_data T value);
        @tc_ControlPath {
            @tc_key bit<32> index;
            @tc_data T value;
        }
    } */
    if (type_extern_name == "Register") {
        auto methods = new IR::Vector<IR::Method>();
        for (auto method : te->methods) {
            auto annotations = method->annotations;
            auto type = method->type;
            if(method->name == "read") {
                annotations = new IR::Annotations({new IR::Annotation(IR::ID("tc_md_read"), {})});
                type = GetAnnotatedType(method->type);
            } else if (method->name == "write") {
                new IR::Annotations({new IR::Annotation(IR::ID("tc_md_write"), {})});
                type = GetAnnotatedType(method->type);
            } else if (method->name == "Register") {
                type = GetAnnotatedType(method->type);
            }
            method =
                new IR::Method(method->srcInfo, method->name, type,
                               method->isAbstract, annotations);
            methods->push_back(method);
        }
        auto new_te = new IR::Type_Extern(te->srcInfo, te->name, te->typeParameters, *methods, te->annotations);
        return new_te;
    }
    /* TODO: Annotate other PNA externs*/
    return te;
}

/* Annotate extern method parameters*/
const IR::Type_Method* AddExternAnnotations::GetAnnotatedType(const IR::Type_Method *type) {
    auto *paramList = new IR::ParameterList();
    for (size_t paramID = 0; paramID < type->parameters->parameters.size(); ++paramID) {
        auto *param = type->parameters->getParameter(paramID)->clone();
        const auto *annos = param->getAnnotations();
        auto *newAnnos = annos->clone();
        if (param->name == "index") {
            newAnnos->add(new IR::Annotation(IR::ID("tc_key"), {}));
        } else if (param->name == "value") {
            newAnnos->add(new IR::Annotation(IR::ID("tc_data"), {}));
        } else if (param->name == "size") {
            newAnnos->add(new IR::Annotation(IR::ID("tc_numel"), {}));
        }
        param->annotations = newAnnos;
        paramList->push_back(param);
    }
    auto *new_type = new IR::Type_Method(type->getSourceInfo(), type->typeParameters, type->returnType, paramList, type->name);
    return new_type;
}

}