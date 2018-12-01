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

#define PARSE_NO_BODY(aname) \
    { aname, &P4::ParseAnnotations::parseNoBody }

#define PARSE(aname, tname)                                             \
    { aname, [](IR::Annotation* annotation) {                           \
            if (!P4::ParseAnnotations::needsParsing(annotation)) {      \
                return;                                                 \
            }                                                           \
                                                                        \
            const IR::tname* parsed =                                   \
                P4::P4ParserDriver::parse ## tname(annotation->srcInfo, \
                                                   annotation->body);   \
            if (parsed != nullptr) {                                    \
                annotation->expr.push_back(parsed);                     \
            }                                                           \
        }                                                               \
    }

#define PARSE_EXPRESSION_LIST(aname) \
    { aname, &P4::ParseAnnotations::parseExpressionList }

#define PARSE_KV_LIST(aname) \
    { aname, &P4::ParseAnnotations::parseKvList }

class ParseAnnotations : public Modifier {
 public:
    using Modifier::postorder;

    typedef std::function<void(IR::Annotation*)> Handler;

    /// Keyed on annotation names.
    typedef std::unordered_map<cstring, Handler> HandlerMap;

    /// Produces a pass that rewrites the spec-defined annotations.
    ParseAnnotations() : handlers(standardHandlers()) {
        setName("ParseAnnotations");
    }

    /// Produces a pass that rewrites a custom set of annotations.
    ParseAnnotations(const char* targetName, HandlerMap handlers)
            : handlers(handlers) {
        std::string buf = targetName;
        buf += "__ParseAnnotations";
        setName(buf.c_str());
    }

    void postorder(IR::Annotation* annotation) final;

    static HandlerMap standardHandlers();

    static void parseNoBody(IR::Annotation* annotation);
    static void parseExpressionList(IR::Annotation* annotation);
    static void parseKvList(IR::Annotation* annotation);

    /// Checks if the annotation needs parsing. An annotation needs parsing if
    /// it was derived from P4₁₆. A BUG is thrown if the annotation is derived
    /// from P4₁₆ and is already parsed.
    static bool needsParsing(IR::Annotation* annotation);

 private:
    HandlerMap handlers;
};

}  // namespace P4

#endif /* _P4_PARSEANNOTATIONS_H_ */
