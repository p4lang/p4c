/*
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

#include "annotations.h"

#include "ir/ir.h"
#include "lib/algorithm.h"
#include "lib/cstring.h"

namespace P4::IR {

const cstring IR::Annotation::nameAnnotation = "name"_cs;
const cstring IR::Annotation::tableOnlyAnnotation = "tableonly"_cs;
const cstring IR::Annotation::defaultOnlyAnnotation = "defaultonly"_cs;
const cstring IR::Annotation::atomicAnnotation = "atomic"_cs;
const cstring IR::Annotation::hiddenAnnotation = "hidden"_cs;
const cstring IR::Annotation::lengthAnnotation = "length"_cs;
const cstring IR::Annotation::maxLengthAnnotation = "max_length"_cs;
const cstring IR::Annotation::optionalAnnotation = "optional"_cs;
const cstring IR::Annotation::pkginfoAnnotation = "pkginfo"_cs;
const cstring IR::Annotation::deprecatedAnnotation = "deprecated"_cs;
const cstring IR::Annotation::synchronousAnnotation = "synchronous"_cs;
const cstring IR::Annotation::pureAnnotation = "pure"_cs;
const cstring IR::Annotation::noSideEffectsAnnotation = "noSideEffects"_cs;
const cstring IR::Annotation::noWarnAnnotation = "noWarn"_cs;
const cstring IR::Annotation::matchAnnotation = "match"_cs;
const cstring IR::Annotation::fieldListAnnotation = "field_list"_cs;
const cstring IR::Annotation::debugLoggingAnnotation = "__debug"_cs;
const cstring IR::Annotation::disableOptimizationAnnotation = "disable_optimization"_cs;

namespace Annotations {
void addIfNew(Vector<Annotation> &annotations, cstring name, const Expression *expr,
              bool structured) {
    if (get(annotations, name) != nullptr) return;

    annotations.emplace_back(name, expr, structured);
}

void addIfNew(Vector<Annotation> &annotations, const IR::Annotation *ann) {
    if (get(annotations, ann->name) != nullptr) return;

    annotations.push_back(ann);
}

void addOrReplace(Vector<Annotation> &annotations, cstring name, const Expression *expr,
                  bool structured) {
    remove_if(annotations, [name](const Annotation *a) -> bool { return a->name == name; });
    annotations.emplace_back(name, expr, structured);
}

void addOrReplace(Vector<Annotation> &annotations, const IR::Annotation *ann) {
    remove_if(annotations, [ann](const Annotation *a) -> bool { return a->name == ann->name; });
    annotations.push_back(ann);
}

Vector<Annotation> maybeAddNameAnnotation(const Vector<Annotation> &annos, cstring name) {
    auto result = annos;
    Annotations::addIfNew(result, IR::Annotation::nameAnnotation, new IR::StringLiteral(name));
    return result;
}

Vector<Annotation> setNameAnnotation(const Vector<Annotation> &annos, cstring name) {
    auto result = annos;
    Annotations::addOrReplace(result, IR::Annotation::nameAnnotation, new IR::StringLiteral(name));
    return result;
}

Vector<Annotation> withoutNameAnnotation(const Vector<Annotation> &annos) {
    return annos.where(
        [](const IR::Annotation *a) { return a->name != IR::Annotation::nameAnnotation; });
}
}  // namespace Annotations

}  // namespace P4::IR
