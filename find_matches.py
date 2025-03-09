import sys
from scipy import spatial
import csv

import datareader as dr


def create_tree(fp):
    fp_reader = csv.reader(fp, delimiter=",")
    tree_input = []
    passwords = []
    for row in fp_reader:
        tree_input.append([int(item) for item in row[1:21]])
        passwords.append(row[0])
    return passwords, spatial.KDTree(tree_input)


if __name__ == "__main__":
    k = 5
    if len(sys.argv) < 3:
        exit(1)
    if len(sys.argv) == 4:
        k = int(sys.argv[3])

    with open(sys.argv[2], "r") as out:
        passwords, tree = create_tree(out)

    fingerprint = dr.iterations(dr.extract_data(sys.argv[1])).to_list()
    print("Simulated fingerprint: " + ", ".join([str(item) for item in fingerprint]))
    print()

    print("Best matches (lower score is better):")
    matches = tree.query(fingerprint, k=k)
    if k == 1:
        print("  " + passwords[matches[1]] + "   (score: " + str(matches[0]) + ")")
    else:
        for i in range(k):
            print(
                "  "
                + passwords[matches[1][i]]
                + "   (score: "
                + str(matches[0][i])
                + ")"
            )
