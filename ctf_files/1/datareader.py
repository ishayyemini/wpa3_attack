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

    ##################
    # YOUR CODE HERE #
    ##################

    pass


def get_address_quantiles(df: pd.DataFrame, low=0.15, high=0.35) -> pd.DataFrame:
    """
    Creates a pandas DataFrame containing, for each spoofed address:
    the address byte, the (low)-quantile, and (high)-quantile.
    """

    ##################
    # YOUR CODE HERE #
    ##################

    pass


def estimate_iter_time(qsums) -> int:
    """
    Calculates an estimation for the time it takes to preform one iteration, assuming min_iter is
    equal to the real number of iterations for i <= 3.
    """

    ##################
    # YOUR CODE HERE #
    ##################

    pass


def min_iterations(df: pd.DataFrame, low=0.15, high=0.35) -> pd.DataFrame:
    """
    Groups the measurements to find which ones have the same iteration count, and sorts them to get
    a lower bound on the real iteration count
    """

    addr_quantiles = get_address_quantiles(df, low, high)

    # Adds a new column for the lower bound calculated here
    addr_quantiles["min_iters"] = 0

    ##################
    # YOUR CODE HERE #
    ##################

    pass


def iterations(df: pd.DataFrame, low=0.15, high=0.35) -> pd.DataFrame:
    """
    Calculates an estimation for the number of iterations the server preformed for each spoofed address.
    Returns DataFrame list of iterations for each address.
    """

    addr_quantiles = min_iterations(df, low, high)

    # A new column for estimated iteration count
    addr_quantiles["est_iters"] = 0

    ##################
    # YOUR CODE HERE #
    ##################

    # Pairwise max
    return addr_quantiles["min_iters"].clip(lower=addr_quantiles["est_iters"])
