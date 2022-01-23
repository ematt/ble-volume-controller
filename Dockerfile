FROM ubuntu:18.04

RUN mkdir /builds && \
    apt-get update && \
    apt-get upgrade -y && \
    apt-get install -y make cmake zip curl wget git doxygen graphviz python3 python3-pip && \
    wget -O archive.tar.bz2 "https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2020q2/gcc-arm-none-eabi-9-2020-q2-update-x86_64-linux.tar.bz2?revision=05382cca-1721-44e1-ae19-1e7c3dc96118&hash=AE874AE7513FAE5077350E4E23B1AC08" && \
    echo 2b9eeccc33470f9d3cda26983b9d2dc6 archive.tar.bz2 > /tmp/archive.md5 && md5sum -c /tmp/archive.md5 && rm /tmp/archive.md5 && \
    tar xf archive.tar.bz2 -C /opt && \
    rm archive.tar.bz2 && \
    apt-get autoclean -y && \
    apt-get autoremove -y && \
    apt-get clean

ENV PATH=/opt/gcc-arm-none-eabi-9-2020-q2-update/bin:$PATH

WORKDIR /builds

ENV LANG C.UTF-8
ENV LC_ALL C.UTF-8

RUN apt-get update && \
    apt-get upgrade -y && \
    apt-get install -y libusb-1.0-0 && \
    pip3 install pc-ble-driver-py nrfutil && \
    nrfutil version && \
    apt-get autoclean -y && \
    apt-get autoremove -y && \
    apt-get clean