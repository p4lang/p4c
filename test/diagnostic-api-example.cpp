// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "frontends/common/parseInput.h"
#include "lib/diagnostic.h"

std::vector<P4::Diagnostic> compile(const std::string &source) {
    using Context = P4::P4CContextWithOptions<P4::CompilerOptions>;
    P4::AutoCompileContext context(std::make_unique<Context>());
    auto &reporter = Context::get().errorReporter();
    reporter.enableDiagnosticCollection();
    // A caller can also install setDiagnosticConsumer() for incremental access.
    P4::parseP4String("editor-buffer.p4", 1, source);
    return reporter.getDiagnostics();
}

int main() {
    // No SARIF file or terminal parsing. The context above has already died.
    auto diagnostics = compile("/*é😀*/ bad;");
    for (const auto &diagnostic : diagnostics) {
        unsigned lspSeverity = diagnostic.severity == P4::DiagnosticSeverity::Error     ? 1
                               : diagnostic.severity == P4::DiagnosticSeverity::Warning ? 2
                                                                                        : 3;
        std::cout << "severity=" << lspSeverity << ' ' << diagnostic.code << ": "
                  << diagnostic.message << '\n';
        if (!diagnostic.location || !diagnostic.location->range) continue;
        const auto &range = *diagnostic.location->range;
        if (!range.start.column) continue;
        // An LSP client negotiating UTF-16 can map these fields directly.
        // UTF-8/UTF-32 clients require conversion using the document text.
        std::cout << diagnostic.location->uri << " line=" << range.start.line
                  << " character=" << *range.start.column << '\n';
    }
    // A fresh compilation has no stale errors. LSP publishers must also publish
    // empty arrays for documents whose previous diagnostics have disappeared.
    auto next = compile("struct S { bit<8> x; }");
    return diagnostics.empty() || !next.empty();
}
