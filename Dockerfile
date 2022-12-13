ARG BASE_IMAGE=p4lang/behavioral-model:latest
FROM ${BASE_IMAGE}
LABEL maintainer="P4 Developers <p4-dev@lists.p4.org>"

ENV MAKEFLAGS=-j2

# Select the type of image we're building. Use `build` for a normal build, which
# is optimized for image size. Use `test` if this image will be used for
# testing; in this case, the source code and build-only dependencies will not be
# removed from the image.
ARG IMAGE_TYPE=build
ENV IMAGE_TYPE=$IMAGE_TYPE
# Whether to do a unified build.
ARG ENABLE_UNIFIED_COMPILATION=ON
ENV ENABLE_UNIFIED_COMPILATION=$ENABLE_UNIFIED_COMPILATION
# Whether to enable translation validation
ARG VALIDATION=OFF
ENV VALIDATION=$VALIDATION
# This creates a release build that includes link time optimization and links
# all libraries statically.
ARG BUILD_STATIC_RELEASE=OFF
ENV BUILD_STATIC_RELEASE=$BUILD_STATIC_RELEASE
# Whether to install dependencies required to run PTF-ebpf tests
ARG INSTALL_PTF_EBPF_DEPENDENCIES=OFF
ENV INSTALL_PTF_EBPF_DEPENDENCIES=$INSTALL_PTF_EBPF_DEPENDENCIES
# Whether to build the P4Tools back end and platform.
ARG ENABLE_TEST_TOOLS=OFF
ENV ENABLE_TEST_TOOLS=$ENABLE_TEST_TOOLS

# Delegate the build to tools/ci-build.
COPY . /p4c/
RUN /p4c/tools/ci-build.sh
# Set the workdir after building p4c.
WORKDIR /p4c/
