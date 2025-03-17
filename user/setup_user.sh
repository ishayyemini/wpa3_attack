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
echo '#include "utils/includes.h"
#include "utils/common.h"
#include "crypto/crypto.h"
#include "crypto/dh_groups.h"
#include "common/sae.h"
' > fingerprint-wpa3/hostapd/main.c
cat finger/fingerprint.c >> fingerprint-wpa3/hostapd/main.c
sed -i 's/#include "fingerprint.h"//' fingerprint-wpa3/hostapd/main.c
cp finger/sae.c fingerprint-wpa3/src/common/sae.c
(cd fingerprint-wpa3/hostapd && make)
if [ -e "fingerprint" ]; then
  rm -f fingerprint
fi
ln -s fingerprint-wpa3/hostapd/fingerprint fingerprint

# Compiles dragontime
(cd attack && make)
if [ -e "dragontime" ]; then
  rm -f dragontime
fi
ln -s attack/dragontime dragontime

# Stops network manager
sudo systemctl stop NetworkManager.service
