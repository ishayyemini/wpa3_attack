#!/bin/sh

LEVEL="${1}"
DEVICE="${2:-wlan0}"
FOLDER="user_ctf_$LEVEL"
DEF_CONFIG_FILE="hostapd_default.conf"
CURR_CONFIG_FILE="hostapd_current.conf"
PASSWORDS_FILE="passwords.txt"
CTFD_FOLDER="./CTFd"

if [ -z ${1+x} ]; then
  echo "Level must be specified"
  exit 1
fi

if [ "$LEVEL" -ne 1 ] && [ "$LEVEL" -ne 2 ] && [ "$LEVEL" -ne 3 ]; then
  echo "Level must be 1, 2 or 3"
  exit 1
fi

# Copies ctf_files files
rm -rf "$FOLDER" || return
cp -r ../user "$FOLDER"

# Deletes files
cd "$FOLDER" || exit
rm dragontime fingerprint DEVICE MAC_AP MAC_USER find_matches.py
rm -r fingerprint-wpa3/build
cd ..

# Prints MAC to file
MAC_AP=$(ip link show "$DEVICE" | grep link/ether | awk '{print $2}')
if [ "$MAC_AP" = "" ]; then
  echo "Can't find MAC of $DEVICE"
  exit 1
fi
echo "$MAC_AP" > MAC_AP
echo "$MAC_AP" > "$FOLDER/MAC_AP"

# Copies ctf_files
cp "ctf_files/$LEVEL/datareader.py" "$FOLDER/datareader.py"
cp "ctf_files/$LEVEL/fingerprint.c" "$FOLDER/finger/fingerprint.c"
cp "ctf_files/$LEVEL/sae.c" "$FOLDER/finger/sae.c"
cp "ctf_files/$LEVEL/dragontime.c" "$FOLDER/attack/dragontime.c"
if [ -e "ctf_files/$LEVEL/find_matches.py" ]; then
  cp "ctf_files/$LEVEL/find_matches.py" "$FOLDER/find_matches.py"
fi
echo "{0215f4f39b38342193918140c4e8c3d4d0d93fa0}" > "$FOLDER/flag"

# Zips user folder
zip -r "$FOLDER.zip" "$FOLDER"
rm -rf "$FOLDER"

# Randomizes password and updates CTFd
cp "$DEF_CONFIG_FILE" "$CURR_CONFIG_FILE"
cp "$FOLDER.zip" "$CTFD_FOLDER/CTFd/uploads/user_files/$FOLDER.zip"
python randomize_password.py "$CURR_CONFIG_FILE" "$DEVICE" "$PASSWORDS_FILE" "$FOLDER.zip"
