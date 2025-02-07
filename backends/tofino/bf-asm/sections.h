/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BF_ASM_SECTIONS_H_
#define BF_ASM_SECTIONS_H_

#include <stdarg.h>

#include <string>

#include "asm-types.h"
#include "bfas.h"
#include "json.h"
#include "map.h"

/// A Section represents a top level section in assembly
/// Current sections include:
/// version, phv, parser, deparser, stage, dynhash, primitives
class Section : virtual public Parsable, virtual public Contextable {
    static std::map<std::string, Section *> *sections;
    std::string name;
    bool isInput = false;
    static Section *get(const char *name) { return ::get(sections, name); }

 protected:
    explicit Section(const char *name_) : name(name_) {
        if (!sections) sections = new std::map<std::string, Section *>();
        if (get(name_)) {
            fprintf(stderr, "Duplicate section handler for %s\n", name_);
            exit(1);
        }
        (*sections)[name] = this;
    }
    virtual ~Section() {
        sections->erase(name);
        if (sections->empty()) {
            delete sections;
            sections = 0;
        }
    }
    /// process the arguments on the same line as the heading
    virtual void start(int lineno, VECTOR(value_t) args) {}
    /// optionally process the data if not done during parsing
    virtual void process() {}

 public:
    static int start_section(int lineno, char *name, VECTOR(value_t) args) {
        if (Section *sec = get(name)) {
            int prev_error_count = error_count;
            sec->isInput = true;
            sec->start(lineno, args);
            return error_count > prev_error_count;
        } else {
            warning(lineno, "Unknown section %s, ignoring\n", name);
            return 1;
        }
    }
    static void asm_section(char *name, VECTOR(value_t) args, value_t data) {
        if (Section *sec = get(name)) sec->input(args, data);
    }
    static void process_all() {
        if (sections)
            for (auto &it : *sections) it.second->process();
    }
    static void output_all(json::map &ctxtJson) {
        if (sections) {
            for (auto &it : *sections) {
                // Skip primitives to be called last
                if (it.first == "primitives") continue;
                it.second->output(ctxtJson);
            }
            auto &s = *sections;
            if (s.count("primitives")) s["primitives"]->output(ctxtJson);
        }
    }
    static bool no_sections_in_assembly() {
        if (sections) {
            for (auto &it : *sections) {
                if (it.second->isInput) return false;
            }
        }
        return true;
    }
    static bool section_in_assembly(const char *name) { return get(name)->isInput; }

 public:  // for gtest
    static Section *test_get(const char *name) { return get(name); }
};

#endif /* BF_ASM_SECTIONS_H_ */
