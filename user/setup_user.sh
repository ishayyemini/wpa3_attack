# Clones repositories
if [ ! -d "dragondrain-and-time" ]; then
  git clone https://github.com/vanhoefm/dragondrain-and-time.git
  rm -rf dragondrain-and-time/.git
fi

if [ ! -d "hostap-wpa3" ]; then
  git clone https://github.com/vanhoefm/hostap-wpa3.git
  rm -rf hostap-wpa3/.git
fi

# Replaces needed files
cp ../changes/dragonfly.c hostap-wpa3/src/common/dragonfly.c
cp ../changes/sae.c hostap-wpa3/src/common/sae.c
cp ../changes/dh_groups.c hostap-wpa3/src/crypto/dh_groups.c
cp ../changes/dragontime.c dragondrain-and-time/src/dragontime.c

# Compile fingerprint
sed -i 's/hostapd:/fingerprint:/' hostap-wpa3/hostapd/Makefile
sed -i 's/-o hostapd /-o fingerprint /' hostap-wpa3/hostapd/Makefile
sed -i 's/ALL=hostapd hostapd_cli/ALL=fingerprint/' hostap-wpa3/hostapd/Makefile
sed -i 's/int main(/int main1(/' hostap-wpa3/hostapd/main.c
cp ../changes/sae.c hostap-wpa3/src/common/sae.c
sed -i 's/sae->tmp->pwe_ffc ? 0 : -1/counter - 1/' hostap-wpa3/src/common/sae.c
cat ../changes/fingerprint.c >> hostap-wpa3/src/common/sae.c
cp hostap-wpa3/hostapd/defconfig hostap-wpa3/hostapd/.config
(cd hostap-wpa3/hostapd && make -j 2)
cp hostap-wpa3/hostapd/fingerprint fingerprint

# Compile dragontime
sed -i 's/} __packed;/};/' dragondrain-and-time/src/aircrack-osdep/radiotap/radiotap.h
(cd dragondrain-and-time && autoreconf -i)
(cd dragondrain-and-time && ./configure)
(cd dragondrain-and-time && make)
ln -s dragondrain-and-time/src/dragontime dragontime
