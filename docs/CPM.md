<!--
SPDX-FileCopyrightText: 2026 The P4 Language Consortium

SPDX-License-Identifier: Apache-2.0
-->

# Managing dependencies with CPM

P4C uses [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake) to obtain its CMake dependencies.
By default, P4C builds the source revisions pinned in its CMake files (`CPM_DOWNLOAD_ALL=ON`). CMake downloads CPM automatically during configuration; no separate CPM installation is needed.

Package names in CPM options are case-sensitive:

| Dependency | CPM name |
| --- | --- |
| Abseil | `absl` |
| Protobuf | `Protobuf` |
| Boehm-Demers-Weiser GC | `LibGc` |
| Multiprecision | `multiprecision` |
| Z3 | `Z3` |
| GoogleTest | `GTest` |
| Inja | `inja` |
| spdlog | `spdlog` |
| P4Runtime | `p4runtime` |

## Choosing installed packages or source builds

Use `CPM_DOWNLOAD_<name>=OFF` to search for an installed package first. If CMake
cannot find a suitable package, CPM falls back to the pinned source revision.
For example:

```sh
cmake -S . -B build \
  -DCPM_DOWNLOAD_Protobuf=OFF \
  -DCMAKE_PREFIX_PATH=/opt/protobuf
```

`CMAKE_PREFIX_PATH` lists installation prefixes. For dependencies that provide a
CMake package configuration, `<name>_DIR` can point directly to the directory
containing that configuration, such as `-Dabsl_DIR=/opt/abseil/lib/cmake/absl`.

P4C builds Protobuf as a static library when it obtains the sources. For an
installed Protobuf package, its exported targets determine whether the library
is static or shared. Select the desired installation through `Protobuf_DIR` or
`CMAKE_PREFIX_PATH`.

The following options apply across dependencies:

| Option | Behavior |
| --- | --- |
| `CPM_DOWNLOAD_ALL=ON` | Build pinned sources by default. This is P4C's default. |
| `CPM_DOWNLOAD_<name>=ON` | Select a source build for one package. |
| `CPM_DOWNLOAD_<name>=OFF` | Prefer an installed package, with source fallback. |
| `CPM_USE_LOCAL_PACKAGES=ON` | Prefer installed packages for all library dependencies, with source fallback. |
| `CPM_LOCAL_PACKAGES_ONLY=ON` | Require installed packages for all library dependencies. |

An explicit `CPM_DOWNLOAD_<name>` overrides `CPM_DOWNLOAD_ALL` for that package.
However, `CPM_USE_LOCAL_PACKAGES` and `CPM_LOCAL_PACKAGES_ONLY` also affect source
requests. Leave both off when you want pinned source builds. For example, in a
build directory previously configured to prefer installed packages:

```sh
cmake -S . -B build \
  -DCPM_USE_LOCAL_PACKAGES=OFF \
  -DCPM_LOCAL_PACKAGES_ONLY=OFF \
  -DCPM_DOWNLOAD_Protobuf=ON
```

To build a dependency from your own checkout, set `CPM_<name>_SOURCE`:

```sh
cmake -S . -B build -DCPM_absl_SOURCE=/path/to/abseil-cpp
```

An explicit checkout takes precedence over installed-package discovery, including
`CPM_LOCAL_PACKAGES_ONLY`. Packages already registered by a parent project are
reused before considering a checkout override.

Some dependencies have additional requirements:

- Installed Multiprecision uses the matching Boost headers. Fetched Multiprecision
  uses standalone mode with the pinned Boost.Config headers.
- Installed P4Runtime must provide a version 1.6 or newer CMake package with the
  `cpp` component. Its schemas are also needed for Python bindings and backend
  imports. See [P4Runtime](../README.md#p4runtime).
- Installed Z3 must be compatible with P4C's garbage collector. CMake checks the
  version and platform when `ENABLE_GC=ON`.
- bpftool and Boost.Config are source-only dependencies. CPM can fetch them even
  when `CPM_LOCAL_PACKAGES_ONLY=ON`. The eBPF build scripts require bpftool at
  `backends/ebpf/runtime/contrib/bpftool`.

## Selecting a dependency version

To select a particular release, check out that release and pass its directory
through `CPM_<name>_SOURCE`. For example, to build P4C with Protobuf 25.4 instead
of the pinned release, run these commands from the P4C repository root:

```sh
git clone --branch v25.4 --depth 1 --recurse-submodules --shallow-submodules \
  https://github.com/protocolbuffers/protobuf.git "$HOME/src/protobuf-v25.4"
cmake -S . -B build-protobuf-v25.4 \
  -DCPM_Protobuf_SOURCE="$HOME/src/protobuf-v25.4"
cmake --build build-protobuf-v25.4
```

Replace `v25.4` with the desired tag, or use an existing checkout at a specific
commit. CPM uses that checkout with P4C's Protobuf build options. Protobuf's
library and `protoc` are built together, and Protobuf reuses the Abseil targets
selected by P4C. If that release requires a different Abseil version, select its
checkout with `CPM_absl_SOURCE` as well.

P4C does not provide a `CPM_Protobuf_VERSION` command-line option. The source
override selects the version without editing the repository's dependency pins.
To change the default version for all builds, update the version and archive
checksum in [cmake/Protobuf.cmake](../cmake/Protobuf.cmake).

## Reusing downloaded sources

Set `CPM_SOURCE_CACHE` to share dependency sources across build directories and
P4C checkouts:

```sh
export CPM_SOURCE_CACHE="$HOME/.cache/CPM"
cmake -S . -B build
```

You can also pass `-DCPM_SOURCE_CACHE=/path/to/cache` to CMake. The cache contains
dependency sources and the pinned CPM script. Each build directory still compiles
its own libraries; use ccache to reuse compilation results. bpftool remains at
the fixed path in the P4C checkout rather than in this shared cache.

Before configuring offline, populate the cache in an online build with the same
dependencies and backends enabled. An eBPF build also needs the bpftool sources in
its checkout. When reusing a build directory from before the CPM migration,
configure once with `-DFETCHCONTENT_FULLY_DISCONNECTED=OFF` so that CMake can
populate dependencies whose names changed. Restore offline mode after that
initial configuration.

## Using P4C as a subproject

When a parent project includes P4C with `add_subdirectory`, P4C reuses the parent's
loaded CPM instance, settings, source cache, and packages registered under the
names above. The first project to add a package determines its version and build options.
P4C does not reconfigure or patch packages that the parent has already supplied;
in particular, a parent-provided Z3 must be compatible with P4C's garbage collector.

For example, if the parent adds `absl` before P4C, P4C uses that version instead
of its pinned Abseil release. When CPM has version metadata, it warns if a later
request needs a newer version, but continues using the version already added.
CPM does not resolve version constraints or upgrade an existing package. The
parent must select a version compatible with all consumers, including P4C and
Protobuf. An already registered package also takes precedence over a
`CPM_absl_SOURCE` override.

Options passed through CPM's `OPTIONS` argument apply in the dependency's scope,
so P4C does not need to save and restore each option in the global CMake cache.
P4C adds its standalone developer targets (`check`, `uninstall`, and linters) and
CPack configuration when it is the top-level project. Backend test environments
still require their usual setup.

## Checking the CPM integration

The configure-only checks use small local packages and require no compiler.
They exercise installed-package discovery, source selection and overrides,
reuse of a parent's package version and options, and source-cache reuse across
build directories. Once the CPM script is cached, they can run offline:

```sh
cmake -S cmake/tests -B build/cpm-tests -DCPM_SOURCE_CACHE="$HOME/.cache/CPM"
ctest --test-dir build/cpm-tests --output-on-failure
```
