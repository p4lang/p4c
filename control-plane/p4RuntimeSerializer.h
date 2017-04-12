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

namespace IR {
class P4Program;
class ToplevelBlock;
}  // namespace IR

namespace P4 {

class ReferenceMap;
class TypeMap;

enum class P4RuntimeFormat {
  BINARY,
  JSON,
  TEXT
};

void serializeP4Runtime(std::ostream* destination,
                        const IR::P4Program* program,
                        const IR::ToplevelBlock* evaluatedProgram,
                        ReferenceMap* refMap,
                        TypeMap* typeMap,
                        P4RuntimeFormat format = P4RuntimeFormat::BINARY);

}  // namespace P4

#endif  /* CONTROL_PLANE_P4RUNTIMESERIALIZER_H_ */
