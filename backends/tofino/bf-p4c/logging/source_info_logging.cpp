/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include "source_info_logging.h"

#include <unistd.h>

#include <stack>

#include "manifest.h"

CollectSourceInfoLogging::CollectSourceInfoLogging(const P4::ReferenceMap &refMap)
    : Inspector(), refMap(refMap) {
    // Set initial source root based on current working directory
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) sourceRoot = cwd;
}

void CollectSourceInfoLogging::addSymbol(const CollectSourceInfoLogging::Symbol &symbol) {
    std::string file = "";
    unsigned line = 0, endLine = 0, column = 0;
    if (symbol.inst.isValid()) {
        file = symbol.inst.toSourcePositionData(&line, &column);
        endLine =
            line + (symbol.inst.getEnd().getLineNumber() - symbol.inst.getStart().getLineNumber());
    }
    // TODO(1): else { return; }
    // Make file path relative to sourceRoot (ignore paths outside sourceRoot)
    if (file.find(sourceRoot) == 0) file.erase(0, sourceRoot.length() + 1);

#ifdef BF_P4C_LOGGING_SOURCE_INFO_LOGGING_H_DEBUG
    std::cout << symbol.name << "(" << symbol.type << ")" << std::endl;
    std::cout << "\t";
    if (file != "")
        std::cout << file << ":" << line << "." << column;
    else
        std::cout << "n/a";
    if (line != endLine) {
        std::cout << "->" << endLine;
    }
    std::cout << std::endl;
#endif

    auto *logSymbol = new Source_Info_Schema_Logger::SymbolDefinition(
        symbol.name.c_str(),
        new Source_Info_Schema_Logger::SourceLocation(
            file, line,
            new int(column + 1),  // SourceInfo column starts from 0
            (endLine != line) ? new int(endLine) : nullptr),
        symbol.type.c_str());

    for (const auto &ref : symbol.refs) {
        file = "";
        line = 0;
        endLine = 0;
        column = 0;
        if (ref.isValid()) {
            file = ref.toSourcePositionData(&line, &column);
            endLine = line + (ref.getEnd().getLineNumber() - ref.getStart().getLineNumber());
        }
        // TODO(2): else { continue; }
        // Make file path relative to sourceRoot (ignore paths outside sourceRoot)
        if (file.find(sourceRoot) == 0) file.erase(0, sourceRoot.length() + 1);

#ifdef BF_P4C_LOGGING_SOURCE_INFO_LOGGING_H_DEBUG
        std::cout << "\t\t";
        if (file != "")
            std::cout << file << ":" << line << "." << column;
        else
            std::cout << "n/a";
        if (line != endLine) std::cout << "->" << endLine;
        std::cout << std::endl;
#endif

        if (ref.isValid())  // TODO: Remove condition when TODO(2) is done
            logSymbol->append_references(new Source_Info_Schema_Logger::SourceLocation(
                file, line,
                new int(column + 1),  // SourceInfo column starts from 0
                (endLine != line) ? new int(endLine) : nullptr));
    }

    if (symbol.inst.isValid()) {  // TODO: Remove condition when TODO(1) is done
        symbols.push_back(logSymbol);
    }
}

Visitor::profile_t CollectSourceInfoLogging::init_apply(const IR::Node *root) {
    auto rv = Inspector::init_apply(root);

    // Set actual source root based on P4 program
    if (root->is<IR::P4Program>()) {
        auto mainDecls = root->to<IR::P4Program>()->getDeclsByName(IR::P4Program::main)->toVector();
        if (!mainDecls.empty()) {
            auto mainSrcInfo = mainDecls.front()->getSourceInfo();
            if (mainSrcInfo.isValid()) {
                sourceRoot = mainSrcInfo.getSourceFile();
                sourceRoot = sourceRoot.substr(0, sourceRoot.find_last_of("/"));
            }
        } else {
            ::warning(ErrorType::WARN_MISSING, "Program does not contain a '%s' module",
                      IR::P4Program::main);
        }
    } else {
        BUG("Invalid root %1% used for Source info collection", root->node_type_name());
    }

    return rv;
}

// Merge two strings by concatenating without possible overlaps (e.g. "a.b.c"+"b.c.d" -> "a.b.c.d")
std::string mergeStrings(const std::string &s1, const std::string &s2) {
    std::string out = s1 + s2;
    unsigned overlap = std::min(s1.length(), s2.length());
    while (overlap > 0) {
        if (s1.substr(s1.length() - overlap, s1.length()) == s2.substr(0, overlap)) {
            out = s1.substr(0, s1.length() - overlap) + s2;
            break;
        }
        overlap--;
    }
    return out;
}

// Remove possible leading dot from input string (e.g. ".NoAction" -> "NoAction")
void removeLeadingDot(std::string &s) {
    if (!s.empty() && s.front() == '.') s.erase(0, 1);
}

std::string findBestRefName(const Visitor_Context *ctxt, std::string initName = "") {
    while (ctxt) {
        // In the reference's context, look for the topmost Member or ArrayIndex
        // node to get the full struct field's name, for example (topmost node is in []):
        //     [Member(struct.a[1].field)] -> ArrayIndex(struct.a[1]) -> Member(struct) ->
        //     PathExpression -> Path
        if (ctxt->node->is<IR::Member>() || ctxt->node->is<IR::ArrayIndex>()) {
            // We don't need method names, so we ignore nodes with MethodCallExpression
            // parents, for example (the ignored node is in []):
            //     MethodCallExpression(pkt.extract) -> [Member(pkt.extract)]
            if (ctxt->parent->node->is<IR::MethodCallExpression>())
                break;
            else
                initName = ctxt->node->toString();
            // Simply skip Path and PathExpression on the way up to Member/ArrayIndex
            // Stop after finding anything else
        } else if (!ctxt->node->is<IR::Path>() && !ctxt->node->is<IR::PathExpression>()) {
            break;
        }
        ctxt = ctxt->parent;
    }
    return initName;
}

void CollectSourceInfoLogging::addSymbolReference(const IR::Node *ref,
                                                  const IR::IDeclaration *decl) {
    if (!ref || !decl) return;

    auto &symbolByNameMap = symbolByTypeMap[decl->node_type_name()];

    // Get symbol Fully Qualified Name
    std::string fqn = "";
    // Handle declarations that provide FQN
    if (decl->is<IR::P4Table>() || decl->is<IR::P4Action>() ||
        decl->is<IR::Declaration_Instance>() || decl->is<IR::Declaration_Variable>()) {
        fqn = decl->externalName();
        // In some cases (e.g. struct fields) the declaration name is not a complete FQN,
        // we need to add the reference name to it (i.e. particular struct field name)
        std::string refName = findBestRefName(getChildContext());
        // Merge the declaration and reference names (dealing with possible overlaps)
        fqn = mergeStrings(fqn, refName);
        // IR::Parameter declarations don't provide FQN, we need to create it
        // ! This is rather a hack, the right way is to modify IR::Parameter to provide FQN
    } else if (decl->is<IR::Parameter>()) {
        std::string refName = "";
        const IR::P4Action *action = nullptr;
        // P4 Action Parameters get FQN from their action and declaration names.
        // Other Parameters, that aren't easily recognizable (here as non-Members), must be ignored.
        if ((action = findContext<IR::P4Action>()) && !ref->is<IR::Member>() &&
            !findContext<IR::Member>()) {
            fqn = std::string(action->externalName()) + ".";
            refName = decl->externalName();
            // Non-P4 Action Parameters get name from their parser or control (incl. deparser) block
            // name and their declaration name, or, if available,
            // the topmost Member or ArrayIndex node's name instead of the declaration name.
        } else {
            // Set FQN to parser or control (incl. deparser) block name
            if (auto *parser = findContext<IR::P4Parser>())
                fqn = std::string(parser->externalName()) + ".";
            else if (auto *control = findContext<IR::P4Control>())
                fqn = std::string(control->externalName()) + ".";
            // Set reference name to declaration name first and then look for a better one
            refName = findBestRefName(getChildContext(), decl->externalName().c_str());
        }
        // Concatenate the declaration and reference names (dealing with possible overlaps)
        fqn += refName;
    }

    // Get symbol type
    // Start with declaration name
    std::string type = decl->node_type_name().c_str();
    // Append subtypes for specific types
    const IR::Type *declType = nullptr;
    if (decl->is<IR::Parameter>()) {
        declType = decl->to<IR::Parameter>()->type;
    } else if (decl->is<IR::Declaration_Instance>()) {
        declType = decl->to<IR::Declaration_Instance>()->type;
    } else if (decl->is<IR::Declaration_Variable>()) {
        declType = decl->to<IR::Declaration_Variable>()->type;
    }
    if (declType != nullptr) {
        // Either use the subtype itself or, if it's IR::Type_Name, use the type that it refers to
        if (declType->is<IR::Type_Name>())
            type +=
                "_" + refMap.getDeclaration(declType->to<IR::Type_Name>()->path)->node_type_name();
        else
            type += "_" + declType->node_type_name();
    }

    // Add reference and declaration source info to symbol FQN
    if (fqn != "") {
        removeLeadingDot(fqn);

        auto &symbol = symbolByNameMap[fqn];
        symbol.name = fqn;
        symbol.type = type;
        symbol.inst = decl->getSourceInfo();
        symbol.refs.insert(ref->getSourceInfo());
    }

#ifdef BF_P4C_LOGGING_SOURCE_INFO_LOGGING_H_DEBUG
    std::cout << "[" << ref->node_type_name() << "] ";
    std::stack<const IR::Node *> node_stack;
    const Context *cntxt = getContext();
    while (cntxt) {
        node_stack.push(cntxt->node);
        cntxt = cntxt->parent;
    }
    while (!node_stack.empty()) {
        const IR::Node *parent = node_stack.top();
        std::cout << "/" << parent->node_type_name() << "("
                  << (parent->is<IR::IDeclaration>()
                          ? parent->to<IR::IDeclaration>()->externalName()
                          : parent->toString())
                  << ")";
        node_stack.pop();
    }
    std::cout << std::endl << "\t";
    std::cout << "/" << ref->node_type_name() << "("
              << (ref->is<IR::IDeclaration>() ? ref->to<IR::IDeclaration>()->externalName()
                                              : ref->toString())
              << ")";
    std::cout << "("
              << (ref->getSourceInfo().isValid() ? ref->getSourceInfo().toPositionString() + ":" +
                                                       ref->getSourceInfo().toBriefSourceFragment()
                                                 : "SrcInfo n/a")
              << ")";
    if (decl) {
        std::cout << " -> ";
        std::cout << std::endl << "\t";
        std::cout << decl->node_type_name() << "(" << decl->externalName() << ")";
        std::cout << "("
                  << (decl->getSourceInfo().isValid()
                          ? decl->getSourceInfo().toPositionString() + ":" +
                                decl->getSourceInfo().toBriefSourceFragment()
                          : "DeclSrcInfo n/a")
                  << ")";
    }
    std::cout << std::endl;
    std::cout << std::endl;
#endif
}

bool CollectSourceInfoLogging::preorder(const IR::Path *path) {
    addSymbolReference(path, refMap.getDeclaration(path));

    return true;
}

bool CollectSourceInfoLogging::preorder(const IR::Argument *argument) {
    // Find the Path to be used as key to the reference map
    auto *expr = argument->expression;
    if (!expr || !expr->is<IR::PathExpression>()) return true;
    auto *exprPath = expr->to<IR::PathExpression>()->path;
    if (!exprPath) return true;

    addSymbolReference(argument, refMap.getDeclaration(exprPath));

    return true;
}

bool CollectSourceInfoLogging::preorder(const IR::Member *member) {
    // Find the Path to be used as key to the reference map
    auto *expr = member->expr;
    if (!expr || !expr->is<IR::PathExpression>()) return true;
    auto *exprPath = expr->to<IR::PathExpression>()->path;
    if (!exprPath) return true;

    // Find the topmost Member or ArrayIndex that should have right source info
    const IR::Node *node = member;
    const Context *ctxt = getContext();
    while (ctxt) {
        if (ctxt->node->is<IR::Member>() || ctxt->node->is<IR::ArrayIndex>())
            node = ctxt->node;
        else
            break;
        ctxt = ctxt->parent;
    }

    addSymbolReference(node, refMap.getDeclaration(exprPath));

    return true;
}

void CollectSourceInfoLogging::end_apply() {
    for (const auto &symbolByTypeMapItem : symbolByTypeMap) {
        auto &symbolByNameMap = symbolByTypeMapItem.second;

#ifdef BF_P4C_LOGGING_SOURCE_INFO_LOGGING_H_DEBUG
        auto &symbolType = symbolByTypeMapItem.first;
        std::cout << std::endl
                  << "################### " << symbolType << " ###################" << std::endl
                  << std::endl;
#endif

        for (const auto &symbolByNameMapItem : symbolByNameMap) {
            addSymbol(symbolByNameMapItem.second);
        }
    }
}

void SourceInfoLogging::end_apply() {
    logger.log();
    Logging::Manifest::getManifest().setSourceInfo(manifestPath);
}
