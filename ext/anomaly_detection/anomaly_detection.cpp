#include <cassert>
#include <iostream>
#include <iterator>
#include <vector>

#include "cdflib.hpp"
#include "stl.hpp"

float median(const std::vector<float>& data) {
    std::vector<float> sorted(data);
    std::sort(sorted.begin(), sorted.end());
    return (sorted[(sorted.size() - 1) / 2] + sorted[sorted.size() / 2]) / 2.0;
}

float mad(const std::vector<float>& data) {
    auto med = median(data);
    std::vector<float> res;
    res.reserve(data.size());
    for (auto v : data) {
        res.push_back(fabs(v - med));
    }
    return 1.4826 * median(res);
}

float qt(double p, double df) {
    int which = 2;
    double q = 1 - p;
    double t;
    int status;
    double bound;
    cdft(&which, &p, &q, &t, &df, &status, &bound);

    if (status != 0) {
        throw std::invalid_argument("Bad status");
    }
    return t;
}

std::vector<size_t> detect_anoms(const std::vector<float>& data, int num_obs_per_period, float k, float alpha, bool one_tail, bool upper_tail) {
    auto num_obs = data.size();

    // Check to make sure we have at least two periods worth of data for anomaly context
    assert(num_obs >= num_obs_per_period * 2);

    // Handle NANs
    auto nan = std::count_if(data.begin(), data.end(), [](const auto& value) { return std::isnan(value); });
    if (nan > 0) {
        throw std::invalid_argument("Data contains NANs");
    }

    // Decompose data. This returns a univarite remainder which will be used for anomaly detection. Optionally, we might NOT decompose.
    auto seasonal_length = data.size() * 10 + 1;
    auto data_decomp = stl::params().robust(true).seasonal_length(seasonal_length).fit(data, num_obs_per_period);

    auto seasonal = data_decomp.seasonal;
    auto med = median(data);
    std::vector<float> data2;
    data2.reserve(data.size());
    for (auto i = 0; i < data.size(); i++) {
        data2.push_back(data[i] - seasonal[i] - med);
    }

    auto max_outliers = (size_t) num_obs * k;
    assert(max_outliers > 0);

    auto n = data2.size();

    std::vector<size_t> r_idx;

    std::vector<size_t> indexes;
    indexes.reserve(data2.size());
    for (auto i = 0; i < data2.size(); i++) {
        indexes.push_back(i);
    }

    // Compute test statistic until r=max_outliers values have been removed from the sample
    for (auto i = 1; i <= max_outliers; i++) {
        // TODO Improve performance between loop iterations
        auto ma = median(data2);
        std::vector<float> ares;
        ares.reserve(data2.size());
        if (one_tail) {
            if (upper_tail) {
                for (auto v : data2) {
                    ares.push_back(v - ma);
                }
            } else {
                for (auto v : data2) {
                    ares.push_back(ma - v);
                }
            }
        } else {
            for (auto v : data2) {
                ares.push_back(fabs(v - ma));
            }
        }

        // Protect against constant time series
        auto data_sigma = mad(data2);
        if (data_sigma == 0.0) {
            break;
        }

        auto iter = std::max_element(ares.begin(), ares.end());
        auto r_idx_i = std::distance(ares.begin(), iter);
        auto r_idx_i2 = indexes[r_idx_i];

        // Only need to take sigma of r for performance
        auto r = ares[r_idx_i] / data_sigma;

        // TODO Swap to last position and delete
        data2.erase(data2.begin() + r_idx_i);
        indexes.erase(indexes.begin() + r_idx_i);

        // Compute critical value
        float p;
        if (one_tail) {
            p = 1.0 - alpha / (n - i + 1);
        } else {
            p = 1.0 - alpha / (2.0 * (n - i + 1));
        }

        auto t = qt(p, n - i - 1);
        auto lam = t * (n - i) / sqrt(((n - i - 1) + powf(t, 2.0)) * (n - i + 1));

        if (r > lam) {
            r_idx.push_back(r_idx_i2);
        } else {
            break;
        }
    }

    // Sort like R version
    std::sort(r_idx.begin(), r_idx.end());

    return r_idx;
}

std::vector<size_t> anomalies(const std::vector<float>& x, int period, float k, float alpha, const std::string& direction) {
    bool one_tail;
    bool upper_tail;
    if (direction == "pos") {
        one_tail = true;
        upper_tail = true;
    } else if (direction == "neg") {
        one_tail = true;
        upper_tail = false;
    } else if (direction == "both") {
        one_tail = false;
        upper_tail = true; // not used
    } else {
        throw std::invalid_argument("Bad direction");
    }

    return detect_anoms(x, period, k, alpha, one_tail, upper_tail);
}
