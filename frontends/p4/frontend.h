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

#ifndef P4_FRONTEND_H_
#define P4_FRONTEND_H_

#include "../common/options.h"
#include "ir/ir.h"
#include "parseAnnotations.h"
#include "unusedDeclarations.h"

namespace P4 {

class ConstantFoldingPolicy;  // forward declare to avoid having to include

/// A customization point for frontend. The each tool can provide their own implementation of the
/// policy that customizes its behaviour, or use instance of this class directly to provide the
/// defaults.
class FrontEndPolicy : public RemoveUnusedPolicy {
 public:
    virtual ~FrontEndPolicy() = default;

    /// Get a vector of debug hooks that will be added to frontend and all the pass managers it
    /// uses internally.
    /// @returns Defaults to no hooks.
    virtual std::vector<DebugHook> getDebugHooks() const { return {}; }

    /// A specialized instance of annotations parser for the target, or nullptr to use the
    /// default configuration.
    /// @returns Defaults to nullptr.
    virtual ParseAnnotations *getParseAnnotations() const { return nullptr; }

    /// Indicates whether the side-effect-ordering pass should be skipped.
    /// @returns Defaults to false.
    // TODO: This should probably not be allowed to be skipped at all.
    virtual bool skipSideEffectOrdering() const { return false; }

    /// Indicates whether the frontend should run some optimizations (inlining, action localization,
    /// etc.).
    /// @returns default to enabled optimizations unless -O0 was given in the options (i.e. enabled
    /// unless options.optimizationLevel == 0).
    virtual bool optimize(const CompilerOptions &options) const {
        return options.optimizationLevel > 0;
    }

    /// Get policy for the constant folding pass. @see ConstantFoldingPolicy
    /// @returns Defaults to nullptr, which causes constant folding to use the default policy, which
    /// does not modify the pass defaults in any way.
    virtual ConstantFoldingPolicy *getConstantFoldingPolicy() const { return nullptr; }
};

class FrontEnd {
    FrontEndPolicy *policy;
    std::vector<DebugHook> hooks;

 public:
    FrontEnd() : FrontEnd(new FrontEndPolicy()) {}
    explicit FrontEnd(FrontEndPolicy *policy) : policy(policy), hooks(policy->getDebugHooks()) {}

    void addDebugHook(const DebugHook &hook) { hooks.push_back(hook); }
    // If p4c is run with option '--listFrontendPasses', outStream is used for printing passes names
    const IR::P4Program *run(const CompilerOptions &options, const IR::P4Program *program,
                             std::ostream *outStream = nullptr);
};

}  // namespace P4

#endif /* P4_FRONTEND_H_ */
