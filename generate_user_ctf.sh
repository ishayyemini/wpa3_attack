#!/bin/sh

cp -r user user_ctf

cd user_ctf || exit
rm dragontime fingerprint DEVICE MAC_AP MAC_USER find_matches.py
rm -r fingerprint-wpa3/build
cd ..

cp ctf_files/sae.c user_ctf/fingerprint-wpa3/src/common/sae.c
echo "{0215f4f39b38342193918140c4e8c3d4d0d93fa0}" > user_ctf/flag

zip -r user_ctf.zip user_ctf
rm -rf user_ctf
