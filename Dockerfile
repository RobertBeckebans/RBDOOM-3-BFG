FROM ubuntu:focal AS rbdoom3bfg-buildenv
ENV DEBIAN_FRONTEND=noninteractive

# Add the Vulkan SDK repository
RUN apt-get update \
    && apt-get install -y curl gpg\
    && curl -Lso /tmp/lunarg-signing-key-pub.asc \
        http://packages.lunarg.com/lunarg-signing-key-pub.asc \
    && apt-key add /tmp/lunarg-signing-key-pub.asc \
    && curl -Lso /etc/apt/sources.list.d/lunarg-vulkan-focal.list \
        http://packages.lunarg.com/vulkan/lunarg-vulkan-focal.list

# Install the general RBDoom3BFG dependencies
RUN apt-get update \
    && apt-get install -y \
        build-essential git cmake \
        libsdl2-dev libopenal-dev vulkan-sdk \
        libavcodec-dev libavformat-dev libavutil-dev libswscale-dev

# From the previous build environment stage, actually run the build
FROM rbdoom3bfg-buildenv AS build

ARG CONFIGSCRIPT=cmake-eclipse-linux-profile.sh
ENV CONFIGSCRIPT=${CONFIGSCRIPT}

ADD . /workspace
WORKDIR /workspace
RUN cd neo && ./${CONFIGSCRIPT}
RUN cd build && make

# Copy only the finished RBDoom3BFG executable to an empty image
# for use with docker build --output
FROM scratch AS export
COPY --from=build /workspace/build/RBDoom3BFG /workspace/base/maps/ /
