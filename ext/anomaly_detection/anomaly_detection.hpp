/*!
 * AnomalyDetection.cpp v0.1.0
 * https://github.com/ankane/AnomalyDetection.cpp
 * GPL-3.0-or-later License
 */

#pragma once

#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <vector>

#include "dist.h"
#include "stl.hpp"

namespace anomaly_detection {

enum Direction { Positive, Negative, Both };

float median_sorted(const std::vector<float>& sorted) {
    return (sorted[(sorted.size() - 1) / 2] + sorted[sorted.size() / 2]) / 2.0;
}

float median(const std::vector<float>& data) {
    std::vector<float> sorted(data);
    std::sort(sorted.begin(), sorted.end());
    return median_sorted(sorted);
}

float mad(const std::vector<float>& data, float med) {
    std::vector<float> res;
    res.reserve(data.size());
    for (auto v : data) {
        res.push_back(fabs(v - med));
    }
    std::sort(res.begin(), res.end());
    return 1.4826 * median_sorted(res);
}

std::vector<size_t> detect_anoms(const std::vector<float>& data, int num_obs_per_period, float k, float alpha, bool one_tail, bool upper_tail, bool verbose, std::function<void()> callback) {
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
    auto data_decomp = stl::params().robust(true).seasonal_length(data.size() * 10 + 1).fit(data, num_obs_per_period);
    auto seasonal = data_decomp.seasonal;

    std::vector<float> data2;
    data2.reserve(n);
    auto med = median(data);
    for (auto i = 0; i < n; i++) {
        data2.push_back(data[i] - seasonal[i] - med);
    }

    auto num_anoms = 0;
    auto max_outliers = (size_t) n * k;
    std::vector<size_t> anomalies;
    anomalies.reserve(max_outliers);

    // Sort data for fast median
    // Use stable sort for indexes for deterministic results
    std::vector<size_t> indexes(n);
    std::iota(indexes.begin(), indexes.end(), 0);
    std::stable_sort(indexes.begin(), indexes.end(), [&data2](size_t a, size_t b) { return data2[a] < data2[b]; });
    std::sort(data2.begin(), data2.end());

    // Compute test statistic until r=max_outliers values have been removed from the sample
    for (auto i = 1; i <= max_outliers; i++) {
        if (verbose) {
            std::cout << i << " / " << max_outliers << " completed" << std::endl;
        }

        // TODO Improve performance between loop iterations
        auto ma = median_sorted(data2);
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

        anomalies.push_back(indexes[r_idx_i]);
        data2.erase(data2.begin() + r_idx_i);
        indexes.erase(indexes.begin() + r_idx_i);

        // Compute critical value
        float p;
        if (one_tail) {
            p = 1.0 - alpha / (n - i + 1);
        } else {
            p = 1.0 - alpha / (2.0 * (n - i + 1));
        }

        auto t = students_t_ppf(p, n - i - 1);
        auto lam = t * (n - i) / sqrt(((n - i - 1) + t * t) * (n - i + 1));

        if (r > lam) {
            num_anoms = i;
        }

        if (callback != nullptr) {
            callback();
        }
    }

    anomalies.resize(num_anoms);

    // Sort like R version
    std::sort(anomalies.begin(), anomalies.end());

    return anomalies;
}

class AnomalyDetectionResult {
public:
    std::vector<size_t> anomalies;
};

class AnomalyDetectionParams {
    float alpha_ = 0.05;
    float max_anoms_ = 0.1;
    Direction direction_ = Direction::Both;
    bool verbose_ = false;
    std::function<void()> callback_ = nullptr;

public:
    inline AnomalyDetectionParams alpha(float alpha) {
        this->alpha_ = alpha;
        return *this;
    };

    inline AnomalyDetectionParams max_anoms(float max_anoms) {
        this->max_anoms_ = max_anoms;
        return *this;
    };

    inline AnomalyDetectionParams direction(Direction direction) {
        this->direction_ = direction;
        return *this;
    };

    inline AnomalyDetectionParams verbose(bool verbose) {
        this->verbose_ = verbose;
        return *this;
    };

    inline AnomalyDetectionParams callback(std::function<void()> callback) {
        this->callback_ = callback;
        return *this;
    };

    AnomalyDetectionResult fit(const std::vector<float>& series, size_t period);
};

AnomalyDetectionParams params() {
    return AnomalyDetectionParams();
}

AnomalyDetectionResult AnomalyDetectionParams::fit(const std::vector<float>& series, size_t period) {
    bool one_tail = this->direction_ != Direction::Both;
    bool upper_tail = this->direction_ == Direction::Positive;

    auto res = AnomalyDetectionResult();
    res.anomalies = detect_anoms(series, period, this->max_anoms_, this->alpha_, one_tail, upper_tail, this->verbose_, this->callback_);
    return res;
}

}
