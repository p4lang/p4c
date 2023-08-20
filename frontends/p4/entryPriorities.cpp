/*
Copyright 2023 Mihai Budiu

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

#include "entryPriorities.h"

#include "coreLibrary.h"

namespace P4 {

bool DoEntryPriorities::requiresPriority(const IR::KeyElement *ke) const {
    // Check if the keys support priorities.  Note: since match_kind
    // is extensible, we treat any non-standard match kind as if it
    // may require priorities.  Back-ends should implement additional
    // checks if that's not true.
    auto path = ke->matchType->path;
    auto mt = refMap->getDeclaration(path, true)->to<IR::Declaration_ID>();
    BUG_CHECK(mt != nullptr, "%1%: could not find declaration", ke->matchType);
    if (mt->name.name == corelib.exactMatch.name || mt->name.name == corelib.lpmMatch.name)
        return false;
    return true;
}

const IR::Node *DoEntryPriorities::preorder(IR::EntriesList *entries) {
    auto table = findContext<IR::P4Table>();
    CHECK_NULL(table);
    auto ep = table->properties->getProperty(IR::TableProperties::entriesPropertyName);
    CHECK_NULL(ep);

    const IR::Entry *withPriority = nullptr;
    for (auto entry : entries->entries) {
        if (entry->priority) {
            withPriority = entry;
            break;
        }
    }

    if (ep->isConstant) {
        if (withPriority)
            ::error(ErrorType::ERR_INVALID,
                    "%1%: Table with 'const' entries cannot have priorities", withPriority);
        return entries;
    }

    bool largestWins = true;   // default value
    size_t priorityDelta = 1;  // default value
    auto largestProp = table->getBooleanProperty("largest_priority_wins");
    if (largestProp) largestWins = largestProp->value;
    auto deltaProp = table->getConstantProperty("priority_delta");
    if (deltaProp) {
        if (!deltaProp->fitsUint()) {
            ::error(ErrorType::ERR_INVALID, "%1% must be a positive value", deltaProp);
            return entries;
        }
        priorityDelta = deltaProp->asUnsigned();
    }

    size_t currentPriority = 1;
    if (withPriority == nullptr) {
        // no priorities specified
        for (size_t i = 0; i < entries->size(); ++i) {
            size_t index = largestWins ? entries->size() - i - 1 : i;
            auto entry = entries->entries.at(index);
            BUG_CHECK(entry->priority == nullptr, "%1%: Priority found?", entry);
            auto priority = new IR::Constant((uint64_t)currentPriority);
            auto newEntry = new IR::Entry(entry->srcInfo, entry->annotations, entry->isConst,
                                          priority, entry->keys, entry->action, entry->singleton);
            entries->entries[index] = newEntry;
            size_t nextPriority = currentPriority + priorityDelta;
            if (nextPriority < currentPriority) {
                ::error(ErrorType::ERR_OVERLIMIT, "%1% Overflow in priority computation", table);
                return entries;
            }
            currentPriority = nextPriority;
        }
        return entries;
    }

    // Some priorities specified.
    bool requiresPriorities = false;
    auto key = table->getKey();
    for (auto element : key->keyElements) {
        if (requiresPriority(element)) {
            requiresPriorities = true;
            break;
        }
    }
    if (!requiresPriorities) {
        ::error(ErrorType::ERR_UNSUPPORTED,
                "%1% key match type does not require priorities, but some are specified", key);
        return entries;
    }

    std::map<size_t, const IR::Entry *> usedPriorities;
    size_t previousPriority = 0;
    for (size_t i = 0; i < entries->size(); ++i) {
        auto entry = entries->entries.at(i);
        if (entry->priority) {
            if (auto value = entry->priority->to<IR::Constant>()) {
                currentPriority = value->asUnsigned();
            } else {
                ::error(ErrorType::ERR_INVALID, "%1% must be a constant", entry->priority);
                return entries;
            }
        } else {
            // First entry must have a priority.
            if (i == 0) {
                ::error(ErrorType::ERR_EXPECTED, "%1% entry must have a priority", entry);
                return entries;
            }
            auto priority = new IR::Constant((uint64_t)currentPriority);
            auto newEntry = new IR::Entry(entry->srcInfo, entry->annotations, entry->isConst,
                                          priority, entry->keys, entry->action, entry->singleton);
            entries->entries[i] = newEntry;
        }
        auto previous = usedPriorities.find(currentPriority);
        if (previous != usedPriorities.end()) {
            warn(ErrorType::WARN_DUPLICATE_PRIORITIES, "%1% and %2% have the same priority %3%",
                 previous->second, entries->entries[i], currentPriority);
        }
        usedPriorities.emplace(currentPriority, entries->entries[i]);

        size_t nextPriority;
        if (largestWins) {
            if (i > 0 && currentPriority > previousPriority) {
                warn(ErrorType::WARN_ENTRIES_OUT_OF_ORDER,
                     "%1% (priority %2%) and %3% (priority %4%) have priorities out of order",
                     entries->entries[i - 1], previousPriority, entries->entries[i],
                     currentPriority);
            }

            nextPriority = currentPriority - priorityDelta;
            if (nextPriority > currentPriority) {
                ::error(ErrorType::ERR_OVERLIMIT, "%1% Overflow in priority computation", table);
                return entries;
            }
        } else {
            if (i > 0 && currentPriority < previousPriority) {
                warn(ErrorType::WARN_ENTRIES_OUT_OF_ORDER,
                     "%1% (priority %2%) and %3% (priority %4%) have priorities out of order",
                     entries->entries[i - 1], previousPriority, entries->entries[i],
                     currentPriority);
            }

            nextPriority = currentPriority + priorityDelta;
            if (nextPriority < currentPriority) {
                ::error(ErrorType::ERR_OVERLIMIT, "%1% Overflow in priority computation", table);
                return entries;
            }
        }
        previousPriority = currentPriority;
        currentPriority = nextPriority;
    }
    return entries;
}

}  // namespace P4
