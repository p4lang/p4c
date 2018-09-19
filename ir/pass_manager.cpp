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

#include "ir.h"
#include "lib/gc.h"
#include "lib/n4.h"

const IR::Node *PassManager::apply_visitor(const IR::Node *program, const char *) {
    safe_vector<std::pair<safe_vector<Visitor *>::iterator, const IR::Node *>> backup;
    static indent_t log_indent(-1);
    struct indent_nesting {
        indent_t &indent;
        explicit indent_nesting(indent_t &i) : indent(i) { ++indent; }
        ~indent_nesting() { --indent; }
    } nest_log_indent(log_indent);

    early_exit_flag = false;
    unsigned initial_error_count = ::errorCount();
    BUG_CHECK(running, "not calling apply properly");
    for (auto it = passes.begin(); it != passes.end();) {
        Visitor* v = *it;
        if (auto b = dynamic_cast<Backtrack *>(v)) {
            if (!b->never_backtracks()) {
                backup.emplace_back(it, program); } }
        try {
            try {
                size_t maxmem;
                LOG1(log_indent << name() << " invoking " << v->name());
                auto after = program->apply(**it);
                LOG3(log_indent << "heap after " << v->name() << ": in use " <<
                     n4(gc_mem_inuse(&maxmem)) << "B, max " << n4(maxmem) << "B");
                if (stop_on_error && ::errorCount() > initial_error_count)
                    break;
                if ((program = after) == nullptr) break;
            } catch (Backtrack::trigger::type_t &trig_type) {
                throw Backtrack::trigger(trig_type);
            }
        } catch (Backtrack::trigger &trig) {
            LOG1(log_indent << "caught backtrack trigger " << trig);
            while (!backup.empty()) {
                if (backup.back().first == it) {
                    backup.pop_back();
                    continue; }
                it = backup.back().first;
                auto b = dynamic_cast<Backtrack *>(*it);
                program = backup.back().second;
                if (b->backtrack(trig))
                    break;
                LOG1(log_indent << "pass " << b->name() << " can't handle it"); }
            if (backup.empty()) {
                LOG1(log_indent << "rethrow trigger");
                throw; }
            continue; }
        runDebugHooks(v->name(), program);
        if (early_exit_flag)
            break;
        seqNo++;
        it++; }
    running = false;
    return program;
}

bool PassManager::backtrack(trigger &trig) {
    for (Visitor *v : passes)
        if (auto *bt = dynamic_cast<Backtrack *>(v))
            if (bt->backtrack(trig)) return true;
    return false;
}

bool PassManager::never_backtracks() {
    if (never_backtracks_cache >= 0) return never_backtracks_cache;
    for (auto v : passes) {
        if (auto b = dynamic_cast<Backtrack *>(v)) {
            if (!b->never_backtracks()) {
                never_backtracks_cache = 0;
                return false; } } }
    never_backtracks_cache = 1;
    return true;
}

void PassManager::runDebugHooks(const char* visitorName, const IR::Node* program) {
    for (auto h : debugHooks)
        h(name(), seqNo, visitorName, program);
}

const IR::Node *PassRepeated::apply_visitor(const IR::Node *program, const char *name) {
    bool done = false;
    unsigned iterations = 0;
    unsigned initial_error_count = ::errorCount();
    while (!done) {
        LOG5("PassRepeated state is:\n" << dumpToString(program));
        running = true;
        auto newprogram = PassManager::apply_visitor(program, name);
        if (program == newprogram || newprogram == nullptr)
            done = true;
        if (stop_on_error && ::errorCount() > initial_error_count)
            return program;
        iterations++;
        if (repeats != 0 && iterations > repeats)
            done = true;
        program = newprogram;
    }
    return program;
}

const IR::Node *PassRepeatUntil::apply_visitor(const IR::Node *program, const char *name) {
    do {
        running = true;
        program = PassManager::apply_visitor(program, name);
    } while (!done());
    return program;
}
