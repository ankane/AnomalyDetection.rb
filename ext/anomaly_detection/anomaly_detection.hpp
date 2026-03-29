/*
 * AnomalyDetection.cpp v0.3.0
 * https://github.com/ankane/AnomalyDetection.cpp
 * GPL-3.0-or-later License
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

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

namespace detail {

template<typename T>
T median_sorted(const std::vector<T>& sorted) {
    return (sorted.at((sorted.size() - 1) / 2) + sorted.at(sorted.size() / 2))
        / static_cast<T>(2.0);
}

template<typename T>
T median(std::span<const T> data) {
    std::vector<T> sorted(data.begin(), data.end());
    std::ranges::sort(sorted);
    return median_sorted(sorted);
}

template<typename T>
T mad(const std::vector<T>& data, T med) {
    std::vector<T> res;
    res.reserve(data.size());
    for (auto v : data) {
        res.push_back(std::abs(v - med));
    }
    std::ranges::sort(res);
    return static_cast<T>(1.4826) * median_sorted(res);
}

template<typename T>
std::vector<size_t> detect_anoms(
    std::span<const T> data,
    size_t num_obs_per_period,
    float k,
    float alpha,
    bool one_tail,
    bool upper_tail,
    bool verbose,
    const std::function<void()>& callback
) {
    size_t n = data.size();

    // Check to make sure we have at least two periods worth of data for anomaly context
    if (n / 2 < num_obs_per_period) {
        throw std::invalid_argument{"series must contain at least 2 periods"};
    }

    // Handle NANs
    bool nans = std::ranges::any_of(data, [](const auto& value) { return std::isnan(value); });
    if (nans) {
        throw std::invalid_argument{"series contains NANs"};
    }

    if (k < 0) {
        throw std::invalid_argument{"max_anoms must be non-negative"};
    }

    if (k >= 0.5) {
        throw std::invalid_argument{"max_anoms must be less than 50% of the data points"};
    }

    if (alpha < 0) {
        throw std::invalid_argument{"alpha must be non-negative"};
    }

    if (alpha > 0.5) {
        throw std::invalid_argument{"alpha must not be greater than 0.5"};
    }

    std::vector<T> data2;
    data2.reserve(n);
    T med = median(data);

    if (num_obs_per_period > 1) {
        // Decompose data. This returns a univarite remainder which will be used for anomaly detection. Optionally, we might NOT decompose.
        stl::Stl data_decomp{
            data, num_obs_per_period, {.seasonal_length = data.size() * 10 + 1, .robust = true}
        };
        const std::vector<T>& seasonal = data_decomp.seasonal();

        // TODO use std::views::zip for C++23
        size_t i = 0;
        for (auto v : data) {
            data2.push_back(v - seasonal.at(i) - med);
            i++;
        }
    } else {
        for (auto v : data) {
            data2.push_back(v - med);
        }
    }

    size_t num_anoms = 0;
    auto max_outliers = static_cast<size_t>(static_cast<float>(n) * k);
    std::vector<size_t> anomalies;
    anomalies.reserve(max_outliers);

    // Sort data for fast median
    // Use stable sort for indexes for deterministic results
    std::vector<size_t> indexes(n);
    std::iota(indexes.begin(), indexes.end(), 0);
    std::ranges::stable_sort(indexes, [&data2](size_t a, size_t b) {
        return data2.at(a) < data2.at(b);
    });
    std::ranges::sort(data2);

    // Compute test statistic until r=max_outliers values have been removed from the sample
    for (size_t i = 1; i <= max_outliers; i++) {
        if (verbose) {
            std::cout << i << " / " << max_outliers << " completed" << std::endl;
        }

        // TODO Improve performance between loop iterations
        T ma = median_sorted(data2);
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
        T data_sigma = mad(data2, ma);
        if (data_sigma == 0.0) {
            break;
        }

        auto iter = std::ranges::max_element(ares);
        ptrdiff_t r_idx_i = std::distance(ares.begin(), iter);

        // Only need to take sigma of r for performance
        T r = ares.at(static_cast<size_t>(r_idx_i)) / data_sigma;

        anomalies.push_back(indexes.at(static_cast<size_t>(r_idx_i)));
        data2.erase(data2.begin() + r_idx_i);
        indexes.erase(indexes.begin() + r_idx_i);

        // Compute critical value
        double p = one_tail
            ? (1.0 - alpha / static_cast<double>(n - i + 1))
            : (1.0 - alpha / (2.0 * static_cast<double>(n - i + 1)));

        double t = students_t_ppf(p, static_cast<double>(n - i - 1));
        double lam = t * static_cast<double>(n - i)
            / std::sqrt((static_cast<double>(n - i - 1) + t * t) * static_cast<double>(n - i + 1));

        if (r > lam) {
            num_anoms = i;
        }

        if (callback != nullptr) {
            callback();
        }
    }

    anomalies.resize(num_anoms);

    // Sort like R version
    std::ranges::sort(anomalies);

    return anomalies;
}

} // namespace detail

/// A set of anomaly detection parameters.
struct AnomalyDetectionParams {
    /// Sets the level of statistical significance.
    float alpha = 0.05f;
    /// Sets the maximum number of anomalies as percent of data.
    float max_anoms = 0.1f;
    /// Sets the direction.
    Direction direction = Direction::Both;
    /// Sets whether to show progress.
    bool verbose = false;
    /// Sets a callback for each iteration.
    std::function<void()> callback = nullptr;
};

/// An anomaly detection result.
class AnomalyDetection {
  public:
    /// Detects anomalies in a time series from a span.
    template<typename T>
    AnomalyDetection(
        std::span<const T> series,
        size_t period,
        const AnomalyDetectionParams& params = AnomalyDetectionParams()
    ) {
        bool one_tail = params.direction != Direction::Both;
        bool upper_tail = params.direction == Direction::Positive;

        std::vector<size_t> anomalies = detail::detect_anoms(
            series,
            period,
            params.max_anoms,
            params.alpha,
            one_tail,
            upper_tail,
            params.verbose,
            params.callback
        );
        anomalies_ = std::move(anomalies);
    }

    /// Detects anomalies in a time series from a vector.
    template<typename T>
    AnomalyDetection(
        const std::vector<T>& series,
        size_t period,
        const AnomalyDetectionParams& params = AnomalyDetectionParams()
    ) :
        AnomalyDetection(std::span<const T>{series}, period, params) {}

    /// Returns the anomalies.
    const std::vector<size_t>& anomalies() const {
        return anomalies_;
    }

  private:
    std::vector<size_t> anomalies_;
};

} // namespace anomaly_detection
