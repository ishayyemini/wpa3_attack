#!/bin/sh

# Ensures requirements are installed
sudo apt-get update
sudo apt-get install -y autoconf automake libtool shtool libssl-dev pkg-config libnl-3-dev libnl-genl-3-dev --fix-missing

# Compiles hostapd
(cd hostap-wpa3/hostapd && make)
if [ -e "hostapd" ]; then
  rm -f hostapd
fi
ln -s hostap-wpa3/hostapd/hostapd hostapd

# Stops network manager
sudo systemctl stop NetworkManager.service
