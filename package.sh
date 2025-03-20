#!/bin/sh

# Creates package folder
rm -rf package || return
mkdir package

# Copies code
mkdir package/code
cp setup_mocked.sh package/code/setup_mocked.sh
cp README.md package/code/setup_mocked.sh
cp -r admin package/code/admin
(cd package/code/admin && rm -f hostapd hostapd_current.conf MAC_AP hostap-wpa3/hostapd/hostapd)
cp -r user package/code/user
(cd package/code/user && rm -f fingerprint dragontime MAC_AP MAC_USER DEVICE fingerprint-wpa3/hostapd/fingerprint attack/dragontime)

# Copies report
cp -r report package/report

# Copies guide
cp -r guide package/guide

# Zips it all up
echo "package contents:"
ls -a package
echo "\ncode contents:"
ls -a package/code
echo "\nguide contents:"
ls -a package/guide
echo "\nreport contents:"
ls -a package/report
tar -czf dragonblood_group_2.tar.gz package
rm -rf package
