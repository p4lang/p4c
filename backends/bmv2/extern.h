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

#ifndef _BACKENDS_BMV2_EXTERN_H_
#define _BACKENDS_BMV2_EXTERN_H_

#include "ir/ir.h"
#include "frontends/p4/methodInstance.h"
#include "programStructure.h"

namespace BMV2 {

class ExternConverter {
    static std::map<cstring, ExternConverter*> *cvtForType;

 public:
    static void registerExternConverter(cstring, ExternConverter*);

    virtual Util::IJson* convertExternObject(ConversionContext* ctxt, const P4::ExternMethod* em,
                                             const IR::MethodCallExpression* mc, const IR::StatOrDecl* s);
    virtual Util::IJson* convertExternInstance(ConversionContext* ctxt, const IR::Declaration* c,
                                               const IR::ExternBlock* eb);
    virtual Util::IJson* convertExternFunction(ConversionContext* ctxt, const P4::ExternFunction* ef,
                                               const IR::MethodCallExpression* mc, const IR::StatOrDecl* s);

    static ExternConverter* get(cstring type);
    static ExternConverter* get(const IR::Type_Extern* type) { return get(type->name); }
    static ExternConverter* get(const IR::ExternBlock* eb) { return get(eb->type); }

    static Util::IJson* cvtExternObject(ConversionContext* ctxt, const P4::ExternMethod* em,
                                        const IR::MethodCallExpression* mc, const IR::StatOrDecl* s);
    static Util::IJson* cvtExternInstance(ConversionContext* ctxt, const IR::Declaration* c,
                                          const IR::ExternBlock* eb);
    static Util::IJson* cvtExternFunction(ConversionContext* ctxt, const P4::ExternFunction* ef,
                                          const IR::MethodCallExpression* mc, const IR::StatOrDecl* s);

    // helper function
    void modelError(const char* format, const IR::Node* place) const;
    void addToFieldList(ConversionContext* ctxt, const IR::Expression* expr, Util::JsonArray* fl);
    int createFieldList(ConversionContext* ctxt, const IR::Expression* expr, cstring group,
                        cstring listName, Util::JsonArray* field_lists);
    cstring createCalculation(ConversionContext* ctxt, cstring algo, const IR::Expression* fields,
                              Util::JsonArray* calculations, bool usePayload, const IR::Node* node);
    cstring convertHashAlgorithm(cstring algorithm);
};



#define EXTERN_CONVERTER_W_FUNCTION(extern_name, ...)                           \
    class ExternConverter_##extern_name##__VA_ARGS__ : public ExternConverter { \
        P4V1::V1Model&      v1model;                                            \
        ExternConverter_##extern_name##__VA_ARGS__() :                          \
            v1model(P4V1::V1Model::instance) {                                  \
            registerExternConverter("##extern_name##", this); }                 \
        static ExternConverter_##extern_name##__VA_ARGS__ singleton;            \
        Util::IJson* convertExternFunction(ConversionContext* ctxt,             \
            const P4::ExternFunction* ef, const IR::MethodCallExpression* mc,   \
            const IR::StatOrDecl* s) override;  };

#define EXTERN_CONVERTER_W_OBJECT(extern_name, ...)                            \
   class ExternConverter_##extern_name##__VA_ARGS__ : public ExternConverter { \
       P4V1::V1Model&      v1model;                                            \
       ExternConverter_##extern_name##__VA_ARGS__() :                          \
           v1model(P4V1::V1Model::instance) {                                  \
           registerExternConverter("##extern_name##", this); }                 \
       static ExternConverter_##extern_name##__VA_ARGS__ singleton;            \
       Util::IJson* convertExternObject(ConversionContext* ctxt,                  \
           const P4::ExternMethod* em, const IR::MethodCallExpression* mc,     \
           const IR::StatOrDecl* s) override; };

#define EXTERN_CONVERTER_W_INSTANCE(extern_name, ...)                          \
   class ExternConverter_##extern_name##__VA_ARGS__ : public ExternConverter { \
       P4V1::V1Model&      v1model;                                            \
       ExternConverter_##extern_name##__VA_ARGS__() :                          \
           v1model(P4V1::V1Model::instance) {                                  \
           registerExternConverter("##extern_name##", this); }                 \
       static ExternConverter_##extern_name##__VA_ARGS__ singleton;            \
       Util::IJson* convertExternInstance(ConversionContext* ctxt,                \
           const IR::Declaration* c, const IR::ExternBlock* eb) override; };

#define EXTERN_CONVERTER_W_OBJECT_AND_INSTANCE(extern_name, ...)               \
   class ExternConverter_##extern_name##__VA_ARGS__ : public ExternConverter { \
       P4V1::V1Model&      v1model;                                            \
       ExternConverter_##extern_name##__VA_ARGS__() :                          \
           v1model(P4V1::V1Model::instance) {                                  \
           registerExternConverter("##extern_name##", this); }                 \
       static ExternConverter_##extern_name##__VA_ARGS__ singleton;            \
       Util::IJson* convertExternInstance(ConversionContext* ctxt,                \
           const IR::Declaration* c, const IR::ExternBlock* eb) override;      \
       Util::IJson* convertExternObject(ConversionContext* ctxt,                  \
           const P4::ExternMethod* em, const IR::MethodCallExpression* mc,     \
           const IR::StatOrDecl* s) override; };                               \

#define EXTERN_CONVERTER_SINGLETON(extern_name, ...)                           \
    ExternConverter_##extern_name##__VA_ARGS__                                 \
       ExternConverter_##extern_name##__VA_ARGS__::singleton;

#define CONVERT_EXTERN_INSTANCE(extern_name, ...)                                         \
    Util::IJson* ExternConverter_##extern_name##__VA_ARGS__::convertExternInstance(       \
        ConversionContext* ctxt, const IR::Declaration* c,                            \
        const IR::ExternBlock* eb)


#define CONVERT_EXTERN_OBJECT(extern_name, ...)                                           \
    Util::IJson* ExternConverter_##extern_name##__VA_ARGS__::convertExternObject(         \
        ConversionContext* ctxt, const P4::ExternMethod* em,                          \
        const IR::MethodCallExpression* mc, const IR::StatOrDecl *s)


#define CONVERT_EXTERN_FUNCTION(extern_name, ...)                                         \
    Util::IJson* ExternConverter_##extern_name##__VA_ARGS__::convertExternFunction(       \
        ConversionContext* ctxt, const P4::ExternFunction* ef,                        \
        const IR::MethodCallExpression* mc, const IR::StatOrDecl* s)

}  // namespace BMV2

#endif  /* _BACKENDS_BMV2_EXTERN_H_ */
