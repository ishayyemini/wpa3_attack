import secrets
import sys
import fileinput
import sqlite3
import hashlib

ctfd_folder = "./CTFd"


def calc_sha1(user_ctf_zip):
    sha1sum = hashlib.sha1()
    with open(user_ctf_zip, 'rb') as source:
        block = source.read(2 ** 16)
        while len(block) != 0:
            sha1sum.update(block)
            block = source.read(2 ** 16)
    return sha1sum.hexdigest()


def run_query(q):
    dbfile = ctfd_folder + "/CTFd/ctfd.db"
    con = sqlite3.connect(dbfile)
    cur = con.cursor()
    cur.execute(q)
    print(cur.fetchall())
    cur.close()
    con.commit()
    con.close()


def update_conf(chosen, config_file, device):
    for line in fileinput.input(config_file, inplace=True):
        if "interface=" in line:
            print("interface={}".format(device))
        elif "wpa_passphrase=" in line:
            print("wpa_passphrase={}".format(chosen))
        else:
            print(line, end="")


if __name__ == "__main__":
    if len(sys.argv) < 5:
        exit(1)

    config_file = sys.argv[1]
    device = sys.argv[2]
    passwords_file = sys.argv[3]
    user_ctf_zip = sys.argv[4]

    # Choose password
    with open(passwords_file, "r") as passwords:
        chosen = secrets.choice(passwords.readlines()).strip()

    # Update hostapd configuration file
    update_conf(chosen, config_file, device)

    # Update password as flag
    run_query(f"UPDATE flags SET content = '{chosen}' WHERE id = 1")

    # Update file in CTFd
    sha1 = calc_sha1(user_ctf_zip)
    location = "user_files/" + user_ctf_zip
    run_query(f"UPDATE files SET location = '{location}', sha1sum = '{sha1}' WHERE id = 2")
