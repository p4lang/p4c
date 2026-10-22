#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

"""Keep the latest main caches and remove old or excess PR caches.

Only manage keys starting with ccache-v2-, bazel-v2-, or cpm-v1-.
GitHub can still remove caches itself when the repository runs out of space.
"""

import argparse
import json
import os
import re
import urllib.error
import urllib.request

PR_REF = re.compile(r"refs/pull/(\d+)/merge$")


def cache_prefix(key: str) -> str | None:
    """Remove the changing suffix from keys written by our cache actions."""
    if key.startswith("cpm-v1-"):
        prefix, _, digest = key.rpartition("-")
        if prefix.endswith("-main") and re.fullmatch(r"[0-9a-f]{64}", digest):
            return prefix
        return None
    if not key.startswith(("ccache-v2-", "bazel-v2-")):
        return None
    prefix, run_id, attempt = key.rsplit("-", 2)
    if not run_id.isdigit() or not attempt.isdigit():
        return None
    return prefix


def belongs_to_open_pr(ref: str, prefix: str, open_prs: set[int]) -> bool:
    match = PR_REF.fullmatch(ref)
    if not match:
        return False
    pr_number = int(match.group(1))
    # Bazel keys end in -pr-N; ccache also appends the job name.
    pr_key = f"-pr-{pr_number}"
    matches_key = prefix.endswith(pr_key) or f"{pr_key}-" in prefix
    return pr_number in open_prs and matches_key


def select_evictions(caches, open_prs, pr_budget, total_budget):
    """Select caches to delete, always keeping the latest main cache per key."""
    seen = set()
    pr_caches = []
    delete_ids = []
    kept_bytes = 0
    newest_first = sorted(caches, key=lambda cache: cache["created_at"], reverse=True)

    # Keep one copy of each cache. Remove caches for closed PRs and merge queues.
    for cache in newest_first:
        prefix = cache_prefix(cache["key"])
        if prefix is None:
            kept_bytes += cache["size_in_bytes"]
            continue

        ref = cache["ref"]
        is_main = ref == "refs/heads/main" and prefix.endswith("-main")
        if not is_main and not belongs_to_open_pr(ref, prefix, open_prs):
            delete_ids.append(cache["id"])
            continue

        group = (prefix, ref, cache["version"])
        if group in seen:
            delete_ids.append(cache["id"])
            continue
        seen.add(group)
        kept_bytes += cache["size_in_bytes"]
        if not is_main:
            pr_caches.append(cache)

    # If space is still tight, remove the PR caches that have gone unused longest.
    pr_bytes = sum(cache["size_in_bytes"] for cache in pr_caches)
    oldest_used_first = sorted(pr_caches, key=lambda cache: cache["last_accessed_at"])
    for cache in oldest_used_first:
        if pr_bytes <= pr_budget and kept_bytes <= total_budget:
            break
        delete_ids.append(cache["id"])
        pr_bytes -= cache["size_in_bytes"]
        kept_bytes -= cache["size_in_bytes"]

    if kept_bytes > total_budget:
        print("Main caches and other caches exceed the storage target; check their sizes.")
    return delete_ids


def api(path, method="GET"):
    request = urllib.request.Request(
        f"https://api.github.com/repos/{os.environ['GITHUB_REPOSITORY']}/{path}",
        method=method,
        headers={
            "Authorization": f"Bearer {os.environ['GH_TOKEN']}",
            "Accept": "application/vnd.github+json",
            "X-GitHub-Api-Version": "2022-11-28",
        },
    )
    with urllib.request.urlopen(request, timeout=30) as response:
        data = response.read()
        return json.loads(data) if data else None


def pages(path, field=None):
    separator = "&" if "?" in path else "?"
    page = 1
    while True:
        result = api(f"{path}{separator}per_page=100&page={page}")
        items = result[field] if field else result
        yield from items
        if len(items) < 100:
            break
        page += 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--apply", action="store_true", help="Delete selected caches; default is dry run"
    )
    parser.add_argument("--pr-budget", type=int, default=2_000_000_000)
    parser.add_argument("--total-budget", type=int, default=9_000_000_000)
    args = parser.parse_args()
    caches = list(pages("actions/caches", "actions_caches"))
    open_prs = {pr["number"] for pr in pages("pulls?state=open")}
    selected = select_evictions(caches, open_prs, args.pr_budget, args.total_budget)
    for cache in caches:
        if cache["id"] not in selected:
            continue
        print(
            f"{'Delete' if args.apply else 'Would delete'} {cache['key']} ({cache['size_in_bytes']} B)"
        )
        if args.apply:
            try:
                api(f"actions/caches/{cache['id']}", method="DELETE")
            except urllib.error.HTTPError as error:
                if error.code != 404:  # GitHub may have evicted it since the listing.
                    raise


if __name__ == "__main__":
    main()
