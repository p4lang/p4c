// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
// SPDX-License-Identifier: Apache-2.0

#include "frontends/common/options.h"

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>
#include <utility>

#include "test/gtest/helpers.h"

namespace P4::Test {
namespace {

class SnapshotOptions : public CompilerOptions {
 public:
    using CompilerOptions::copyConfigurationFrom;
};

static_assert(!std::is_copy_constructible_v<CompilerOptions>);
static_assert(!std::is_copy_assignable_v<CompilerOptions>);
static_assert(!std::is_move_constructible_v<CompilerOptions>);
static_assert(!std::is_move_assignable_v<CompilerOptions>);

class LifetimeOptions : public CompilerOptions {
 public:
    void retain(std::shared_ptr<int> token) {
        registerOption(
            "--retained", nullptr, [token](const char *) { return *token == 42; },
            "Keep a resource alive until its registration is destroyed.");
    }
};

using OptionsOwnership = P4CTest;

TEST_F(OptionsOwnership, BorrowedContextUsesOriginalCallbacks) {
    P4CContextWithOptions<SnapshotOptions> owner;
    auto &options = owner.options();
    {
        P4CContextWithOptions<CompilerOptions> borrowed(options);
        AutoCompileContext scope(&borrowed);
        EXPECT_EQ(&borrowed.options(), &options);
        char executable[] = "test";
        char flag[] = "--ndebug";
        char *argv[] = {executable, flag};
        ASSERT_NE(borrowed.options().process_options(2, argv), nullptr);
        EXPECT_TRUE(options.ndebug);
    }
    // Destroying the borrowing context must not release its owner's options.
    EXPECT_TRUE(options.ndebug);
}

TEST_F(OptionsOwnership, MovingOwnerPreservesCallbackTargets) {
    auto original = std::make_unique<CompilerOptions>();
    auto *address = original.get();
    auto owner = std::move(original);
    EXPECT_EQ(owner.get(), address);
    char executable[] = "test";
    char flag[] = "--ndebug";
    char *argv[] = {executable, flag};
    ASSERT_NE(owner->process_options(2, argv), nullptr);
    EXPECT_TRUE(owner->ndebug);
}

TEST_F(OptionsOwnership, SnapshotRetainsSettingsWithIndependentCallbacks) {
    SnapshotOptions snapshot;
    {
        SnapshotOptions source;
        std::string executableName = "snapshot-source-with-owned-argument-storage";
        char *sourceArgv[] = {executableName.data()};
        ASSERT_NE(source.process(1, sourceArgv), nullptr);
        source.langVersion = ParserOptions::FrontendVersion::P4_14;
        source.preprocessor_options = "-DSNAPSHOT=1"_cs;
        source.dumpJsonFile = "source.json";
        source.passesToExcludeFrontend.push_back("OriginalPass"_cs);
        snapshot.copyConfigurationFrom(source);
        EXPECT_EQ(snapshot.langVersion, source.langVersion);
        EXPECT_EQ(snapshot.preprocessor_options, source.preprocessor_options);
        EXPECT_EQ(snapshot.dumpJsonFile, source.dumpJsonFile);
        EXPECT_EQ(snapshot.passesToExcludeFrontend, source.passesToExcludeFrontend);

        char executable[] = "test";
        char flag[] = "--toJSON=snapshot.json";
        char *argv[] = {executable, flag};
        ASSERT_NE(snapshot.process_options(2, argv), nullptr);
        EXPECT_EQ(source.dumpJsonFile, "source.json");
        EXPECT_EQ(snapshot.dumpJsonFile, "snapshot.json");
    }
    EXPECT_STREQ(snapshot.getBinaryName().c_str(), "snapshot-source-with-owned-argument-storage");
    // The source has been destroyed; the snapshot's callbacks must still work.
    char executable[] = "test";
    char flag[] = "--ndebug";
    char *argv[] = {executable, flag};
    ASSERT_NE(snapshot.process_options(2, argv), nullptr);
    EXPECT_TRUE(snapshot.ndebug);
}

TEST_F(OptionsOwnership, DestroyingOptionsReleasesCallbacks) {
    std::weak_ptr<int> retained;
    {
        LifetimeOptions options;
        auto token = std::make_shared<int>(42);
        retained = token;
        options.retain(std::move(token));
        EXPECT_FALSE(retained.expired());
    }
    EXPECT_TRUE(retained.expired());
}

}  // namespace
}  // namespace P4::Test
