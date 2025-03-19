import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
from sklearn.cluster import AgglomerativeClustering as agc


def extract_data(path: str) -> pd.DataFrame:
    """
    Converts measurements to pandas dataframe with one column "STA" which has the spoofed address byte
    and a column "Time" specifying the elapsed time for this measurement.
    """

    file = open(path)
    data = file.readlines()
    file.close()

    # Cut irrelevant lines
    data = data[6:-1]

    data = [str.split(line, " ") for line in data]
    data = [(int(line[1][:-1], 16), int(line[2][:-1], 10)) for line in data]
    return pd.DataFrame(data, columns=["STA", "Time"])


def qplot(df, addrs=20):
    for j in range(addrs):
        tmp = plt.plot(
            range(0, 100, 2),
            [df["Time"][df["STA"] == j].quantile(i / 50) for i in range(50)],
            label=j,
        )
    # if you care about which address matches which graph
    # plt.legend() 
    plt.show()
    

def get_address_quantiles(df, addrs=20, low=0.3, high=0.5):
    addr_quantiles = pd.DataFrame(
        [
            (
                j,
                df["Time"][df["STA"] == j].quantile(low),
                df["Time"][df["STA"] == j].quantile(high),
            )
            for j in range(addrs)
        ],
        columns=["STA", "Low Quantile", "High Quantile"],
    )
    return addr_quantiles


def estimate_iter_time(addr_quantiles) -> int:
    """
    Calculates an estimation for the time it takes to preform one iteration, assuming min_iter is
    equal to the real number of iterations for i <= 3.
    """

    qsums = [
        (
            (addr_quantiles["min_iters"] == j).sum(),
            addr_quantiles["Low Quantile"][addr_quantiles["min_iters"] == j].sum(),
            addr_quantiles["High Quantile"][addr_quantiles["min_iters"] == j].sum(),
        )
        for j in (1, 2, 3)
    ]

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


def min_iterations(df: pd.DataFrame, low=0.15, high=0.35) -> pd.DataFrame:
    """
    Groups the measurements to find which ones have the same iteration count, and sorts them to get
    a lower bound on the real iteration count
    """

    addr_quantiles = get_address_quantiles(df, low, high)

    # Adds a new column for the lower bound calculated here
    addr_quantiles["min_iters"] = 0

    i = 1
    # while there are addresses we didnt assign min_iters yet
    while not addr_quantiles["min_iters"].all() and i < 256:
        # Find the address with the smallest value in "Low Quantile"
        # out of the ones we haven't assigned "min_iters" yet

        ##################
        # YOUR CODE HERE #
        ##################
        

        # get the indices of addresses that have overlapping intervals with the address you found

        ##################
        # YOUR CODE HERE #
        ##################

        # assign i to their "min_iters value". thats their group id and also a lower bound on the number
        # of iterations for that address!

        i += 1


    pass


def iterations(df: pd.DataFrame, low=0.15, high=0.35) -> pd.DataFrame:
    """
    Calculates an estimation for the number of iterations the server preformed for each spoofed address.
    Returns DataFrame list of iterations for each address.
    """

    addr_quantiles = min_iterations(df, low, high)

    # A new column for estimated iteration count
    addr_quantiles["est_iters"] = 0

    # estimated time for 1 iteration
    itrtime = estimate_iter_time(addr_quantiles)

    group_1 = addr_quantiles["min_iters" == 1]

    # average time of 1 iteration execution within the quantile range
    time_1 = -1 # TODO

    maximal_min_iter = addr_quantiles["min_iters"].max()

    for i in range(4, maximal_min_iter + 1):

        group_i = addr_quantiles["min_iters" == i]
        

        #  average time of group i's execution within the quantile range
        time_i = -1 # TODO

        # estimate how many iterations were executed for group i using time_i, time_1 and itrtime

        ##################
        # YOUR CODE HERE #
        ##################


    # Pairwise max
    return addr_quantiles["min_iters"].clip(lower=addr_quantiles["est_iters"])
