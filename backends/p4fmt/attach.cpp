#include "backends/p4fmt/attach.h"

#include "frontends/common/parser_options.h"
#include "lib/source_file.h"

namespace P4::P4Fmt {

void Attach::addPrefixComments(NodeId node, const Util::Comment *prefix) {
    commentsMap[node].prefix.push_back(prefix);
}

void Attach::addSuffixComments(NodeId node, const Util::Comment *suffix) {
    commentsMap[node].suffix.push_back(suffix);
}

const Attach::CommentsMap &Attach::getCommentsMap() const { return commentsMap; }

void Attach::attachCommentsToNode(const IR::Node *node, TraversalType ttype) {
    if (node == nullptr || !node->srcInfo.isValid() || processedComments.empty()) {
        return;
    }

    if (isSystemFile(node->srcInfo.getSourceFile())) {
        return;
    }

    const auto nodeStart = node->srcInfo.getStart();

    for (auto &[comment, isAttached] : processedComments) {
        // Skip if already attached
        if (isAttached) {
            continue;
        }

        const auto commentEnd = comment->getSourceInfo().getEnd();

        switch (ttype) {
            case TraversalType::Preorder:
                if (commentEnd.getLineNumber() == nodeStart.getLineNumber() - 1) {
                    addPrefixComments(node->clone_id, comment);
                    isAttached = true;  // Mark the comment as attached
                }
                break;

            case TraversalType::Postorder:
                if (commentEnd.getLineNumber() == nodeStart.getLineNumber()) {
                    addSuffixComments(node->clone_id, comment);
                    isAttached = true;
                }
                break;

            default:
                P4::error(ErrorType::ERR_INVALID, "traversal type unknown/unsupported.");
                return;
        }
    }
}

bool Attach::preorder(const IR::Node *node) {
    attachCommentsToNode(node, TraversalType::Preorder);
    return true;
}

void Attach::postorder(const IR::Node *node) {
    attachCommentsToNode(node, TraversalType::Postorder);
}

}  // namespace P4::P4Fmt
