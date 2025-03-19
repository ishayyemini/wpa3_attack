#!/bin/sh

LEVEL="${1:-1}"
FOLDER="user_ctf_$LEVEL"

cp -r user "$FOLDER"

cd "$FOLDER" || exit
rm dragontime fingerprint DEVICE MAC_AP MAC_USER find_matches.py
rm -r fingerprint-wpa3/build
cd ..

cp "ctf_files/$LEVEL/datareader.py" "$FOLDER/datareader.py"
cp "ctf_files/$LEVEL/fingerprint.c" "$FOLDER/finger/fingerprint.c"
cp "ctf_files/$LEVEL/sae.c" "$FOLDER/finger/sae.c"
cp "ctf_files/$LEVEL/dragontime.c" "$FOLDER/attack/dragontime.c"
if [ -e "ctf_files/$LEVEL/find_matches.py" ]; then
  cp "ctf_files/$LEVEL/find_matches.py" "$FOLDER/find_matches.py"
fi
echo "{0215f4f39b38342193918140c4e8c3d4d0d93fa0}" > "$FOLDER/flag"

zip -r "$FOLDER.zip" "$FOLDER"
rm -rf "$FOLDER"
