FROM ubuntu:20.04

ENV https_proxy=http://ow.lixinrui000.cn:7890

ENV DEBIAN_FRONTEND=noninteractive \
    TZ=UTC \
    APT_LISTCHANGES_FRONTEND=none

# Keep upstream mirrors; add retries and clean cache properly
RUN apt-get update -o Acquire::Retries=5 && \
    apt-get install -y --no-install-recommends \
      build-essential \
      cmake \
      git \
      pkg-config \
      numactl \
      libnuma-dev \
      libtbb-dev \
      libpmem-dev \
      libpmemobj-dev \
      libssh-dev \
      libssl-dev \
      libatomic1 \
      autoconf \
      automake \
      libtool \
      libdaxctl-dev \
      libhwloc-dev \
      libjemalloc-dev \
      libboost-all-dev \
      ca-certificates \
      curl \
      python3 \
      python3-pip && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Default workdir; the project will be bind-mounted here at runtime
WORKDIR /workspace

# Optional: Build and install memkind from source. Disabled by default since libmemkind-dev is installed above.
ARG MEMKIND_FROM_SOURCE=0
ARG MEMKIND_VERSION=master
RUN if [ "$MEMKIND_FROM_SOURCE" = "1" ]; then \
      set -e; \
      git config --global advice.detachedHead false; \
      rm -rf /tmp/memkind; \
      success=""; \
      for URL in \
        # https://github.com/memkind/memkind.git \
        https://ghfast.top/https://github.com/memkind/memkind.git; do \
        echo "Cloning memkind from $URL"; \
        if git clone --branch ${MEMKIND_VERSION} "$URL" /tmp/memkind; then \
          success=1; \
          break; \
        else \
          rm -rf /tmp/memkind; \
        fi; \
      done; \
      test -n "$success" && test -d /tmp/memkind; \
      cd /tmp/memkind && \
      ( ./autogen.sh || (autoreconf -fi && ./autogen.sh) ) && \
      ./configure --prefix=/usr && \
      make -j"$(nproc)" && \
      make install && \
      ldconfig && \
      cd / && rm -rf /tmp/memkind; \
    else \
      echo "Skipping memkind source build; using distro package libmemkind-dev"; \
    fi

CMD ["bash"]


