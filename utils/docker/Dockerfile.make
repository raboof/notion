# http://blog.csicar.de/docker/window-manger/2016/05/24/docker-wm.html

# Xephyr :1 -ac -br -screen 1024x768 -resizeable -reset
# Xephyr :1 -ac -br -screen 1024x768 -screen 1024x768 +xinerama +extension RANDR -resizeable -reset
# Xephyr :1 -ac -br -screen 1024x768 -screen 1024x768 +extension RANDR -resizeable -reset

# docker build -f ../Dockerfile.notion . -t notion && docker run --rm -it -e DISPLAY=:1 --name notion-test -v /tmp/.X11-unix:/tmp/.X11-unix notion
# docker build -f ../Dockerfile.notion . -t notion && docker run --rm -it -e DISPLAY=:1 --name notion-test -v /tmp/.X11-unix:/tmp/.X11-unix --entrypoint /bin/bash notion
# docker exec -it `docker ps --filter "name=notion-test" -q` /bin/bash

FROM ubuntu:18.04

ENV DEBIAN_FRONTEND=noninteractive
RUN echo 'Acquire::http { Proxy "http://172.17.0.1:3142"; };' >> /etc/apt/apt.conf.d/01proxy
RUN apt update && apt install -y pkg-config build-essential groff

RUN apt update && apt install -y libx11-dev libxext-dev libsm-dev libxft-dev libxinerama-dev libxrandr-dev gettext x11-utils \
 xterm x11-xserver-utils wget unzip xserver-xorg-video-dummy

# RUN apt update && apt install -y lua5.1 liblua5.1-dev
RUN apt update && apt install -y lua5.2 liblua5.2-dev
# RUN apt update && apt install -y lua5.3 liblua5.3-dev

# https://bugs.launchpad.net/ubuntu/+source/lua-posix/+bug/1752082
RUN apt update && apt install -y lua-posix \
 && ln -s /usr/lib/x86_64-linux-gnu/lua/5.2/posix_c.so /usr/lib/x86_64-linux-gnu/lua/5.2/posix.so

# If lua-posix package is not available, use luarocks
# RUN wget https://luarocks.org/releases/luarocks-3.0.4.tar.gz \
#  && tar zxpf luarocks-3.0.4.tar.gz \
#  && cd luarocks-3.0.4 \
#  && ./configure \
#  && make build && make install
# RUN luarocks install luaposix

# Icon branch
# RUN apt update && apt install -y libcairo2-dev

RUN mkdir /notion
WORKDIR /notion
COPY . /notion/
RUN make clean
RUN make
ENTRYPOINT ["/bin/bash"]
