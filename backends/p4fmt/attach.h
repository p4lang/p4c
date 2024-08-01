#ifndef BACKENDS_P4FMT_ATTACH_H_
#define BACKENDS_P4FMT_ATTACH_H_

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "backends/p4fmt/p4fmt.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/source_file.h"

namespace P4::P4Fmt {

class Attach : public Transform {
 public:
    using NodeId = int;
    struct Comments {
        std::vector<const Util::Comment *> prefix;
        std::vector<const Util::Comment *> suffix;
    };
    using CommentsMap = std::unordered_map<NodeId, Comments>;
    enum class TraversalType { Preorder, Postorder };

    explicit Attach(const std::unordered_set<const Util::Comment *> &clist) : clist(clist){};
    ~Attach() override;

    const IR::Node *attachCommentsToNode(IR::Node *, TraversalType);

    using Transform::postorder;
    using Transform::preorder;

    const IR::Node *preorder(IR::Node *node) override;
    const IR::Node *postorder(IR::Node *node) override;

    static bool isSystemFile(const std::filesystem::path &file);

    void addPrefixComments(NodeId, const Util::Comment *);
    void addSuffixComments(NodeId, const Util::Comment *);
    const CommentsMap &getCommentsMap() const;

 private:
    std::unordered_set<const Util::Comment *> clist;
    CommentsMap commentsMap;
};

}  // namespace P4::P4Fmt

#endif /* BACKENDS_P4FMT_ATTACH_H_ */
