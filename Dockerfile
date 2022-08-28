FROM ubuntu:focal AS build

ENV DEBIAN_FRONTEND noninteractive

ADD . /workspace
RUN apt-get update \
    && apt-get install -y \
        build-essential git cmake \
        libsdl2-dev libopenal-dev libavcodec-dev libavformat-dev libavutil-dev libswscale-dev

WORKDIR /workspace
RUN cd neo && ./cmake-eclipse-linux-profile.sh
RUN cd build && make

FROM scratch AS export
COPY --from=build /workspace/build/RBDoom3BFG /workspace/base/maps/ /
