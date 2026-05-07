#!/bin/bash

set -euo pipefail

IMAGE_NAME="${1:-p4lang/p4c}"
CONTAINER_ID="$(docker create --rm "${IMAGE_NAME}")"
cleanup() {
  docker rm -f "${CONTAINER_ID}" >/dev/null 2>&1 || true
}
trap cleanup EXIT

docker cp "${CONTAINER_ID}:/p4c/.ccache" .
if [[ ! -d .ccache ]]; then
  echo "Failed to export /p4c/.ccache from ${IMAGE_NAME}." >&2
  echo "Hint: build the image with IMAGE_TYPE=test so /p4c is preserved." >&2
  exit 1
fi
