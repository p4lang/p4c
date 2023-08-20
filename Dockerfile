ARG BASE_IMAGE=p4lang/behavioral-model:latest
FROM ${BASE_IMAGE}
LABEL maintainer="P4 Developers <p4-dev@lists.p4.org>"

# Default to using 2 make jobs, which is a good default for CI. If you're
# building locally or you know there are more cores available, you may want to
# override this.
ARG MAKEFLAGS=-j2
# Useful environment variable for scripts.
ARG IN_DOCKER=TRUE
# Select the type of image we're building. Use `build` for a normal build, which
# is optimized for image size. Use `test` if this image will be used for
# testing; in this case, the source code and build-only dependencies will not be
# removed from the image.
ARG IMAGE_TYPE=build
# Whether to do a unity build.
ARG CMAKE_UNITY_BUILD=ON
# Whether to enable translation validation
ARG VALIDATION=OFF
# This creates a release build that includes link time optimization and links
# all libraries statically.
ARG BUILD_STATIC_RELEASE=OFF
# No questions asked during package installation.
ARG DEBIAN_FRONTEND=noninteractive
# Whether to install dependencies required to run PTF-ebpf tests
ARG INSTALL_PTF_EBPF_DEPENDENCIES=OFF
# Whether to build the P4Tools back end and platform.
ARG ENABLE_TEST_TOOLS=ON
# Whether to treat warnings as errors.
ARG ENABLE_WERROR=ON
# Compile with Clang compiler
ARG COMPILE_WITH_CLANG=OFF
# Compile with sanitizers (UBSan, ASan)
ARG ENABLE_SANITIZERS=OFF
# Only execute the steps necessary to successfully run CMake.
ARG CMAKE_ONLY=OFF
# Build with -ftrivial-auto-var-init=pattern to catch more bugs caused by
# uninitialized variables.
ARG BUILD_AUTO_VAR_INIT_PATTERN=OFF

# Configuration of ASAN and UBSAN sanitizers:
# - Print symbolized stack trace for each error report.
# - Disable leaks detector as p4c uses GC.
ENV UBSAN_OPTIONS=print_stacktrace=1
ENV ASAN_OPTIONS=print_stacktrace=1:detect_leaks=0

# Delegate the build to tools/ci-build.
COPY . /p4c/
RUN /p4c/tools/ci-build.sh
# Set the workdir after building p4c.
WORKDIR /p4c/
