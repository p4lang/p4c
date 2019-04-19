FROM p4lang/third-party:latest
LABEL maintainer="P4 API Working Group <p4-dev@lists.p4.org>"
LABEL description="Dockerfile used for CI testing of p4lang/p4runtime"

COPY . /p4runtime/
WORKDIR /p4runtime/
RUN apt-get update && \
    apt-get install -y --no-install-recommends software-properties-common git
