import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
from sklearn.cluster import AgglomerativeClustering as agc


# Converts measurements to pandas dataframe with one column "STA" which has the spoofed adress byte
# and a column "Time" specifying the elapsed time for this measurment
def extract_data(path):
    file = open(path)
    
    ##################
    # YOUR CODE HERE #
    ##################

    return df



# create a pandas dataframe containing for each spoofed address:
# the adress byte, the (low)-quantile, and (high)-quantile
def get_address_quantiles(df, addrs=20, low=0.15, high=0.35):
    
    ##################
    # YOUR CODE HERE #
    ##################

    return addr_quantiles

# group the measurments to find which ones have the same iteration count, and sort them to get
# a lower bound on the real iteration count
def min_iterations(df, addrs=20, low=0.15, high=0.35):

    addr_quantiles = get_address_quantiles(df, addrs, low, high)

    # adding a new column for the lower bound calculated here
    addr_quantiles["min_iters"] = 0

    ##################
    # YOUR CODE HERE #
    ##################

    return

# calculate an estimation for the time it takes to preform one iteration, assuming min_iter is
# equal to the real number of iterations for i <= 3
def estimate_iter_time(addr_quantiles):

    iter_time = -1

    ##################
    # YOUR CODE HERE #
    ##################
    
    return iter_time


# calculate an estimation for the number of iterations the server preformed for each spoofed address
def iterations(df, addrs=20, low=0.15, high=0.35):
    """returns a DataFrame list of iterations for each address, assuming there are addresses with 1,2,3 iterations
    version {1,2,3} decides which min_iterations is used
    distance_threshold is only used in version 3"""
    
    addr_quantiles = min_iterations(df, addrs, low, high)

    # estimated time for 1 iteration
    itrtime = estimate_iter_time(addr_quantiles)

    # A new column for estimated iteration count
    addr_quantiles["est_iters"] = 0

    
    ##################
    # YOUR CODE HERE #
    ##################
    

    # pairwise max
    return addr_quantiles["min_iters"].clip(lower=addr_quantiles["est_iters"])  
   