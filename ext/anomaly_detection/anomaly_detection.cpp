#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>

#include "anomaly_detection.hpp"
#include "cdflib.hpp"
#include "stl.hpp"

namespace anomaly_detection {

float median(const std::vector<float>& sorted) {
    return (sorted[(sorted.size() - 1) / 2] + sorted[sorted.size() / 2]) / 2.0;
}

float mad(const std::vector<float>& data, float med) {
    std::vector<float> res;
    res.reserve(data.size());
    for (auto v : data) {
        res.push_back(fabs(v - med));
    }
    std::sort(res.begin(), res.end());
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

std::vector<size_t> detect_anoms(const std::vector<float>& data, int num_obs_per_period, float k, float alpha, bool one_tail, bool upper_tail, bool verbose, std::function<void()> interrupt) {
    auto n = data.size();

    // Check to make sure we have at least two periods worth of data for anomaly context
    if (n < num_obs_per_period * 2) {
        throw std::invalid_argument("series must contain at least 2 periods");
    }

    // Handle NANs
    auto nan = std::count_if(data.begin(), data.end(), [](const auto& value) { return std::isnan(value); });
    if (nan > 0) {
        throw std::invalid_argument("series contains NANs");
    }

    // Decompose data. This returns a univarite remainder which will be used for anomaly detection. Optionally, we might NOT decompose.
    auto seasonal_length = n * 10 + 1;
    auto data_decomp = stl::params().robust(true).seasonal_length(seasonal_length).fit(data, num_obs_per_period);

    auto seasonal = data_decomp.seasonal;
    auto med = median(data);
    std::vector<float> data2;
    data2.reserve(n);
    for (auto i = 0; i < n; i++) {
        data2.push_back(data[i] - seasonal[i] - med);
    }

    std::vector<size_t> r_idx;
    auto num_anoms = 0;
    auto max_outliers = (size_t) n * k;

    // Sort data for fast median
    std::vector<size_t> indexes(n);
    std::iota(indexes.begin(), indexes.end(), 0);
    std::stable_sort(indexes.begin(), indexes.end(), [&data2](size_t a, size_t b) { return data2[a] < data2[b]; });
    std::sort(data2.begin(), data2.end());

    // Compute test statistic until r=max_outliers values have been removed from the sample
    for (auto i = 1; i <= max_outliers; i++) {
        // Check for interrupts
        interrupt();

        if (verbose) {
            std::cout << i << " / " << max_outliers << " completed" << std::endl;
        }

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
        auto data_sigma = mad(data2, ma);
        if (data_sigma == 0.0) {
            break;
        }

        auto iter = std::max_element(ares.begin(), ares.end());
        auto r_idx_i = std::distance(ares.begin(), iter);

        // Only need to take sigma of r for performance
        auto r = ares[r_idx_i] / data_sigma;

        r_idx.push_back(indexes[r_idx_i]);
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
            num_anoms = i;
        }
    }

    std::vector<size_t> anomalies(r_idx.begin(), r_idx.begin() + num_anoms);

    // Sort like R version
    std::sort(anomalies.begin(), anomalies.end());

    return anomalies;
}

std::vector<size_t> anomalies(const std::vector<float>& x, int period, float k, float alpha, Direction direction, bool verbose, std::function<void()> interrupt) {
    bool one_tail = direction != Direction::Both;
    bool upper_tail = direction == Direction::Positive;
    return detect_anoms(x, period, k, alpha, one_tail, upper_tail, verbose, interrupt);
}

}
