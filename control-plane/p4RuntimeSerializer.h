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
#include <unordered_map>

#include "lib/cstring.h"

namespace p4 {
namespace config {
namespace v1 {
class P4Info;
}  // namespace v1
}  // namespace config
namespace v1 {
class WriteRequest;
}  // namespace v1
}  // namespace p4

namespace IR {
class P4Program;
}  // namespace IR

class CompilerOptions;

namespace P4 {

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
    void serializeP4InfoTo(std::ostream* destination, P4RuntimeFormat format);
    /// Serialize the WriteRequest message containing all the table entries to
    /// the @destination stream in the requested protobuf serialization @format.
    void serializeEntriesTo(std::ostream* destination, P4RuntimeFormat format);

    /// A P4Runtime P4Info message, which encodes the control-plane API of the
    /// program. Never null.
    const ::p4::config::v1::P4Info* p4Info;
    /// All static table entries as one P4Runtime WriteRequest object. Never
    /// null.
    const ::p4::v1::WriteRequest* entries;
};

namespace ControlPlaneAPI {
struct P4RuntimeArchHandlerBuilderIface;
}  // namespace ControlPlaneAPI

/// Public APIs to generate P4Info message. Uses the singleton pattern.
class P4RuntimeSerializer {
 public:
    static P4RuntimeSerializer* get();

    void registerArch(const cstring archName,
                      const ControlPlaneAPI::P4RuntimeArchHandlerBuilderIface* builder);

    /**
     * Generate a P4Runtime control-plane API for the provided program and
     * architecture.
     *
     * API generation never fails, but if errors are encountered in the program
     * some constructs may be excluded from the API. In this case, a program
     * error will be reported.
     *
     * @param program  The program to construct the control-plane API from. All
     *                 frontend passes must have already run.
     * @return the generated P4Runtime API.
     */
    P4RuntimeAPI generateP4Runtime(const IR::P4Program* program, cstring arch);

    /**
     * A convenience wrapper for P4::generateP4Runtime() which generates the
     * P4RuntimeAPI structure for the provided program and serializes it
     * according to the provided command-line options.
     *
     * @param program  The program to construct the control-plane API from. All
     *                 frontend passes must have already run.
     * @param options  The command-line options used to invoke the compiler.
     */
    void serializeP4RuntimeIfRequired(const IR::P4Program* program,
                                      const CompilerOptions& options);

    /// @return architecture name based on provided command-line @options and
    /// environment.
    static cstring resolveArch(const CompilerOptions& options);

 private:
    P4RuntimeSerializer();

    std::unordered_map<cstring, const ControlPlaneAPI::P4RuntimeArchHandlerBuilderIface*>
    archHandlerBuilders{};
};

/// Calls @ref P4RuntimeSerializer::generateP4Runtime on the @ref
/// P4RuntimeSerializer singleton.
P4RuntimeAPI generateP4Runtime(const IR::P4Program* program, cstring arch = "v1model");

/// Calls @ref P4RuntimeSerializer::serializeP4RuntimeIfRequired on the @ref
/// P4RuntimeSerializer singleton.
void serializeP4RuntimeIfRequired(const IR::P4Program* program,
                                  const CompilerOptions& options);

}  // namespace P4

#endif  /* CONTROL_PLANE_P4RUNTIMESERIALIZER_H_ */
