/*******************************************************************************
 *  Copyright (C) 2024 Intel Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions
 *  and limitations under the License.
 *
 *
 *  SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/

#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_RENAME_KEYS_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_RENAME_KEYS_H_

#include <ir/pass_manager.h>

#include <regex>  // NOLINT

#include "ir/ir.h"
#include "lib/cstring.h"

#include "backends/p4tools/modules/testgen/options.h"

namespace P4::P4Tools::P4Testgen::Tofino {

/** Implements a pass that -
 * 1. prunes all valid key matches with trailing '$'.
 * 2. replaces header stack indices hdr[<index>] with hdr$<index>.
 *
 * 1. Frontend converts `isValid()` calls on headers and header unions
 * to `$valid$` (note the trailing "$") as expected by P4Runtime.
 * However, bf-p4c and Bfruntime use "$valid".
 *
 * 2. The context.json has header stack indices as hdr$<index.
 * However, frontend has them as hdr[<index>].
 *
 * @pre: None
 *
 * @post: Ensure that
 *   - key matches with header stack indices have the subscript
 *     operator replaced with a '$'.
 *   - valid key matches have the trailing '$' removed.
 *   - Header stack indices with hdr[<index>] are replaced with hdr$<index>
 *   - For STF tests, slices are removed.
 *
 */
class ProcessKeyElems : public Transform {
 public:
    ProcessKeyElems() { setName("ProcessKeyElems"); }

    /// Check if there is a header stack index field matching
    /// the format hdr[<index>], and replace with hdr$<index>.
    /// Check if there is a `$valid$` key match and remove the
    /// trailing '$'.
    IR::Node *postorder(IR::KeyElement *keyElem) override {
        const auto *nameAnnot = keyElem->getAnnotation(IR::Annotation::nameAnnotation);
        CHECK_NULL(nameAnnot);
        cstring keyName = nameAnnot->getName();

        auto isReplaced = false;

        const auto *foundValid = keyName.find(".$valid");
        if (foundValid != nullptr) {
            // Remove the trailing '$'.
            keyName = keyName.exceptLast(1);
            isReplaced = true;
        }

        // Replace header stack indices hdr[<index>] with hdr$<index>. This
        // is output in the context.json
        std::regex hdrStackRegex(R"(\[([0-9]+)\])");
        auto indexName = std::regex_replace(keyName.c_str(), hdrStackRegex, "$$$1");
        if (indexName != keyName.c_str()) {
            keyName = cstring::to_cstring(indexName);
            isReplaced = true;
        }

        if (isReplaced) {
            ::warning("Replacing invalid key %1% with %2% for key %3%. ", keyName, keyName,
                      keyElem);
            // Update the key's annotation with the new fieldName.
            keyElem->addOrReplaceAnnotation(IR::Annotation::nameAnnotation,
                                            new IR::StringLiteral(keyName));
            return keyElem;
        }

        return keyElem;
    }
};

/// Check for a @ternary annotation attached to the parent table.
/// Rewrite every exact match key to be a ternary key.
class ProcessAnnotatedTables : public Transform {
    class ReplaceExactKeyMatches : public Transform {
     public:
        ReplaceExactKeyMatches() { setName("ReplaceExactKeyMatches"); }
        IR::KeyElement *preorder(IR::KeyElement *keyElem) override {
            const auto *keyElemMatchType = keyElem->matchType;
            if (keyElemMatchType->path->name == "exact") {
                auto *newMatchType = keyElemMatchType->clone();
                newMatchType->path = new IR::Path("ternary");
                keyElem->matchType = newMatchType;
            }
            return keyElem;
        }
    };

 public:
    ProcessAnnotatedTables() { setName("ProcessAnnotatedTables"); }
    const IR::Node *preorder(IR::P4Table *table) override {
        if (table->getAnnotation("ternary"_cs) != nullptr) {
            return table->apply(ReplaceExactKeyMatches());
        }
        return table;
    }
};

class RenameKeys : public PassManager {
 public:
    RenameKeys() {
        setName("RenameKeys");
        passes.emplace_back(new ProcessKeyElems());
        if (TestgenOptions::get().testBackend == "STF") {
            passes.emplace_back(new ProcessAnnotatedTables());
        }
    }
};

}  // namespace P4::P4Tools::P4Testgen::Tofino

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_RENAME_KEYS_H_ */
