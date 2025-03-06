import secrets
import sys
import fileinput

if __name__ == "__main__":
    if len(sys.argv) < 4:
        exit(1)

    config_file = sys.argv[1]
    device = sys.argv[2]
    passwords_file = sys.argv[3]

    with open(passwords_file, "r") as passwords:
        chosen = secrets.choice(passwords.readlines()).strip()

    for line in fileinput.input(config_file, inplace=True):
        if "interface=" in line:
            print("interface={}".format(device))
        elif "wpa_passphrase=" in line:
            print("wpa_passphrase={}".format(chosen))
        else:
            print(line, end="")
