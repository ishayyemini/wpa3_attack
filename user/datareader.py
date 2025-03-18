import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
from sklearn.cluster import AgglomerativeClustering as agc


# Converts measurements to pandas dataframe
def extract_data(path) -> pd.DataFrame:
    file = open(path)
    data = file.readlines()
    file.close()

    # Cut irrelevant lines
    data = data[6:-1]

    data = [str.split(line, " ") for line in data]
    data = [(int(line[1][:-1], 16), int(line[2][:-1], 10)) for line in data]
    return pd.DataFrame(data, columns=["STA", "Time"])


def get_address_quantiles(df, addrs=20, low=0.3, high=0.5):
    addr_quantiles = pd.DataFrame(  # min_iterations3() doesn't use low,high but iterations() does so they are added here
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


def min_iterations(df, addrs=20, low=0.3, high=0.5):
    """retuns a DataFrame list of minimum iterations preformed for each address using Crosby's box test. this is a lower bound on the true number"""

    addr_quantiles = get_address_quantiles(df, addrs, low, high)

    # adding a new column for the lower bound
    addr_quantiles["min_iters"] = 0

    # addr_quantiles collums: address, low quantile, high quantile, min_iterations

    i = 1
    while not addr_quantiles["min_iters"].all() and i < 256:
        idmin = addr_quantiles["Low Quantile"][
            addr_quantiles["min_iters"] == 0
        ].idxmin()  # get the index with the smallest quantile to not be selected

        # get the indices of addresses that are in the range of our box
        rows_in_box = np.logical_and(
            addr_quantiles["min_iters"] == 0,
            addr_quantiles["Low Quantile"] <= addr_quantiles["High Quantile"][idmin],
        )
        addr_quantiles.loc[rows_in_box, "min_iters"] = (
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


def iterations(df, addrs=20, low=0.6, high=0.8):
    """returns a DataFrame list of iterations for each address, assuming there are addresses with 1,2,3 iterations
    version {1,2,3} decides which min_iterations is used
    distance_threshold is only used in version 3"""

    addr_quantiles = min_iterations(df, addrs, low, high)

    # A new column for estimated iteration count
    addr_quantiles["est_iters"] = 0

    qsums = [
        (
            (addr_quantiles["min_iters"] == j).sum(),
            addr_quantiles["Low Quantile"][addr_quantiles["min_iters"] == j].sum(),
            addr_quantiles["High Quantile"][addr_quantiles["min_iters"] == j].sum(),
        )
        for j in (1, 2, 3)
    ]

    # estimated time for 1 iteration
    itrtime = estimate_iter_time(qsums)

    maximal_min_iter = addr_quantiles["min_iters"].max()

    # average time of 1 iteration execution within the quantile range
    time_1 = (qsums[0][1] + qsums[0][2]) / (2 * qsums[0][0])

    for i in range(4, maximal_min_iter + 1):
        sum_low_quantile = addr_quantiles["Low Quantile"][
            addr_quantiles["min_iters"] == i
        ].sum()
        sum_high_quantile = addr_quantiles["High Quantile"][
            addr_quantiles["min_iters"] == i
        ].sum()
        samples_amount = (addr_quantiles["min_iters"] == i).sum()

        #  average time of i iterations execution within the quantile range
        time_i = (sum_low_quantile + sum_high_quantile) / (2 * samples_amount)

        # number of iterations estimate
        # adding 1 because (time_i - time_1) is the time of i-1 iterations
        addr_quantiles.loc[addr_quantiles["min_iters"] == i, "est_iters"] = 1 + round(
            (time_i - time_1) / itrtime
        )

    return addr_quantiles["min_iters"].clip(
        lower=addr_quantiles["est_iters"]
    )  # pairwise max


def min_iterations2(df, addrs=20, low=0.3, high=0.5):
    """retuns a DataFrame list of minimum iterations preformed for each address using Crosby's box test. this is a lower bound on the true number.
    this version assumes no overlap between the boxes of different iteration addresses, works better on data where all the addresess are further apart,
    but worse on data where addresses with different iterations are closer"""
    addr_quantiles = get_address_quantiles(df, addrs, low, high)

    # adding a new column for the lower bound
    addr_quantiles["min_iters"] = 0

    # addr_quantiles collums: address, low quantile, high quantile, min_iterations

    i = 1
    while not addr_quantiles["min_iters"].all() and i < 256:
        idmin = addr_quantiles["Low Quantile"][
            addr_quantiles["min_iters"] == 0
        ].idxmin()  # get the index with the smallest quantile to not be selected
        # get the indices of addresses that are in the range of our box
        rows_in_box = np.logical_and(
            addr_quantiles["min_iters"] == 0,
            addr_quantiles["Low Quantile"] <= addr_quantiles["High Quantile"][idmin],
        )
        while (
            rows_in_box.sum() > 0
        ):  # continues until no more quantiles intersect with the current group of addresses
            addr_quantiles.loc[rows_in_box, "min_iters"] = (
                i  # assign min iterations to all intersecting quantiles
            )
            max_quantile = addr_quantiles["High Quantile"][
                rows_in_box
            ].max()  # maximum quantile in the current group
            # get the indices of addresses that are in the range of our box
            rows_in_box = np.logical_and(
                addr_quantiles["min_iters"] == 0,
                addr_quantiles["Low Quantile"] <= max_quantile,
            )
        i += 1
    return addr_quantiles


def min_iterations3(df, addrs=20, distance_threshold=10000):
    """retuns a DataFrame list of minimum iterations preformed for each address using Agglomerative Clustering. this is a lower bound on the true number
    The algorithm starts with every point as a cluster, and recursively merges clusters in a way that minimizes the variance within clusters until all the clusters are further apart than the distance threshold
    """

    quantiles = [
        [df["Time"][df["STA"] == j].quantile(i / 50) for i in range(10, 40)]
        for j in range(addrs)
    ]  # vectors of quntiles, without the outlier edges
    labels = agc(n_clusters=None, distance_threshold=distance_threshold).fit_predict(
        quantiles
    )  # returns array of labels for each observation
    labels = pd.DataFrame(
        [
            (labels[j], sum(quantiles[j]) / 30)  # average time for each address
            for j in range(addrs)
        ],
        columns=["Label", "Average"],
    )
    labels["Average"] = [
        labels["Average"][labels["Label"] == labels["Label"][j]].mean()
        for j in range(addrs)
    ]  # average times for every cluster
    sorted_avg = np.sort(
        labels["Average"].unique()
    )  # sorted to estimate minimum iteration count
    labels["Label"] = [
        np.argwhere(sorted_avg == avg)[0][0] + 1 for avg in labels["Average"]
    ]  # replacing labels with the order of averages
    return labels["Label"]
