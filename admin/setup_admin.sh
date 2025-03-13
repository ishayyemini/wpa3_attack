#!/bin/sh

CHANGES_DIR="../changes"

# Ensures requirements are installed
apt-get update
apt-get install -y autoconf automake libtool shtool libssl-dev pkg-config

# Clones repositories
if [ ! -d "hostap-wpa3" ]; then
  git clone https://github.com/vanhoefm/hostap-wpa3.git
  rm -rf hostap-wpa3/.git
fi

# Replaces needed files
cp $CHANGES_DIR/dragonfly.c hostap-wpa3/src/common/dragonfly.c
cp $CHANGES_DIR/sae.c hostap-wpa3/src/common/sae.c
cp $CHANGES_DIR/dh_groups.c hostap-wpa3/src/crypto/dh_groups.c

# Compiles hostapd
cp hostap-wpa3/hostapd/defconfig hostap-wpa3/hostapd/.config
sed -i 's/#CONFIG_DRIVER_WIRED=y/CONFIG_DRIVER_WIRED=y/' hostap-wpa3/hostapd/.config
(cd hostap-wpa3/hostapd && make -j 2)
cp hostap-wpa3/hostapd/hostapd hostapd

# Stops network manager
sudo systemctl stop NetworkManager.service
