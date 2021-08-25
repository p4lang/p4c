#include "pna.h"
#include "ir/ir.h"

namespace P4 {

bool ParsePnaArchitecture::preorder(const IR::ToplevelBlock* block) {
    /// Blocks are not in IR tree, use a custom visitor to traverse
    for (auto it : block->constantValue) {
        if (it.second->is<IR::Block>())
            visit(it.second->getNode());
    }
    return false;
}

bool ParsePnaArchitecture::preorder(const IR::ExternBlock* block) {
    if (block->node->is<IR::Declaration>())
        structure->globals.push_back(block);
    return false;
}

const IR::ParserBlock* getParserBlock(const IR::PackageBlock* block, cstring param) {
    auto pkg = block->findParameterValue(param);
    if (pkg == nullptr)
        ::error("Package %1% has no parameter named %2%", block, param);
    auto parser = pkg->to<IR::ParserBlock>();
    if (parser == nullptr)
        ::error("%1%: %2% argument of main should be bound to a parser", block, param);
    return parser;
}

const IR::ControlBlock* getControlBlock(const IR::PackageBlock* block, cstring param) {
    auto pkg = block->findParameterValue(param);
    if (pkg == nullptr)
        ::error("Package %1% has no parameter named %2%", block, param);
    auto control = pkg->to<IR::ControlBlock>();
    if (control == nullptr)
        ::error("%1%: %2% argument of main should be bound to a control", block, param);
    return control;
}

bool ParsePnaArchitecture::preorder(const IR::PackageBlock* block) {
    auto main_parser = getParserBlock(block, "main_parser");
    auto pre_control = getControlBlock(block, "pre_control");
    auto main_control = getControlBlock(block, "main_control");
    auto main_deparser = getControlBlock(block, "main_deparser");

    return true;
}

}  // P4
