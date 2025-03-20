#!/bin/sh

# Setup mocked wlan devices
sudo modprobe -r mac80211_hwsim
sudo modprobe mac80211_hwsim radios=2
sudo macchanger --mac=00:11:00:00:00:00 wlan0
sudo macchanger --mac=00:22:00:00:00:00 wlan1
