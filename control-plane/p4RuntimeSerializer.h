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

#ifndef CONTROL_PLANE_P4RUNTIMESERIALIZER_H_
#define CONTROL_PLANE_P4RUNTIMESERIALIZER_H_

#include <iosfwd>

namespace p4 {
namespace config {
class P4Info;
}  // namespace config
}  // namespace p4

namespace IR {
class P4Program;
class ToplevelBlock;
}  // namespace IR

namespace P4 {

class ReferenceMap;
class TypeMap;

/// P4Runtime serialization formats.
enum class P4RuntimeFormat {
  BINARY,
  JSON,
  TEXT
};

/// A P4 program's control-plane API, represented in terms of P4Runtime's data
/// structures. Can be inspected or serialized.
struct P4RuntimeAPI {
    /// Serialize this control-plane API to the provided output stream, using
    /// the given serialization format.
    void serializeTo(std::ostream* destination, P4RuntimeFormat format);

    /// A P4Runtime P4Info message, which encodes the control-plane API of the
    /// program. Never null.
    const p4::config::P4Info* p4Info;
};

/**
 * Generate a P4Runtime control-plane API for the provided program.
 *
 * API generation never fails, but if errors are encountered in the program some
 * constructs may be excluded from the API. In this case, a program error will
 * be reported.
 *
 * XXX(seth): Ideally this should only depend on the frontend, but currently
 * some midend passes are also required. We depend on at least EliminateTuples,
 * SynthesizeValidField, and LocalizeAllActions.  That list is probably not
 * exhaustive.
 *
 * @param program  The program to construct the control-plane API from. All
 *                 frontend passes must have already run.
 * @param evaluatedProgram  An up-to-date version of the program with
 *                          compile-time evaluation performed.
 * @param refMap  An up-to-date reference map.
 * @param typeMap An up-to-date type map.
 * @return the generated P4Runtime API.
 */
P4RuntimeAPI generateP4Runtime(const IR::P4Program* program,
                               const IR::ToplevelBlock* evaluatedProgram,
                               ReferenceMap* refMap,
                               TypeMap* typeMap);

}  // namespace P4

#endif  /* CONTROL_PLANE_P4RUNTIMESERIALIZER_H_ */
