// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
//
// SPDX-License-Identifier: Apache-2.0

#include "ir/shared_ptr.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <utility>

#include "config.h"
#include "lib/gc.h"

namespace P4::Test {
namespace {

struct OwnedNode : virtual IR::shared_ptr_base {
    int &destroyed;
    IR::shared_ptr<OwnedNode> child;

    explicit OwnedNode(int &destroyed, IR::shared_ptr<OwnedNode> child = nullptr)
        : destroyed(destroyed), child(std::move(child)) {}
    virtual ~OwnedNode() { ++destroyed; }
};

TEST(IRSharedPtr, DeletesLastOwner) {
    int destroyed = 0;
    IR::shared_ptr<OwnedNode> owner = new OwnedNode(destroyed);
    {
        IR::shared_ptr<const OwnedNode> copy = owner;
        owner = nullptr;
        EXPECT_EQ(0, destroyed);
    }
    EXPECT_EQ(1, destroyed);
}

TEST(IRSharedPtr, AssignmentFromOwnedChild) {
    int destroyed = 0;
    IR::shared_ptr<OwnedNode> owner = new OwnedNode(destroyed);
    owner->child = new OwnedNode(destroyed);
    owner = owner->child;
    EXPECT_EQ(1, destroyed);
    EXPECT_TRUE(owner->check_referenced());
    owner = nullptr;
    EXPECT_EQ(2, destroyed);
}

TEST(IRSharedPtr, AssignmentFromRawChild) {
    int destroyed = 0;
    IR::shared_ptr<OwnedNode> owner = new OwnedNode(destroyed);
    owner->child = new OwnedNode(destroyed);
    owner = owner->child.get();
    EXPECT_EQ(1, destroyed);
    owner = nullptr;
    EXPECT_EQ(2, destroyed);
}

TEST(IRSharedPtr, MoveReleasesPreviousOwner) {
    int destroyed = 0;
    IR::shared_ptr<OwnedNode> first = new OwnedNode(destroyed);
    IR::shared_ptr<OwnedNode> second = new OwnedNode(destroyed);
    first = std::move(second);
    EXPECT_EQ(nullptr, second.get());
    EXPECT_EQ(1, destroyed);
    first = nullptr;
    EXPECT_EQ(2, destroyed);
}

TEST(IRSharedPtr, NestedAllocationInConstructorArgument) {
    int destroyed = 0;
    IR::shared_ptr<OwnedNode> owner = new OwnedNode(destroyed, new OwnedNode(destroyed));
    owner = nullptr;
    EXPECT_EQ(2, destroyed);
}

TEST(IRSharedPtr, ConstructorArgumentException) {
    int destroyed = 0;
    auto fail = []() -> IR::shared_ptr<OwnedNode> { throw std::runtime_error("argument"); };
    EXPECT_THROW(new OwnedNode(destroyed, fail()), std::runtime_error);
    // The failed construction must not make this stack object look heap allocated.
    OwnedNode stack(destroyed);
    { IR::shared_ptr<OwnedNode> borrowed = &stack; }
    EXPECT_EQ(0, destroyed);
    EXPECT_FALSE(stack.check_referenced());
    IR::shared_ptr<OwnedNode> owner = new OwnedNode(destroyed);
    owner = nullptr;
    EXPECT_EQ(1, destroyed);
}

#if HAVE_LIBGC
TEST(IRSharedPtr, CollectionDuringConstructorArgument) {
    int destroyed = 0;
    auto collect = []() -> IR::shared_ptr<OwnedNode> {
        gc_mem_inuse();
        return nullptr;
    };
    IR::shared_ptr<OwnedNode> owner = new OwnedNode(destroyed, collect());
    owner = nullptr;
    EXPECT_EQ(1, destroyed);
}
#endif

TEST(IRSharedPtr, StackAndInlineNodesAreNotDeleted) {
    int destroyed = 0;
    {
        OwnedNode stack(destroyed);
        { IR::shared_ptr<OwnedNode> borrowed = &stack; }
        EXPECT_FALSE(stack.check_referenced());
        struct Container {
            OwnedNode node;
            explicit Container(int &destroyed) : node(destroyed) {}
        };
        auto container = std::make_unique<Container>(destroyed);
        { IR::shared_ptr<OwnedNode> borrowed = &container->node; }
        EXPECT_FALSE(container->node.check_referenced());
        EXPECT_EQ(0, destroyed);
    }
    EXPECT_EQ(2, destroyed);
}

TEST(IRSharedPtr, PlacementConstructionIsNotDeleted) {
    int destroyed = 0;
    alignas(OwnedNode) unsigned char storage[sizeof(OwnedNode)];
    auto *node = new (storage) OwnedNode(destroyed);
    { IR::shared_ptr<OwnedNode> borrowed = node; }
    EXPECT_EQ(0, destroyed);
    EXPECT_FALSE(node->check_referenced());
    node->~OwnedNode();
    EXPECT_EQ(1, destroyed);
}

TEST(IRSharedPtr, OverAlignedAllocation) {
    struct alignas(128) AlignedNode : OwnedNode {
        using OwnedNode::OwnedNode;
    };
    int destroyed = 0;
    IR::shared_ptr<OwnedNode> owner = new AlignedNode(destroyed);
    EXPECT_EQ(0U, reinterpret_cast<uintptr_t>(owner.get()) % alignof(AlignedNode));
    owner = nullptr;
    EXPECT_EQ(1, destroyed);
}

}  // namespace
}  // namespace P4::Test
