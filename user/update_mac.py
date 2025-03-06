import re
import sys
import fileinput


def check_mac(mac):
    mac_arr = mac.split(":")
    return len(mac_arr) == 6 and all(len(i) == 2 for i in mac_arr)


def replace_mac(original_line, mac):
    parsed_mac = [str(int(i, 16)) for i in mac.split(":")]
    return re.sub(r"{.+}", "{" + ", ".join(parsed_mac) + "}", original_line)


if __name__ == "__main__":
    if len(sys.argv) < 4:
        exit(1)

    fingerprint_file = sys.argv[1]
    mac1 = sys.argv[2]
    mac2 = sys.argv[3]

    if (not check_mac(mac1)) or not check_mac(mac2):
        exit(1)

    for line in fileinput.input(fingerprint_file, inplace=True):
        if "addr1[6] =" in line:
            flag = 1
            print(replace_mac(line, mac1), end="")
        elif "addr2[6] =" in line:
            print(replace_mac(line, mac2), end="")
        else:
            print(line, end="")
