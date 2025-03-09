import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

# converts measurments to pandas dataframe
def extract_data(path):
    file = open(path)
    data = file.readlines()
    file.close()

    # Cut irrelevant lines
    data = data[6:-1]

    data = [str.split(line, " ") for line in data]
    data = [(int(line[1][:-1], 16), int(line[2][:-1], 10)) for line in data]
    return pd.DataFrame(data)


def qplot(df, addrs=20):
    for j in range(addrs):
        tmp = plt.plot(
            range(50), [df[1][df[0] == j].quantile(i / 50) for i in range(50)], label=j
        )
    # plt.legend() if you care about which address matches which graph
    # plt.show() if you don't wanna add anything else


def min_iterations(df, addrs=20):
    """retuns a DataFrame list of minimum iterations for each address using Crosby's box test. this is a lower bound on the true number"""
    addr_quantiles = pd.DataFrame(
        [
            (j, df[1][df[0] == j].quantile(0.15), df[1][df[0] == j].quantile(0.35))
            for j in range(addrs)
        ]
    )

    # adding a new column for the lower bound
    addr_quantiles[3] = 0

    # addr_quantiles collums: address, low quantile, high quantile, min_iterations

    i = 1
    while not addr_quantiles[3].all() and i < 256:
        idmin = addr_quantiles[1][
            addr_quantiles[3] == 0
        ].idxmin()  # get the index with the smallest quantile to not be selected
        addr_quantiles.loc[np.logical_and(addr_quantiles[3] == 0, addr_quantiles[1] <= addr_quantiles[2][idmin]), 3] = (
            i  # assign min iterations to all intersecting quantiles
        )
        i += 1
    return addr_quantiles

def estimate_iter_time(qsums):
    # time(1 iteration)≈E[((quantile of x iterations)-(quantile of x-k iterations))/k]≈sum(all such possible pairs)/#(all such possible pairs)
    num_pairs = qsums[1][0] * (qsums[0][0] + qsums[2][0]) + qsums[0][0] * qsums[2][0]

    itr_time_low_high = [
        (
            qsums[2][j] * (qsums[1][0] + 0.5 * qsums[0][0])
            + qsums[1][j] * (qsums[0][0] - qsums[2][0])
            + qsums[0][j] * (-qsums[1][0] - 0.5 * qsums[2][0])
        )
        / num_pairs
        for j in (1, 2)
    ]
    return sum(itr_time_low_high) / 2


def iterations(df, addrs=20):
    """retuns a DataFrame list of iterations for each address, assuming there are addresses with 1,2,3 iterations"""
   
    addr_quantiles = min_iterations(df)

    qsums = [
        ((addr_quantiles[3] == j).sum(), addr_quantiles[1][addr_quantiles[3] == j].sum(), addr_quantiles[2][addr_quantiles[3] == j].sum())
        for j in (1, 2, 3)
    ]
    print(qsums)

    itrtime = estimate_iter_time(qsums)
    
    maximal_min_iter = addr_quantiles[3].max()

    floor1 = (qsums[0][1] + qsums[0][2]) / (
        2 * qsums[0][0]
    )  # average of average quantile of 1 iteration addresses
    addr_quantiles[4] = 0
    for i in range(4, maximal_min_iter + 1):
        floor = (addr_quantiles[1][addr_quantiles[3] == i].sum() + addr_quantiles[2][addr_quantiles[3] == i].sum()) / (
            2 * (addr_quantiles[3] == i).sum()
        )  # average of average quantile of group i
        addr_quantiles.loc[addr_quantiles[3] == i, 4] = 1 + round(
            (floor - floor1) / itrtime
        )  # number of iterations estimate

    return addr_quantiles[3].clip(lower=addr_quantiles[4])  # pairwise max
