#!/bin/sh

GROUP_NAME="dragonblood_group_2"

# Creates $GROUP_NAME folder
rm -rf $GROUP_NAME || return
mkdir $GROUP_NAME

# Copies code
mkdir $GROUP_NAME/code
cp setup_mocked.sh $GROUP_NAME/code/setup_mocked.sh
cp README.md $GROUP_NAME/code/README.md
cp -r admin $GROUP_NAME/code/admin
(cd $GROUP_NAME/code/admin && rm -f hostapd hostapd_current.conf MAC_AP hostap-wpa3/hostapd/hostapd)
cp -r user $GROUP_NAME/code/user
(cd $GROUP_NAME/code/user && rm -f fingerprint dragontime MAC_AP MAC_USER DEVICE fingerprint-wpa3/hostapd/fingerprint attack/dragontime)

# Copies report
cp -r report $GROUP_NAME/report

# Copies guide
cp -r guide $GROUP_NAME/guide

# Zips it all up
echo "$GROUP_NAME contents:"
ls -a $GROUP_NAME
echo "\ncode contents:"
ls -a $GROUP_NAME/code
echo "\nguide contents:"
ls -a $GROUP_NAME/guide
echo "\nreport contents:"
ls -a $GROUP_NAME/report
tar -czf dragonblood_group_2.tar.gz $GROUP_NAME
rm -rf $GROUP_NAME
