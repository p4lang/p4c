#!/usr/bin/env bash

# Install BMv2 from source using CMake.

set -euo pipefail
set -x

usage() {
  cat <<'USAGE'
Usage: install_bmv2_from_source.sh [options]

Options:
  --ref <ref>                Git ref (branch, tag, or commit).
  --repo-url <url>           BMv2 repository URL.
  --prefix <path>            Install prefix (default: /usr/local).
  --jobs <N>                 Number of build jobs (default: nproc+1 or hw.ncpu+1).
  --work-dir <path>          Parent work directory (default: temporary dir).
  --cmake-arg <arg>          Additional CMake configure argument (repeatable).
  --build-arg <arg>          Additional CMake build argument (repeatable).
  --install-arg <arg>        Additional CMake install argument (repeatable).
  --help                     Show this help and exit.
USAGE
}

default_jobs() {
  if command -v nproc >/dev/null 2>&1; then
    echo $(( $(nproc) + 1 ))
    return
  fi

  if command -v sysctl >/dev/null 2>&1; then
    local ncpu
    ncpu=$(sysctl -n hw.ncpu 2>/dev/null || echo 2)
    echo $(( ncpu + 1 ))
    return
  fi

  echo 2
}

run_install() {
  local prefix="$1"
  shift

  if [[ -w "$prefix" || ( ! -e "$prefix" && -w "$(dirname "$prefix")" ) || "${EUID}" -eq 0 ]]; then
    cmake --install build "$@"
    return
  fi

  if command -v sudo >/dev/null 2>&1; then
    sudo cmake --install build "$@"
    return
  fi

  echo "Install prefix '$prefix' is not writable and sudo is unavailable." >&2
  exit 1
}

REPO_URL="https://github.com/p4lang/behavioral-model"
REF=""
PREFIX="/usr/local"
JOBS="$(default_jobs)"
WORK_DIR=""
CMAKE_ARGS=()
BUILD_ARGS=()
INSTALL_ARGS=()

while [[ $# -gt 0 ]]; do
  case "$1" in
    --ref)
      REF="$2"
      shift 2
      ;;
    --repo-url)
      REPO_URL="$2"
      shift 2
      ;;
    --prefix)
      PREFIX="$2"
      shift 2
      ;;
    --jobs)
      JOBS="$2"
      shift 2
      ;;
    --work-dir)
      WORK_DIR="$2"
      shift 2
      ;;
    --cmake-arg)
      CMAKE_ARGS+=("$2")
      shift 2
      ;;
    --build-arg)
      BUILD_ARGS+=("$2")
      shift 2
      ;;
    --install-arg)
      INSTALL_ARGS+=("$2")
      shift 2
      ;;
    --help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage
      exit 1
      ;;
  esac
done

cleanup_workdir=false
if [[ -z "$WORK_DIR" ]]; then
  WORK_DIR="$(mktemp -d -t bmv2-build-XXXXXXXXXX)"
  cleanup_workdir=true
else
  mkdir -p "$WORK_DIR"
fi

if [[ "$cleanup_workdir" == true ]]; then
  trap 'rm -rf "$WORK_DIR"' EXIT
fi

pushd "$WORK_DIR"
git clone --filter=blob:none "$REPO_URL" behavioral-model
cd behavioral-model

if [[ -n "$REF" ]]; then
  git fetch --depth=1 origin "$REF"
  git checkout --detach FETCH_HEAD
fi

BMV2_SHA=$(git rev-parse HEAD)
echo "Building BMv2 from $REPO_URL @ $BMV2_SHA"

cmake -S . -B build -DCMAKE_INSTALL_PREFIX="$PREFIX" "${CMAKE_ARGS[@]}"
cmake --build build --parallel "$JOBS" "${BUILD_ARGS[@]}"
run_install "$PREFIX" "${INSTALL_ARGS[@]}"
popd
