FROM ubuntu:22.04 AS build

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential cmake git libtinyxml2-dev && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . /build
RUN ./make.sh -b release -t linux

# Final image
FROM ubuntu:22.04
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    libtinyxml2-9 && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /build/build/linux/release/fsl /app/fsl
COPY src/config.xml /app/config.xml

ENTRYPOINT ["/app/fsl"]
