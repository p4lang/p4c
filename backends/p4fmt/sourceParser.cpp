/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 * SPDX-License-Identifier: Apache-2.0
 */
#include "backends/p4fmt/sourceParser.h"

#include <algorithm>
#include <string>

#include "backends/p4fmt/preprocessor.h"

namespace P4::P4Fmt {
namespace {

using Break = SourceFragment::Break;
struct Atom {
    size_t begin;
    size_t end;
    std::string_view first;
    std::string_view last;
    bool comment = false;
    std::optional<Directive> directive;
};

bool flow(std::string_view text) {
    return text == "if" || text == "switch" || text == "select" || text == "for" || text == "while";
}

bool needsSpace(std::string_view left, std::string_view right) {
    const auto spelling = std::string(left) + std::string(right);
    const FormatTokens joined(spelling);
    return joined.tokens.size() != 2 || joined.logicalSpelling(joined.tokens[0]) != left ||
           joined.logicalSpelling(joined.tokens[1]) != right;
}

// Track alternative branch contexts without expanding their combinations. Only
// delimiter frames agreed on by every branch survive an #endif. A missing #else
// contributes the entry context as an implicit empty branch.
struct Conditional {
    std::vector<char> entry;
    std::optional<std::vector<char>> merged;
    bool hasElse = false;
    void merge(const std::vector<char> &context) {
        if (!merged) {
            merged = context;
            return;
        }
        size_t common = 0;
        while (common < merged->size() && common < context.size() &&
               (*merged)[common] == context[common])
            ++common;
        merged->resize(common);
    }
};

}  // namespace

std::optional<SourceDocument> parseSource(const FormatTokens &source) {
    if (!source.complete) return std::nullopt;
    const auto directives = scanDirectives(source);
    std::vector<Atom> atoms;
    size_t directive = 0;
    for (size_t i = 0; i < source.tokens.size(); ++i) {
        const auto &token = source.tokens[i];
        if (token.kind == FormatTokens::Kind::Whitespace) continue;
        if (!atoms.empty() && token.begin < atoms.back().end) continue;
        const auto text = source.logicalSpelling(token);
        if (directive < directives.size() && directives[directive].begin == token.begin) {
            const auto &d = directives[directive++];
            atoms.push_back({d.begin, d.end, text, text, false, d});
            continue;
        }
        Atom atom{token.begin, token.end, text, text, token.kind == FormatTokens::Kind::Comment,
                  std::nullopt};
        // Without expansion/type resolution any identifier may name a macro.
        // Keep argument trivia: stringification makes even whitespace semantic.
        // Matching is lexical, so strings/comments cannot close the group.
        if (auto end = macroArgumentEnd(source, i)) {
            atom.end = *end;
            atom.last = ")";
        }
        atoms.push_back(atom);
    }

    SourceDocument document;
    std::vector<char> context;
    std::vector<Conditional> conditions;
    size_t end = 0;
    bool previousListClose = false;
    for (size_t i = 0; i < atoms.size(); ++i) {
        const auto &atom = atoms[i];
        const auto gap = source.source.substr(end, atom.begin - end);
        const auto current = atom.first;
        const auto previous = i == 0 ? std::string_view{} : atoms[i - 1].last;
        const bool listClose = current == "}" && !context.empty() && context.back() == 'L';
        bool closes = current == "}" || current == ")" || current == "]" ||
                      (current == ">" && !context.empty() && context.back() == '<');
        if (closes && !context.empty()) {
            const char opener = listClose        ? 'L'
                                : current == "}" ? '{'
                                : current == ")" ? '('
                                : current == "]" ? '['
                                                 : '<';
            // Error fixtures can contain mismatched delimiters. Recover to the
            // nearest compatible frame; never invent, delete, or move a token.
            auto frame = std::find(context.rbegin(), context.rend(), opener);
            if (frame != context.rend())
                context.resize(context.size() - (frame - context.rbegin()) - 1);
        }
        unsigned indent = std::count(context.begin(), context.end(), '{');
        Break before = Break::Space;
        const bool newline = gap.find('\n') != std::string_view::npos;
        const bool protectedGap = (current == "#" && !atom.directive) ||
                                  gap.find('\\') != std::string_view::npos || atom.comment ||
                                  (i && atoms[i - 1].comment) || atom.directive.has_value();
        if (i == 0 || protectedGap) {
            before = Break::Original;
        } else if (atoms[i - 1].directive || newline ||
                   (previous == "{" && !listClose && !context.empty() && context.back() == '{') ||
                   (current == "}" && !listClose) ||
                   (previous == ";" && current != ")" &&
                    (context.empty() || context.back() == '{')) ||
                   (previous == "}" && !previousListClose && current != ";" && current != "," &&
                    current != ")" && current != "else")) {
            before = std::count(gap.begin(), gap.end(), '\n') > 1 ? Break::BlankLine : Break::Line;
        } else if (current == ";" || current == "," || current == ")" || current == "]" ||
                   current == "." || previous == "." || previous == "@" || previous == "(" ||
                   previous == "[" || current == "[" || previous == "!" || previous == "~" ||
                   (current == "(" && !flow(previous)) ||
                   (current == "<" &&
                    (previous == "bit" || previous == "int" || previous == "varbit")) ||
                   (!context.empty() && context.back() == '<' && previous == "<") ||
                   (current == ">" && closes)) {
            before = Break::None;
        }
        if (before == Break::None && needsSpace(previous, current)) before = Break::Space;
        document.push_back(
            {source.source.substr(atom.begin, atom.end - atom.begin), gap, before, indent});
        end = atom.end;
        previousListClose = listClose;

        if (atom.directive) {
            const auto &d = *atom.directive;
            if (d.kind == Directive::Kind::If) conditions.push_back({context, std::nullopt, false});
            if (!conditions.empty()) {
                auto &condition = conditions.back();
                if (d.kind == Directive::Kind::Else) {
                    condition.merge(context);
                    condition.hasElse |= d.hasElse;
                    context = condition.entry;
                } else if (d.kind == Directive::Kind::Endif) {
                    condition.merge(context);
                    if (!condition.hasElse) condition.merge(condition.entry);
                    context = *condition.merged;
                    conditions.pop_back();
                }
            }
        } else if (atom.first == atom.last && !atom.comment) {
            if (current == "{") {
                const bool tableProperty =
                    i >= 2 && (atoms[i - 2].last == "key" || atoms[i - 2].last == "actions" ||
                               atoms[i - 2].last == "entries");
                bool expression = false;
                for (size_t previousAtom = i; previousAtom > 0;) {
                    const auto part = atoms[--previousAtom].last;
                    if (part == ";" || part == "{" || part == "}") break;
                    if (part == "=" || part == "return") {
                        expression = true;
                        break;
                    }
                }
                // A braced keyset in a select case is an expression, even
                // though it follows the select block's opening brace.
                bool keyset = false;
                unsigned braces = 1;
                for (size_t next = i + 1; next < atoms.size(); ++next) {
                    if (atoms[next].directive) break;
                    if (atoms[next].first == "{") ++braces;
                    if (atoms[next].first == "}" && --braces == 0) {
                        keyset = next + 1 < atoms.size() && atoms[next + 1].first == ":";
                        break;
                    }
                }
                const bool list =
                    !tableProperty &&
                    (keyset || expression || previous == "(" || previous == "," ||
                     previous == ":" ||
                     (!context.empty() &&
                      (context.back() == 'L' || context.back() == '(' || context.back() == '[')));
                context.push_back(list ? 'L' : '{');
            }
            if (current == "(" || current == "[") context.push_back(current[0]);
            if (current == "<" && (previous == "bit" || previous == "int" || previous == "varbit"))
                context.push_back('<');
        }
    }
    auto tail = source.source.substr(end);
    const bool keepTail = atoms.empty() || atoms.back().comment || atoms.back().directive ||
                          tail.find('\\') != std::string_view::npos ||
                          (!atoms.empty() && atoms.back().last == "\\");
    document.push_back({{}, tail, keepTail ? Break::Original : Break::Line, 0});
    return document;
}

bool sameSourceTokens(const FormatTokens &source, const FormatTokens &proposal) {
    if (!source.complete || !proposal.complete) return false;
    const auto originalDirectives = scanDirectives(source);
    const auto proposedDirectives = scanDirectives(proposal);
    if (originalDirectives.size() != proposedDirectives.size()) return false;
    for (size_t i = 0; i < originalDirectives.size(); ++i) {
        const auto &left = originalDirectives[i];
        const auto &right = proposedDirectives[i];
        if (source.source.substr(left.begin, left.end - left.begin) !=
            proposal.source.substr(right.begin, right.end - right.begin))
            return false;
    }
    size_t next = 0;
    auto significant = [](const FormatTokens &tokens, const FormatTokens::Token &token) {
        return token.kind != FormatTokens::Kind::Whitespace ||
               tokens.spelling(token).find('\\') != std::string_view::npos;
    };
    for (const auto &token : source.tokens) {
        if (!significant(source, token)) continue;
        while (next < proposal.tokens.size() && !significant(proposal, proposal.tokens[next]))
            ++next;
        if (next == proposal.tokens.size()) return false;
        const auto &other = proposal.tokens[next++];
        if (token.kind != other.kind || source.spelling(token) != proposal.spelling(other))
            return false;
    }
    while (next < proposal.tokens.size()) {
        if (significant(proposal, proposal.tokens[next++])) return false;
    }
    return true;
}

}  // namespace P4::P4Fmt
