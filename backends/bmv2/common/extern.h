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

#ifndef BACKENDS_BMV2_COMMON_EXTERN_H_
#define BACKENDS_BMV2_COMMON_EXTERN_H_

#include "frontends/p4/methodInstance.h"
#include "helpers.h"
#include "ir/ir.h"
#include "programStructure.h"

namespace BMV2 {

class ExternConverter {
    static std::map<cstring, ExternConverter*> *cvtForType;

 public:
    static void registerExternConverter(cstring, ExternConverter*);

    virtual Util::IJson*
    convertExternObject(ConversionContext* ctxt, const P4::ExternMethod* em,
                        const IR::MethodCallExpression* mc, const IR::StatOrDecl* s,
                        const bool& emitExterns);
    virtual void
    convertExternInstance(ConversionContext* ctxt, const IR::Declaration* c,
                          const IR::ExternBlock* eb, const bool& emitExterns);
    virtual Util::IJson*
    convertExternFunction(ConversionContext* ctxt, const P4::ExternFunction* ef,
                          const IR::MethodCallExpression* mc, const IR::StatOrDecl* s,
                          const bool emitExterns);

    static ExternConverter* get(cstring type);
    static ExternConverter* get(const IR::Type_Extern* type) { return get(type->name); }
    static ExternConverter* get(const IR::ExternBlock* eb) { return get(eb->type); }
    static ExternConverter* get(const P4::ExternFunction* ef) { return get(ef->method->name); }
    static ExternConverter* get(const P4::ExternMethod* em) {
        return get(em->originalExternType->name); }

    static Util::IJson*
    cvtExternObject(ConversionContext* ctxt, const P4::ExternMethod* em,
                    const IR::MethodCallExpression* mc, const IR::StatOrDecl* s,
                    const bool& emitExterns);
    static void
    cvtExternInstance(ConversionContext* ctxt, const IR::Declaration* c,
                      const IR::ExternBlock* eb,
                      const bool& emitExterns);
    static Util::IJson*
    cvtExternFunction(ConversionContext* ctxt, const P4::ExternFunction* ef,
                      const IR::MethodCallExpression* mc, const IR::StatOrDecl* s,
                      const bool emitExterns);

    // helper function for simple switch
    void modelError(const char* format, const IR::Node* place) const;
    void addToFieldList(ConversionContext* ctxt, const IR::Expression* expr, Util::JsonArray* fl);
    int createFieldList(ConversionContext* ctxt, const IR::Expression* expr, cstring group,
                        cstring listName, Util::JsonArray* field_lists);
    cstring createCalculation(ConversionContext* ctxt, cstring algo, const IR::Expression* fields,
                              Util::JsonArray* calculations, bool usePayload, const IR::Node* node);
    static cstring convertHashAlgorithm(cstring algorithm);
};

#define EXTERN_CONVERTER_W_FUNCTION_AND_MODEL(extern_name, model_type, model_name)  \
    class ExternConverter_##extern_name : public ExternConverter {              \
        model_type&  model_name;                                                \
        ExternConverter_##extern_name() :                                       \
            model_name(model_type::instance) {                                  \
            registerExternConverter(#extern_name, this); }                      \
        static ExternConverter_##extern_name singleton;                         \
        Util::IJson* convertExternFunction(ConversionContext* ctxt,             \
            const P4::ExternFunction* ef, const IR::MethodCallExpression* mc,   \
            const IR::StatOrDecl* s, const bool emitExterns) override;  };

#define EXTERN_CONVERTER_W_FUNCTION(extern_name)                                \
    class ExternConverter_##extern_name : public ExternConverter {              \
        ExternConverter_##extern_name() {                                       \
            registerExternConverter(#extern_name, this); }                      \
        static ExternConverter_##extern_name singleton;                         \
        Util::IJson* convertExternFunction(ConversionContext* ctxt,             \
            const P4::ExternFunction* ef, const IR::MethodCallExpression* mc,   \
            const IR::StatOrDecl* s, const bool emitExterns) override;  };

#define EXTERN_CONVERTER_W_INSTANCE_AND_MODEL(extern_name, model_type, model_name) \
    class ExternConverter_##extern_name : public ExternConverter {              \
        model_type&      model_name;                                            \
        ExternConverter_##extern_name() :                                       \
            model_name(model_type::instance) {                                  \
            registerExternConverter(#extern_name, this); }                      \
        static ExternConverter_##extern_name singleton;                         \
        void convertExternInstance(ConversionContext* ctxt,                     \
            const IR::Declaration* c, const IR::ExternBlock* eb,                \
            const bool& emitExterns) override; };

#define EXTERN_CONVERTER_W_INSTANCE(extern_name)                                \
    class ExternConverter_##extern_name : public ExternConverter {              \
        ExternConverter_##extern_name() {                                       \
            registerExternConverter(#extern_name, this); }                      \
        static ExternConverter_##extern_name singleton;                         \
        void convertExternInstance(ConversionContext* ctxt,                     \
            const IR::Declaration* c, const IR::ExternBlock* eb,                \
            const bool& emitExterns) override; };

#define EXTERN_CONVERTER_W_OBJECT_AND_INSTANCE_AND_MODEL(extern_name, type, name)  \
    class ExternConverter_##extern_name : public ExternConverter {              \
        type&      name;                                                        \
        ExternConverter_##extern_name() :                                       \
            name(type::instance) {                                              \
            registerExternConverter(#extern_name, this); }                      \
        static ExternConverter_##extern_name singleton;                         \
        void convertExternInstance(ConversionContext* ctxt,                     \
            const IR::Declaration* c, const IR::ExternBlock* eb,                \
            const bool& emitExternes) override;                                 \
        Util::IJson* convertExternObject(ConversionContext* ctxt,               \
            const P4::ExternMethod* em, const IR::MethodCallExpression* mc,     \
            const IR::StatOrDecl* s, const bool& emitExterns) override; };

#define EXTERN_CONVERTER_W_OBJECT_AND_INSTANCE(extern_name)                     \
    class ExternConverter_##extern_name : public ExternConverter {              \
        ExternConverter_##extern_name() {                                       \
            registerExternConverter(#extern_name, this); }                      \
        static ExternConverter_##extern_name singleton;                         \
        void convertExternInstance(ConversionContext* ctxt,                     \
            const IR::Declaration* c, const IR::ExternBlock* eb,                \
            const bool& emitExterns) override;                                  \
        Util::IJson* convertExternObject(ConversionContext* ctxt,               \
            const P4::ExternMethod* em, const IR::MethodCallExpression* mc,     \
            const IR::StatOrDecl* s, const bool& emitExterns) override; };

}  // namespace BMV2

#endif  /* BACKENDS_BMV2_COMMON_EXTERN_H_ */
