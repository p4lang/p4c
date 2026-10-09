#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

"""Offline tests for cache cleanup; no GitHub requests are made."""

import unittest

from cache_retention import cache_prefix, select_evictions


def cache(cache_id, key, ref="refs/heads/main", version="v1"):
    return {
        "id": cache_id,
        "key": key,
        "ref": ref,
        "version": version,
        "created_at": f"2026-01-{cache_id:02d}T00:00:00Z",
        "last_accessed_at": f"2026-01-{cache_id:02d}T00:00:00Z",
        "size_in_bytes": 100,
    }


class CacheRetentionTest(unittest.TestCase):
    def test_cpm_keeps_latest_per_job_and_archive_version(self):
        core = "cpm-v1-Linux-X64-core-main-"
        tofino = "cpm-v1-Linux-X64-tofino-main-"
        caches = [
            cache(1, core + "a" * 64),
            cache(2, core + "b" * 64),
            cache(3, tofino + "b" * 64),
            cache(4, core + "b" * 64, version="v2"),
        ]
        self.assertEqual(select_evictions(caches, set(), 1000, 1000), [1])

    def test_cpm_pr_and_merge_queue_entries_are_removed(self):
        key = "cpm-v1-Linux-X64-core-main-" + "a" * 64
        caches = [
            cache(1, key, ref="refs/pull/42/merge"),
            cache(2, key, ref="refs/heads/gh-readonly-queue/main/test"),
            cache(3, key),
        ]
        self.assertEqual(set(select_evictions(caches, {42}, 1000, 1000)), {1, 2})

    def test_foreign_and_malformed_keys_are_untouched(self):
        for key in ("other-cache", "cpm-v1-Linux-main-not-a-digest"):
            self.assertIsNone(cache_prefix(key))
            self.assertEqual(select_evictions([cache(1, key)], set(), 1000, 1000), [])

    def test_compiler_and_bazel_retention_still_works(self):
        caches = [
            cache(1, "ccache-v2-Linux-X64-core-main-123-1"),
            cache(2, "ccache-v2-Linux-X64-core-main-124-1"),
            cache(3, "bazel-v2-Linux-pr-42-123-1", ref="refs/pull/42/merge"),
            cache(4, "bazel-v2-Linux-pr-43-124-1", ref="refs/pull/43/merge"),
        ]
        self.assertEqual(set(select_evictions(caches, {42}, 1000, 1000)), {1, 4})


if __name__ == "__main__":
    unittest.main()
