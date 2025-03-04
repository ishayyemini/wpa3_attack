 Clones repositories
if [ ! -d "dragondrain-and-time" ]; then
  git clone https://github.com/vanhoefm/dragondrain-and-time.git
  rm -rf dragondrain-and-time/.git
fi

if [ ! -d "hostap-wpa3" ]; then
  git clone https://github.com/vanhoefm/hostap-wpa3.git
  rm -rf hostap-wpa3/.git
fi

# Replaces needed files
cp changes/dragonfly.c hostap-wpa3/src/common/dragonfly.c
cp changes/sae.c hostap-wpa3/src/common/sae.c
cp changes/dh_groups.c hostap-wpa3/src/crypto/dh_groups.c
cp changes/dragontime.c dragondrain-and-time/src/dragontime.c

if [ -d "fingerprint-dir" ]; then
  rm -rf fingerprint-dir
fi
cp -r hostap-wpa3 fingerprint-dir

# Compile hostapd
cp hostap-wpa3/hostapd/defconfig hostap-wpa3/hostapd/.config
sed -i 's/#CONFIG_DRIVER_WIRED=y/CONFIG_DRIVER_WIRED=y/' hostap-wpa3/hostapd/.config
(cd hostap-wpa3/hostapd && make -j 2)
cp hostap-wpa3/hostapd/hostapd hostapd

# Compile fingerprint
sed -i 's/hostapd:/fingerprint:/' fingerprint-dir/hostapd/Makefile
sed -i 's/-o hostapd /-o fingerprint /' fingerprint-dir/hostapd/Makefile
sed -i 's/ALL=hostapd hostapd_cli/ALL=fingerprint/' fingerprint-dir/hostapd/Makefile
sed -i 's/int main(/int main1(/' fingerprint-dir/hostapd/main.c
cp changes/sae.c fingerprint-dir/src/common/sae.c
sed -i 's/sae->tmp->pwe_ffc ? 0 : -1/counter - 1/' fingerprint-dir/src/common/sae.c
cat changes/fingerprint.c >> fingerprint-dir/src/common/sae.c
cp fingerprint-dir/hostapd/defconfig fingerprint-dir/hostapd/.config
(cd fingerprint-dir/hostapd && make -j 2)
cp fingerprint-dir/hostapd/fingerprint fingerprint

# Compile dragontime
sed -i 's/} __packed;/};/' dragondrain-and-time/src/aircrack-osdep/radiotap/radiotap.h
(cd dragondrain-and-time && autoreconf -i)
(cd dragondrain-and-time && ./configure)
(cd dragondrain-and-time && make)
ln -s dragondrain-and-time/src/dragontime dragontime

# Setup mocked wlan devices
sudo modprobe -r mac80211_hwsim
sudo modprobe mac80211_hwsim radios=3
sudo systemctl stop NetworkManager.service
sudo macchanger --mac=00:11:00:00:00:00 wlan0
sudo macchanger --mac=00:22:00:00:00:00 wlan1
sudo macchanger --mac=00:33:00:00:00:00 wlan2
