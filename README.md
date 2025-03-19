## Admin

Please ```cd``` into the admin folder.

Before starting, run:

```
sh setup_admin.sh
```

After the initial setup, you usually don't need to run the setup again.

To start the CTFd run:

```
sh run_ctfd.sh 1 wlan0
```

You can replace "1" with your desired level (1, 2 or 3) and "wlan0" with the Wi-Fi adapter of your choice.

After running run_ctfd.sh, you can start the AP by running:

```
sh run_hostapd.sh
```

## Player

Before starting, run:

```
sh setup_user.sh wlan0
```

You can replace "wlan0" with the relevant Wi-Fi adapter.

To start taking measurements, run:

```
sudo ./dragontime -d $(<DEVICE) -a $(<MAC_AP) -o measurements/PASSWORD.txt
```

You can replace "PASSWORD" with your chosen filename.
After getting 50-100 measurements, stop dragontime with CTRL+C.

To generate fingerprints for passwords, run:

```
./fingerprint passwords.txt out.csv
```

The fingerprints will be saved to out.csv.

After getting measurements and fingerprints, run:

```
python find_matches.py measurements/PASSWORD.txt out.csv
```
