# Reproducible build and test environment for the IEEE-754 compression engine.
#
#   docker build -t ieee754-compression .
#   docker run --rm ieee754-compression                  # run the gating tests
#   docker build --build-arg CXX=clang++-18 .            # build with clang
#
# Base image pinned by digest so this builds the same way later.
FROM ubuntu@sha256:33ceb71981b602c1a7443a53469e4dba065f7503eab3078a2d7a57a2ab987517

ARG CXX=g++-14

# Exported so both the build below and `make test` at container run time use the
# same compiler and flags. An empty ARCHFLAGS counts as defined, so the
# Makefile's ?= default of -march=native does not reassert itself.
ENV CXX=${CXX}
ENV ARCHFLAGS=

RUN apt-get update && apt-get install -y --no-install-recommends \
        g++-14 \
        clang-18 \
        make \
        python3 \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

# ARCHFLAGS is emptied: the local default of -march=native tunes the binary to
# the building machine's CPU, which defeats the point of a portable image.
RUN make -j"$(nproc)"

CMD ["make", "test"]
