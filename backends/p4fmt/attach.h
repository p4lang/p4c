#ifndef BACKENDS_P4FMT_ATTACH_H_
#define BACKENDS_P4FMT_ATTACH_H_

#include "ir/visitor.h"
#include "lib/source_file.h"

namespace P4::P4Fmt {

class Attach : public Inspector {
 public:
    using NodeId = int;
    struct Comments {
        std::vector<const Util::Comment *> prefix;
        std::vector<const Util::Comment *> suffix;
    };
    using CommentsMap = std::unordered_map<NodeId, Comments>;
    enum class TraversalType { Preorder, Postorder };

    explicit Attach(const std::unordered_map<const Util::Comment *, bool> &processedComments)
        : processedComments(processedComments){};

    void attachCommentsToNode(const IR::Node *, TraversalType);

    bool preorder(const IR::Node *node) override;
    void postorder(const IR::Node *node) override;

    void addPrefixComments(NodeId, const Util::Comment *);
    void addSuffixComments(NodeId, const Util::Comment *);
    const CommentsMap &getCommentsMap() const;

 private:
    /// This Hashmap tracks each comment’s attachment status to IR nodes. Initially, all comments
    /// are set to 'false'.
    std::unordered_map<const Util::Comment *, bool> processedComments;

    CommentsMap commentsMap;
};

}  // namespace P4::P4Fmt

#endif /* BACKENDS_P4FMT_ATTACH_H_ */
