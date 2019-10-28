FROM p4lang/behavioral-model:latest

# Default to using 2 make jobs, which is a good default for CI. If you're
# building locally or you know there are more cores available, you may want to
# override this.
ARG MAKEFLAGS=-j2

# Select the type of image we're building. Use `build` for a normal build, which
# is optimized for image size. Use `test` if this image will be used for
# testing; in this case, the source code and build-only dependencies will not be
# removed from the image.
ARG IMAGE_TYPE=build

# Whether to do a unified build.
ARG ENABLE_UNIFIED_COMPILATION=ON

# Delegate the build to tools/travis-build.
COPY . /p4c/
WORKDIR /p4c/
RUN chmod u+x tools/travis-build && tools/travis-build
