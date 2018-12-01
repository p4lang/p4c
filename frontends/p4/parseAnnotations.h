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

#ifndef _P4_PARSEANNOTATIONS_H_
#define _P4_PARSEANNOTATIONS_H_

#include "ir/ir.h"
#include "frontends/parsers/parserDriver.h"

/*
 * Parses known/predefined annotations used by the compiler.
 */
namespace P4 {

#define PARSE_NO_BODY(aname)                                          \
    if (annotation->name == aname) {                                  \
        if (!annotation->body.empty()) {                              \
            ::error("%1% should not have any arguments", annotation); \
        }                                                             \
                                                                      \
        return;                                                       \
    }

#define PARSE(aname, tname)                                         \
    if (annotation->name == aname) {                                \
        if (!needsParsing(annotation)) {                            \
            return;                                                 \
        }                                                           \
                                                                    \
        const IR::tname* parsed =                                   \
            P4::P4ParserDriver::parse ## tname(annotation->srcInfo, \
                                               annotation->body);   \
        if (parsed != nullptr) {                                    \
            annotation->expr.push_back(parsed);                     \
        }                                                           \
        return;                                                     \
    }

#define PARSE_EXPRESSION_LIST(aname)                                     \
    if (annotation->name == aname) {                                     \
        if (!needsParsing(annotation)) {                                 \
            return;                                                      \
        }                                                                \
                                                                         \
        const IR::Vector<IR::Expression>* parsed =                       \
            P4::P4ParserDriver::parseExpressionList(annotation->srcInfo, \
                                                    annotation->body);   \
        if (parsed != nullptr) {                                         \
            annotation->expr.append(*parsed);                            \
        }                                                                \
        return;                                                          \
    }

#define PARSE_KV_LIST(aname)                                     \
    if (annotation->name == aname) {                             \
        if (!needsParsing(annotation)) {                         \
            return;                                              \
        }                                                        \
                                                                 \
        const IR::IndexedVector<IR::NamedExpression>* parsed =   \
            P4::P4ParserDriver::parseKvList(annotation->srcInfo, \
                                            annotation->body);   \
        if (parsed != nullptr) {                                 \
            annotation->kv.append(*parsed);                      \
        }                                                        \
        return;                                                  \
    }

class ParseAnnotations : public Modifier {
 public:
    using Modifier::postorder;
    ParseAnnotations() { setName("ParseAnnotations"); }
    explicit ParseAnnotations(const char* targetName) {
        cstring s = cstring(targetName) + cstring("__ParseAnnotations");
        setName(s);
    }
    void postorder(IR::Annotation* annotation) override;

 protected:
    /// Checks if the annotation needs parsing. An annotation needs parsing if
    /// it was derived from P4₁₆. A BUG is thrown if the annotation is derived
    /// from P4₁₆ and is already parsed.
    bool needsParsing(IR::Annotation* annotation);
};

}  // namespace P4

#endif /* _P4_PARSEANNOTATIONS_H_ */
