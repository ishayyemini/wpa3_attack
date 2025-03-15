#!/bin/sh

MAC_AP="$1"
DEVICE="${2:-wlan0}"

if [ -z ${1+x} ]; then
  echo "Access point MAC must be provided"
  exit 1
fi

# Ensures requirements are installed
sudo apt-get update
sudo apt-get install -y autoconf automake libtool shtool libssl-dev pkg-config libnl-3-dev libnl-genl-3-dev --fix-missing

# Inputs MACs
echo "$MAC_AP" > MAC_AP
echo "$DEVICE" > DEVICE
MAC_USER=$(ip link show "$DEVICE" | grep link/ether | awk '{print $2}')
echo "$MAC_USER" > MAC_USER

# Compiles fingerprint
cp fingerprint-wpa3/hostapd/defconfig fingerprint-wpa3/hostapd/.config
(cd fingerprint-wpa3/hostapd && make)
if [ -e "fingerprint" ]; then
  rm -f fingerprint
fi
ln -s fingerprint-wpa3/hostapd/fingerprint fingerprint

# Compiles dragontime
(cd dragondrain-and-time && autoreconf -i)
(cd dragondrain-and-time && ./configure)
(cd dragondrain-and-time && make)
if [ -e "dragontime" ]; then
  rm -f dragontime
fi
ln -s dragondrain-and-time/src/dragontime dragontime

# Stops network manager
sudo systemctl stop NetworkManager.service
