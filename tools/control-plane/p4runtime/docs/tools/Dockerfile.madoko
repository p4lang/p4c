FROM ubuntu:16.04
LABEL maintainer="P4 API Working Group <p4-dev@lists.p4.org>"
LABEL description="Dockerfile used for building the Madoko specification"

ENV PKG_DEPS software-properties-common build-essential libreoffice \
    nodejs npm texlive-full fonts-liberation

RUN apt-get update && \
    apt-get install -y --no-install-recommends $PKG_DEPS && \
    apt-get autoremove --purge -y && \
    rm -rf /var/cache/apt/* /var/lib/apt/lists/*

RUN npm install madoko -g && \
    ln -s /usr/bin/nodejs /usr/local/bin/node

VOLUME ["/usr/src/p4-spec"]
WORKDIR /usr/src/p4-spec

# add extra fonts for P4_14 look-alike
RUN mkdir -p /usr/share/fonts/truetype/UtopiaStd /usr/share/fonts/truetype/LuxiMono
COPY fonts/UtopiaStd-Regular.otf /usr/share/fonts/truetype/UtopiaStd/
COPY fonts/luximr.ttf /usr/share/fonts/truetype/LuxiMono/
COPY fonts/fix_helvetica.conf /etc/fonts/local.conf
RUN fc-cache -fv
