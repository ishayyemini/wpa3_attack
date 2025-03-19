#!/bin/sh

CURR_CONFIG_FILE="hostapd_current.conf"

# Runs hostapd
sudo ./hostapd "$CURR_CONFIG_FILE" -dd -K
