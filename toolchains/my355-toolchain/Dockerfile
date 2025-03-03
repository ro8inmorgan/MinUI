FROM debian:bookworm-slim
ENV DEBIAN_FRONTEND noninteractive

ENV TZ=America/New_York
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt-get -y update && apt-get -y install \
	bc \
    build-essential \
    bzip2 \
	bzr \
	cmake \
	cmake-curses-gui \
	cpio \
	device-tree-compiler \
	git \
	golang \
	libncurses5-dev \
	libsdl1.2-dev \
	libsdl-image1.2-dev \
	libsdl-ttf2.0-dev \
	libsdl2-dev \
	libsdl2-image-dev \
	libsdl2-ttf-dev \
	locales \
	make \
	rsync \
	scons \
	squashfs-tools \
	tree \
	unzip \
	wget \
  && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /root/workspace
WORKDIR /root

VOLUME /root/workspace
WORKDIR /root/workspace

ENV CROSS_COMPILE=/usr/bin/aarch64-linux-gnu-   
ENV PREFIX=/usr 
ENV UNION_PLATFORM=my355

CMD ["/bin/bash"]