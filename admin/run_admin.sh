#!/bin/sh

DEVICE="${1:-wlan0}"
DEF_CONFIG_FILE="hostapd_default.conf"
CURR_CONFIG_FILE="hostapd_current.conf"
PASSWORDS_FILE="passwords.txt"

# Randomizes password
cp "$DEF_CONFIG_FILE" "$CURR_CONFIG_FILE"
python randomize_password.py "$CURR_CONFIG_FILE" "$DEVICE" "$PASSWORDS_FILE"

# Prints MAC to file
MAC_AP=$(ip link show "$DEVICE" | grep link/ether | awk '{print $2}')
echo "$MAC_AP" > MAC_AP

# Runs hostapd
sudo ./hostapd "$CURR_CONFIG_FILE" -dd -K
