FROM hub.oepkgs.net/openeuler/openeuler:latest

ENV TZ=UTC

# Install development tools and dependencies using dnf (openEuler package manager)
RUN dnf update -y && \
    dnf install -y --setopt=install_weak_deps=False \
      gcc \
      gcc-c++ \
      make \
      cmake \
      git \
      pkgconfig \
      numactl \
      numactl-devel \
      tbb-devel \
      libpmem-devel \
      libpmemobj-devel \
      libssh-devel \
      openssl-devel \
      libatomic \
      autoconf \
      automake \
      libtool \
      daxctl-devel \
      hwloc-devel \
      jemalloc-devel \
      boost-devel \
      ca-certificates \
      curl \
      python3 \
      python3-pip \
      which \
      tar \
      wget \
      sudo \
      glibc-devel \
      libstdc++-devel && \
    dnf clean all

# Set up openEuler specific configurations
RUN echo "LANG=en_US.UTF-8" >> /etc/environment && \
    echo "LC_ALL=en_US.UTF-8" >> /etc/environment

# Default workdir; the project will be bind-mounted here at runtime
WORKDIR /workspace

# Create a non-root user for development
RUN useradd -m -s /bin/bash developer && \
    usermod -aG wheel developer && \
    echo "developer ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# Set up git configuration for the container
RUN git config --global init.defaultBranch main && \
    git config --global pull.rebase false

# Install cityhash library from source
RUN echo "Installing cityhash from source..."; \
    cd /tmp && \
    git clone https://github.com/google/cityhash.git && \
    cd cityhash && \
    ./configure --prefix=/usr && \
    make -j"$(nproc)" && \
    make install && \
    ldconfig && \
    cd / && rm -rf /tmp/cityhash

# Install memkind from openEuler repository or build from source
ARG MEMKIND_FROM_SOURCE=0
ARG MEMKIND_VERSION=master
RUN if [ "$MEMKIND_FROM_SOURCE" = "1" ]; then \
      set -e; \
      git config --global advice.detachedHead false; \
      rm -rf /tmp/memkind; \
      success=""; \
      for URL in \
        https://github.com/memkind/memkind.git \
        https://gitee.com/mirrors/memkind.git; do \
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
      echo "Installing memkind from openEuler repository"; \
      dnf install -y memkind-devel || echo "memkind-devel not available in repository, will build from source if needed"; \
    fi

# Switch to developer user and set up the environment
USER developer
WORKDIR /workspace

# Set up environment variables for the developer user
ENV HOME=/home/developer
ENV USER=developer

# Create a startup script that shows useful information
RUN echo '#!/bin/bash' > /home/developer/startup.sh && \
    echo 'echo "=== openEuler Development Environment ==="' >> /home/developer/startup.sh && \
    echo 'echo "OS: $(cat /etc/os-release | grep PRETTY_NAME)"' >> /home/developer/startup.sh && \
    echo 'echo "Kernel: $(uname -r)"' >> /home/developer/startup.sh && \
    echo 'echo "Architecture: $(uname -m)"' >> /home/developer/startup.sh && \
    echo 'echo "Available tools: gcc, cmake, git, numactl, etc."' >> /home/developer/startup.sh && \
    echo 'echo "Working directory: $(pwd)"' >> /home/developer/startup.sh && \
    echo 'echo "========================================="' >> /home/developer/startup.sh && \
    echo 'exec "$@"' >> /home/developer/startup.sh && \
    chmod +x /home/developer/startup.sh

ENTRYPOINT ["/home/developer/startup.sh"]
CMD ["bash"]


