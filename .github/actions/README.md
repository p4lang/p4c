<!--
SPDX-FileCopyrightText: 2026 The P4 Language Consortium

SPDX-License-Identifier: Apache-2.0
-->

# CI caches

Keep the main caches useful while leaving some space for PR caches.

- Restore the main cache on every build so GitHub sees that it is still in use.
- Add the PR's saved files after restoring the main cache.
- Let successful main builds add their results to the shared cache.
- Save only the files a PR added or changed, up to 200 MB per job.
- Skip uploads when the cache has not changed.
- Let merge-queue builds use caches without saving new ones.
- Skip compiler caching in formatting jobs.

The main shared cache has 2 GB, each static-build cache 1 GB, and other compiler caches 750 MB. These are size limits. Keep different compilers and build settings under separate keys.

`setup-p4c-build` calls `restore-ccache` before the build and `save-ccache` after
it. The Docker workflow calls these actions around its build and cache export.
The Python helper compares file contents, since ccache changes timestamps on
cache hits. It copies result and manifest files without changing their contents.
If a file is missing after cleanup, ccache can compile it again.

# Daily cleanup

- Keep the latest main cache for each build configuration and archive version.
- Keep the latest cache for each open PR and job; remove older copies and closed-PR caches.
- Remove PR caches that have gone unused longest to target 2 GB of PR caches and 9 GB total.
- Leave 1 GB below GitHub's default limit for new uploads between cleanups.
- Only delete caches whose keys start with `ccache-v2-` or `bazel-v2-`.

# Checks

Run `python3 .github/scripts/cache_retention.py` with `GH_TOKEN` and
`GITHUB_REPOSITORY` set to preview cleanup; add `--apply` to delete the listed caches.

Check compiler cache hits, build times, and saved cache sizes before changing the limits.
