#include "ir.h"

const IR::Node *PassManager::apply_visitor(const IR::Node *program, const char *) {
    vector<std::pair<vector<Visitor *>::iterator, const IR::Node *>> backup;

    for (auto it = passes.begin(); it != passes.end();) {
        Visitor* v = *it;
        if (dynamic_cast<Backtrack *>(v))
            backup.emplace_back(it, program);
        try {
            try {
                LOG1("Invoking " << v->name());
                program = program->apply(**it);
                int errors = ErrorReporter::instance.getErrorCount();
                if (stop_on_error && errors > 0) {
                    ::error("%1% errors encountered, aborting compilation", errors);
                    program = nullptr;
                }
                if (program == nullptr) break;
            } catch (Backtrack::trigger::type_t &trig_type) {
                throw Backtrack::trigger(trig_type);
            }
        } catch (Backtrack::trigger &trig) {
            while (!backup.empty()) {
                if (backup.back().first == it) {
                    backup.pop_back();
                    continue; }
                it = backup.back().first;
                auto b = dynamic_cast<Backtrack *>(*it);
                program = backup.back().second;
                if (b->backtrack(trig))
                    break; }
            if (backup.empty())
                throw trig;
            continue; }
        runDebugHooks(v->name(), program);
        seqNo++;
        it++; }
    return program;
}

void PassManager::runDebugHooks(const char* visitorName, const IR::Node* program) {
    for (auto h : debugHooks)
        h(managerName, seqNo, visitorName, program);
}

const IR::Node *PassRepeated::apply_visitor(const IR::Node *program, const char *name) {
    bool done = false;
    unsigned iterations = 0;
    while (!done) {
        LOG5("PassRepeated state is:\n" << dumpToString(program));
        auto newprogram = PassManager::apply_visitor(program, name);
        if (program == newprogram || newprogram == nullptr)
            done = true;
        int errors = ErrorReporter::instance.getErrorCount();
        if (stop_on_error && errors > 0)
            return nullptr;
        iterations++;
        if (repeats != 0 && iterations > repeats)
            done = true;
        program = newprogram;
    }
    return program;
}
