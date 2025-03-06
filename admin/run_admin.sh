#!/bin/sh

DEVICE="${1:-wlan0}"
CONFIG_FILE="hostapd_wpa3.conf"
PASSWORDS_FILE="../passwords.txt"

# Randomizes password
python randomize_password.py "$CONFIG_FILE" "$DEVICE" "$PASSWORDS_FILE"

# Runs hostapd
sudo ./hostapd "$CONFIG_FILE" -dd -K
