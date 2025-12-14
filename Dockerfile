# Set build type (debug or release), default to debug
ARG BUILD_TYPE=debug

# Set build target (linux or petalinux), default to linux
ARG TARGET=linux


FROM ubuntu:22.04 AS build

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential cmake git libtinyxml2-dev && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . /build
RUN ./make.sh -b ${BUILD_TYPE} -t ${TARGET}

# Final image
FROM ubuntu:22.04
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    libtinyxml2-9 && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /build/build/${TARGET}/${BUILD_TYPE}/fsl /app/fsl
COPY src/config.xml /app/config.xml

ENTRYPOINT ["/app/fsl"]
