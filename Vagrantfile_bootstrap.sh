#!/usr/bin/env bash

apt-get update
apt-get install -y python3 python3-pip
apt-get install -y build-essential checkinstall
apt-get install -y gcc-arm-none-eabi

pip3 install adafruit-nrfutil
