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
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/parsers/parserDriver.h"

/*
 * Parses known/predefined annotations used by the compiler.
 */
namespace P4 {

// A no-op handler. Useful for avoiding warnings about ignored annotations.
#define PARSE_SKIP(aname) \
    { aname, &P4::ParseAnnotations::parseSkip }

// Parses an empty annotation.
#define PARSE_EMPTY(aname) \
    { aname, &P4::ParseAnnotations::parseEmpty }

// Parses an annotation with a single-element body.
#define PARSE(aname, tname)                                             \
    { aname, [](IR::Annotation* annotation) {                           \
            const IR::tname* parsed =                                   \
                P4::P4ParserDriver::parse ## tname(annotation->srcInfo, \
                                                   annotation->body);   \
            if (parsed != nullptr) {                                    \
                annotation->expr.push_back(parsed);                     \
            }                                                           \
            return parsed != nullptr;                                   \
        }                                                               \
    }

// Parses an annotation whose body is a pair.
#define PARSE_PAIR(aname, tname)                                   \
    { aname, [](IR::Annotation* annotation) {                      \
            const IR::Vector<IR::Expression>* parsed =             \
                P4::P4ParserDriver::parse ## tname ## Pair(        \
                    annotation->srcInfo,                           \
                    annotation->body);                             \
            if (parsed != nullptr) {                               \
                annotation->expr.append(*parsed);                  \
            }                                                      \
            return parsed != nullptr;                              \
        }                                                          \
    }

// Parses an annotation whose body is a triple.
#define PARSE_TRIPLE(aname, tname)                                 \
    { aname, [](IR::Annotation* annotation) {                      \
            const IR::Vector<IR::Expression>* parsed =             \
                P4::P4ParserDriver::parse ## tname ## Triple(      \
                    annotation->srcInfo, annotation->body);        \
            if (parsed != nullptr) {                               \
                annotation->expr.append(*parsed);                  \
            }                                                      \
            return parsed != nullptr;                              \
        }                                                          \
    }

#define PARSE_EXPRESSION_LIST(aname) \
    { aname, &P4::ParseAnnotations::parseExpressionList }

#define PARSE_KV_LIST(aname) \
    { aname, &P4::ParseAnnotations::parseKvList }

class ParseAnnotations : public Modifier {
 public:
    using Modifier::postorder;

    /// A handler returns true when the body of the given annotation is parsed
    /// successfully.
    typedef std::function<bool(IR::Annotation*)> Handler;

    /// Keyed on annotation names.
    typedef std::unordered_map<cstring, Handler> HandlerMap;

    /// Produces a pass that rewrites the spec-defined annotations.
    explicit ParseAnnotations(bool warn = false)
            : warnUnknown(warn), handlers(standardHandlers()) {
        setName("ParseAnnotations");
    }

    /// Produces a pass that rewrites a custom set of annotations.
    ParseAnnotations(const char* targetName, bool includeStandard,
                     HandlerMap handlers,
                     bool warn = false)
            : warnUnknown(warn) {
        std::string buf = targetName;
        buf += "__ParseAnnotations";
        setName(buf.c_str());

        if (includeStandard) {
            this->handlers = standardHandlers();
            this->handlers.insert(handlers.begin(), handlers.end());
        } else {
            this->handlers = handlers;
        }
    }

    void postorder(IR::Annotation* annotation) final;

    static HandlerMap standardHandlers();

    static bool parseSkip(IR::Annotation* annotation);
    static bool parseEmpty(IR::Annotation* annotation);
    static bool parseExpressionList(IR::Annotation* annotation);
    static bool parseKvList(IR::Annotation* annotation);

    void addHandler(cstring name, Handler h) { handlers.insert({name, h}); }

 private:
    /// Whether to warn about unknown annotations.
    const bool warnUnknown;

    /// The set of unknown annotations for which warnings have already been
    /// made.
    std::set<cstring> warned;

    HandlerMap handlers;
};

/// Clears a type map after calling a ParseAnnotations instance.
class ParseAnnotationBodies final : public PassManager {
 public:
    ParseAnnotationBodies(ParseAnnotations* pa, TypeMap* typeMap) {
        passes.push_back(pa);
        passes.push_back(new ClearTypeMap(typeMap));
        setName("ParseAnnotationBodies");
    }
};

}  // namespace P4

#endif /* _P4_PARSEANNOTATIONS_H_ */
