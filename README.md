How to run the wlan simulation:

Clone the relevant repositories from GitHub into "Projects" directory.

Make wpa_supplicant and hostapd as instructed.

Run:

```
sudo modprobe mac80211_hwsim radios=3

sudo systemctl stop NetworkManager.service

sudo macchanger --mac=00:11:00:00:00:00 wlan0
sudo macchanger --mac=00:22:00:00:00:00 wlan1
sudo macchanger --mac=00:33:00:00:00:00 wlan2

sudo ~/Projects/hostap-wpa3/hostapd/hostapd hostapd_wpa3.conf -dd -K
sudo ~/Projects/hostap-wpa3/wpa_supplicant/wpa_supplicant -D nl80211 -i wlan1 -c supp_wpa3.conf -dd -K
```

Now we have a simulated AP running on wlan0 with MAC 00:11:00:00:00:00, and a simulated client running on wlan1 with MAC
00:22:00:00:00:00.

To actually gather data, run:

```
sudo ~/Projects/dragondrain-and-time/src/dragontime -d wlan2 -a 00:11:00:00:00:00 -c 1 -g 22 -i 250 -t 750 -o measurements.txt
```

Which will attack through wlan2 the AP and gather the measurements.