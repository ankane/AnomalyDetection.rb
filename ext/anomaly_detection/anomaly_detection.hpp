/*!
 * AnomalyDetection.cpp v0.2.1
 * https://github.com/ankane/AnomalyDetection.cpp
 * GPL-3.0-or-later License
 */

#pragma once

#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <vector>

#if __cplusplus >= 202002L
#include <span>
#endif

#include "dist.h"
#include "stl.hpp"

namespace anomaly_detection {

/// The direction to detect anomalies.
enum class Direction {
    /// Positive direction.
    Positive,
    /// Negative direction.
    Negative,
    /// Both directions.
    Both
};

namespace {

template<typename T>
T median_sorted(const std::vector<T>& sorted) {
    return (sorted[(sorted.size() - 1) / 2] + sorted[sorted.size() / 2]) / 2.0;
}

template<typename T>
T median(const T* data, size_t data_size) {
    std::vector<T> sorted(data, data + data_size);
    std::sort(sorted.begin(), sorted.end());
    return median_sorted(sorted);
}

template<typename T>
T mad(const std::vector<T>& data, T med) {
    std::vector<T> res;
    res.reserve(data.size());
    for (auto v : data) {
        res.push_back(std::abs(v - med));
    }
    std::sort(res.begin(), res.end());
    return 1.4826 * median_sorted(res);
}

template<typename T>
std::vector<size_t> detect_anoms(const T* data, size_t data_size, size_t num_obs_per_period, float k, float alpha, bool one_tail, bool upper_tail, bool verbose, std::function<void()> callback) {
    auto n = data_size;

    // Check to make sure we have at least two periods worth of data for anomaly context
    if (n < num_obs_per_period * 2) {
        throw std::invalid_argument("series must contain at least 2 periods");
    }

    // Handle NANs
    auto nan = std::count_if(data, data + data_size, [](const auto& value) {
        return std::isnan(value);
    });
    if (nan > 0) {
        throw std::invalid_argument("series contains NANs");
    }

    std::vector<T> data2;
    data2.reserve(n);
    auto med = median(data, data_size);

    if (num_obs_per_period > 1) {
        // Decompose data. This returns a univarite remainder which will be used for anomaly detection. Optionally, we might NOT decompose.
        auto data_decomp = stl::params().robust(true).seasonal_length(data_size * 10 + 1).fit(data, data_size, num_obs_per_period);
        auto seasonal = data_decomp.seasonal;

        for (size_t i = 0; i < n; i++) {
            data2.push_back(data[i] - seasonal[i] - med);
        }
    } else {
        for (size_t i = 0; i < n; i++) {
            data2.push_back(data[i] - med);
        }
    }

    auto num_anoms = 0;
    auto max_outliers = (size_t) n * k;
    std::vector<size_t> anomalies;
    anomalies.reserve(max_outliers);

    // Sort data for fast median
    // Use stable sort for indexes for deterministic results
    std::vector<size_t> indexes(n);
    std::iota(indexes.begin(), indexes.end(), 0);
    std::stable_sort(indexes.begin(), indexes.end(), [&data2](size_t a, size_t b) {
        return data2[a] < data2[b];
    });
    std::sort(data2.begin(), data2.end());

    // Compute test statistic until r=max_outliers values have been removed from the sample
    for (auto i = 1; i <= max_outliers; i++) {
        if (verbose) {
            std::cout << i << " / " << max_outliers << " completed" << std::endl;
        }

        // TODO Improve performance between loop iterations
        auto ma = median_sorted(data2);
        std::vector<T> ares;
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
                ares.push_back(std::abs(v - ma));
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
        double p;
        if (one_tail) {
            p = 1.0 - alpha / (n - i + 1);
        } else {
            p = 1.0 - alpha / (2.0 * (n - i + 1));
        }

        auto t = students_t_ppf(p, n - i - 1);
        auto lam = t * (n - i) / std::sqrt(((n - i - 1) + t * t) * (n - i + 1));

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

}

/// An anomaly detection result.
class AnomalyDetectionResult {
public:
    /// Returns the anomalies.
    std::vector<size_t> anomalies;
};

/// A set of anomaly detection parameters.
class AnomalyDetectionParams {
    float alpha_ = 0.05;
    float max_anoms_ = 0.1;
    Direction direction_ = Direction::Both;
    bool verbose_ = false;
    std::function<void()> callback_ = nullptr;

public:
    /// Sets the level of statistical significance.
    inline AnomalyDetectionParams alpha(float alpha) {
        this->alpha_ = alpha;
        return *this;
    };

    /// Sets the maximum number of anomalies as percent of data.
    inline AnomalyDetectionParams max_anoms(float max_anoms) {
        this->max_anoms_ = max_anoms;
        return *this;
    };

    /// Sets the direction.
    inline AnomalyDetectionParams direction(Direction direction) {
        this->direction_ = direction;
        return *this;
    };

    /// Sets whether to show progress.
    inline AnomalyDetectionParams verbose(bool verbose) {
        this->verbose_ = verbose;
        return *this;
    };

    /// Sets a callback for each iteration.
    inline AnomalyDetectionParams callback(std::function<void()> callback) {
        this->callback_ = callback;
        return *this;
    };

    /// Detects anomalies in a time series from an array.
    template<typename T>
    inline AnomalyDetectionResult fit(const T* series, size_t series_size, size_t period) const {
        bool one_tail = this->direction_ != Direction::Both;
        bool upper_tail = this->direction_ == Direction::Positive;

        auto anomalies = detect_anoms(series, series_size, period, this->max_anoms_, this->alpha_, one_tail, upper_tail, this->verbose_, this->callback_);
        return AnomalyDetectionResult { anomalies };
    }

    /// Detects anomalies in a time series from a vector.
    template<typename T>
    inline AnomalyDetectionResult fit(const std::vector<T>& series, size_t period) const {
        return fit(series.data(), series.size(), period);
    }

#if __cplusplus >= 202002L
    /// Detects anomalies in a time series from a span.
    template<typename T>
    inline AnomalyDetectionResult fit(std::span<const T> series, size_t period) const {
        return fit(series.data(), series.size(), period);
    }
#endif
};

/// Creates a new set of parameters.
inline AnomalyDetectionParams params() {
    return AnomalyDetectionParams();
}

}
