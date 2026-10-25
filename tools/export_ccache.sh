#!/bin/bash

# SPDX-FileCopyrightText: 2022 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

# Export the ccache directory from a built p4c Docker image into the current working directory.

set -euo pipefail

IMAGE_NAME="${1:-p4lang/p4c}"
WORK_DIR="$(mktemp -d)"
CONTAINER_ID="$(docker create --rm "${IMAGE_NAME}")"
cleanup() {
  docker rm -f "${CONTAINER_ID}" >/dev/null 2>&1 || true
  # WORK_DIR is created by mktemp above.
  rm -rf "${WORK_DIR}"
}
trap cleanup EXIT

if ! docker cp "${CONTAINER_ID}:/p4c/.ccache" "${WORK_DIR}/.ccache"; then
  echo "Failed to export /p4c/.ccache from ${IMAGE_NAME}." >&2
  echo "Hint: build the image with IMAGE_TYPE=test so /p4c is preserved." >&2
  exit 1
fi

# Swap the exported cache into place, replacing any previously restored cache.
# The replaced cache is discarded together with WORK_DIR on exit.
if [[ -e .ccache ]]; then
  mv .ccache "${WORK_DIR}/.ccache.previous"
fi
mv "${WORK_DIR}/.ccache" .ccache
