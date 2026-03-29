/*
 * STL C++ v0.3.0
 * https://github.com/ankane/stl-cpp
 * Unlicense OR MIT License
 *
 * Ported from https://www.netlib.org/a/stl
 *
 * Cleveland, R. B., Cleveland, W. S., McRae, J. E., & Terpenning, I. (1990).
 * STL: A Seasonal-Trend Decomposition Procedure Based on Loess.
 * Journal of Official Statistics, 6(1), 3-33.
 *
 * Bandara, K., Hyndman, R. J., & Bergmeir, C. (2021).
 * MSTL: A Seasonal-Trend Decomposition Algorithm for Time Series with Multiple Seasonal Patterns.
 * arXiv:2107.13462 [stat.AP]. https://doi.org/10.48550/arXiv.2107.13462
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <optional>
#include <span>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace stl {

namespace detail {

// TODO use span.at() for C++26
template<typename T>
T& span_at(std::span<T> sp, size_t pos) {
    if (pos >= sp.size()) [[unlikely]] {
        throw std::out_of_range("pos >= size()");
    }
    return sp[pos];
}

template<typename T>
bool est(
    const std::vector<T>& y,
    size_t n,
    size_t len,
    int ideg,
    T xs,
    T& ys,
    size_t nleft,
    size_t nright,
    std::vector<T>& w,
    bool userw,
    const std::vector<T>& rw
) {
    T range = static_cast<T>(n) - static_cast<T>(1.0);
    T h = std::max(xs - static_cast<T>(nleft), static_cast<T>(nright) - xs);

    if (len > n) {
        h += static_cast<T>((len - n) / 2);
    }

    T h9 = static_cast<T>(0.999) * h;
    T h1 = static_cast<T>(0.001) * h;

    // compute weights
    T a = 0.0;
    for (size_t j = nleft; j <= nright; j++) {
        w.at(j - 1) = 0.0;
        T r = std::abs(static_cast<T>(j) - xs);
        if (r <= h9) {
            if (r <= h1) {
                w.at(j - 1) = 1.0;
            } else {
                w.at(j - 1) = static_cast<T>(std::pow(1.0 - std::pow(r / h, 3.0), 3.0));
            }
            if (userw) {
                w.at(j - 1) *= rw.at(j - 1);
            }
            a += w.at(j - 1);
        }
    }

    if (a <= 0.0) {
        return false;
    } else {
        // weighted least squares
        for (size_t j = nleft; j <= nright; j++) {
            // make sum of w(j) == 1
            w.at(j - 1) /= a;
        }

        if (h > 0.0 && ideg > 0) {
            // use linear fit
            T a = 0.0;
            for (size_t j = nleft; j <= nright; j++) {
                // weighted center of x values
                a += w.at(j - 1) * static_cast<T>(j);
            }
            T b = xs - a;
            T c = 0.0;
            for (size_t j = nleft; j <= nright; j++) {
                c += w.at(j - 1) * std::pow(static_cast<T>(j) - a, static_cast<T>(2.0));
            }
            if (std::sqrt(c) > 0.001 * range) {
                b /= c;

                // points are spread out enough to compute slope
                for (size_t j = nleft; j <= nright; j++) {
                    w.at(j - 1) *= b * (static_cast<T>(j) - a) + static_cast<T>(1.0);
                }
            }
        }

        ys = 0.0;
        for (size_t j = nleft; j <= nright; j++) {
            ys += w.at(j - 1) * y.at(j - 1);
        }

        return true;
    }
}

template<typename T>
void ess(
    const std::vector<T>& y,
    size_t n,
    size_t len,
    int ideg,
    size_t njump,
    bool userw,
    const std::vector<T>& rw,
    std::span<T> ys,
    std::vector<T>& res
) {
    if (n < 2) {
        span_at(ys, 0) = y.at(0);
        return;
    }

    size_t nleft = 0;
    size_t nright = 0;

    size_t newnj = std::min(njump, n - 1);
    if (len >= n) {
        nleft = 1;
        nright = n;
        for (size_t i = 1; i <= n; i += newnj) {
            bool ok = est(
                y, n, len, ideg, static_cast<T>(i), span_at(ys, i - 1), nleft, nright, res, userw,
                rw
            );
            if (!ok) {
                span_at(ys, i - 1) = y.at(i - 1);
            }
        }
    } else if (newnj == 1) {
        // newnj equal to one, len less than n
        size_t nsh = (len + 1) / 2;
        nleft = 1;
        nright = len;
        for (size_t i = 1; i <= n; i++) {
            // fitted value at i
            if (i > nsh && nright != n) {
                nleft += 1;
                nright += 1;
            }
            bool ok = est(
                y, n, len, ideg, static_cast<T>(i), span_at(ys, i - 1), nleft, nright, res, userw,
                rw
            );
            if (!ok) {
                span_at(ys, i - 1) = y.at(i - 1);
            }
        }
    } else {
        // newnj greater than one, len less than n
        size_t nsh = (len + 1) / 2;
        for (size_t i = 1; i <= n; i += newnj) {
            // fitted value at i
            if (i < nsh) {
                nleft = 1;
                nright = len;
            } else if (i >= n - nsh + 1) {
                nleft = n - len + 1;
                nright = n;
            } else {
                nleft = i - nsh + 1;
                nright = len + i - nsh;
            }
            bool ok = est(
                y, n, len, ideg, static_cast<T>(i), span_at(ys, i - 1), nleft, nright, res, userw,
                rw
            );
            if (!ok) {
                span_at(ys, i - 1) = y.at(i - 1);
            }
        }
    }

    if (newnj != 1) {
        for (size_t i = 1; i <= n - newnj; i += newnj) {
            T delta = (span_at(ys, i + newnj - 1) - span_at(ys, i - 1)) / static_cast<T>(newnj);
            for (size_t j = i + 1; j <= i + newnj - 1; j++) {
                span_at(ys, j - 1) = span_at(ys, i - 1) + delta * static_cast<T>(j - i);
            }
        }
        size_t k = ((n - 1) / newnj) * newnj + 1;
        if (k != n) {
            bool ok = est(
                y, n, len, ideg, static_cast<T>(n), span_at(ys, n - 1), nleft, nright, res, userw,
                rw
            );
            if (!ok) {
                span_at(ys, n - 1) = y.at(n - 1);
            }
            if (k != n - 1) {
                T delta = (span_at(ys, n - 1) - span_at(ys, k - 1)) / static_cast<T>(n - k);
                for (size_t j = k + 1; j <= n - 1; j++) {
                    span_at(ys, j - 1) = span_at(ys, k - 1) + delta * static_cast<T>(j - k);
                }
            }
        }
    }
}

template<typename T>
void ma(const std::vector<T>& x, size_t n, size_t len, std::vector<T>& ave) {
    size_t newn = n - len + 1;
    auto flen = static_cast<double>(len);
    double v = 0.0;

    // get the first average
    for (size_t i = 0; i < len; i++) {
        v += x.at(i);
    }

    ave.at(0) = static_cast<T>(v / flen);
    if (newn > 1) {
        size_t k = len;
        size_t m = 0;
        for (size_t j = 1; j < newn; j++) {
            // window down the array
            v = v - x.at(m) + x.at(k);
            ave.at(j) = static_cast<T>(v / flen);
            k += 1;
            m += 1;
        }
    }
}

template<typename T>
void fts(
    const std::vector<T>& x,
    size_t n,
    size_t np,
    std::vector<T>& trend,
    std::vector<T>& work
) {
    ma(x, n, np, trend);
    ma(trend, n - np + 1, np, work);
    ma(work, n - 2 * np + 2, 3, trend);
}

template<typename T>
void rwts(std::span<const T> y, const std::vector<T>& fit, std::vector<T>& rw) {
    // TODO use std::views::zip for C++23
    for (size_t i = 0; i < y.size(); i++) {
        rw.at(i) = std::abs(span_at(y, i) - fit.at(i));
    }

    size_t n = y.size();
    size_t mid1 = (n - 1) / 2;
    size_t mid2 = n / 2;

    // sort
    std::ranges::sort(rw);

    T cmad = static_cast<T>(3.0) * (rw.at(mid1) + rw.at(mid2)); // 6 * median abs resid
    T c9 = static_cast<T>(0.999) * cmad;
    T c1 = static_cast<T>(0.001) * cmad;

    // TODO use std::views::zip for C++23
    for (size_t i = 0; i < y.size(); i++) {
        T r = std::abs(span_at(y, i) - fit.at(i));
        if (r <= c1) {
            rw.at(i) = 1.0;
        } else if (r <= c9) {
            rw.at(i) = static_cast<T>(std::pow(1.0 - std::pow(r / cmad, 2.0), 2.0));
        } else {
            rw.at(i) = 0.0;
        }
    }
}

template<typename T>
void ss(
    const std::vector<T>& y,
    size_t n,
    size_t np,
    size_t ns,
    int isdeg,
    size_t nsjump,
    bool userw,
    std::vector<T>& rw,
    std::vector<T>& season,
    std::vector<T>& work1,
    std::vector<T>& work2,
    std::vector<T>& work3,
    std::vector<T>& work4
) {
    for (size_t j = 1; j <= np; j++) {
        size_t k = (n - j) / np + 1;

        for (size_t i = 1; i <= k; i++) {
            work1.at(i - 1) = y.at((i - 1) * np + j - 1);
        }
        if (userw) {
            for (size_t i = 1; i <= k; i++) {
                work3.at(i - 1) = rw.at((i - 1) * np + j - 1);
            }
        }
        ess(work1, k, ns, isdeg, nsjump, userw, work3, std::span{work2}.subspan(1), work4);
        T xs = 0.0;
        size_t nright = std::min(ns, k);
        bool ok = est(work1, k, ns, isdeg, xs, work2.at(0), 1, nright, work4, userw, work3);
        if (!ok) {
            work2.at(0) = work2.at(1);
        }
        xs = static_cast<T>(k + 1);
        size_t nleft = static_cast<size_t>(
            std::max(1, static_cast<int>(k) - static_cast<int>(ns) + 1)
        );
        ok = est(work1, k, ns, isdeg, xs, work2.at(k + 1), nleft, k, work4, userw, work3);
        if (!ok) {
            work2.at(k + 1) = work2.at(k);
        }
        for (size_t m = 1; m <= k + 2; m++) {
            season.at((m - 1) * np + j - 1) = work2.at(m - 1);
        }
    }
}

template<typename T>
void onestp(
    std::span<const T> y,
    size_t np,
    size_t ns,
    size_t nt,
    size_t nl,
    int isdeg,
    int itdeg,
    int ildeg,
    size_t nsjump,
    size_t ntjump,
    size_t nljump,
    size_t ni,
    bool userw,
    std::vector<T>& rw,
    std::vector<T>& season,
    std::vector<T>& trend,
    std::vector<T>& work1,
    std::vector<T>& work2,
    std::vector<T>& work3,
    std::vector<T>& work4,
    std::vector<T>& work5
) {
    size_t n = y.size();

    for (size_t j = 0; j < ni; j++) {
        // TODO use std::views::zip for C++23
        for (size_t i = 0; i < y.size(); i++) {
            work1.at(i) = span_at(y, i) - trend.at(i);
        }

        ss(work1, n, np, ns, isdeg, nsjump, userw, rw, work2, work3, work4, work5, season);
        fts(work2, n + 2 * np, np, work3, work1);
        ess(work3, n, nl, ildeg, nljump, false, work4, std::span{work1}, work5);
        // TODO use std::views::zip for C++23
        for (size_t i = 0; i < n; i++) {
            season.at(i) = work2.at(np + i) - work1.at(i);
        }
        // TODO use std::views::zip for C++23
        for (size_t i = 0; i < y.size(); i++) {
            work1.at(i) = span_at(y, i) - season.at(i);
        }
        ess(work1, n, nt, itdeg, ntjump, userw, rw, std::span{trend}, work3);
    }
}

template<typename T>
void stl(
    std::span<const T> y,
    size_t np,
    size_t ns,
    size_t nt,
    size_t nl,
    int isdeg,
    int itdeg,
    int ildeg,
    size_t nsjump,
    size_t ntjump,
    size_t nljump,
    size_t ni,
    size_t no,
    std::vector<T>& rw,
    std::vector<T>& season,
    std::vector<T>& trend
) {
    size_t n = y.size();

    if (ns < 3) {
        throw std::invalid_argument{"seasonal_length must be at least 3"};
    }
    if (nt < 3) {
        throw std::invalid_argument{"trend_length must be at least 3"};
    }
    if (nl < 3) {
        throw std::invalid_argument{"low_pass_length must be at least 3"};
    }
    if (np < 2) {
        throw std::invalid_argument{"period must be at least 2"};
    }

    if (isdeg != 0 && isdeg != 1) {
        throw std::invalid_argument{"seasonal_degree must be 0 or 1"};
    }
    if (itdeg != 0 && itdeg != 1) {
        throw std::invalid_argument{"trend_degree must be 0 or 1"};
    }
    if (ildeg != 0 && ildeg != 1) {
        throw std::invalid_argument{"low_pass_degree must be 0 or 1"};
    }

    if (ns % 2 != 1) {
        throw std::invalid_argument{"seasonal_length must be odd"};
    }
    if (nt % 2 != 1) {
        throw std::invalid_argument{"trend_length must be odd"};
    }
    if (nl % 2 != 1) {
        throw std::invalid_argument{"low_pass_length must be odd"};
    }

    std::vector<T> work1(n + 2 * np);
    std::vector<T> work2(n + 2 * np);
    std::vector<T> work3(n + 2 * np);
    std::vector<T> work4(n + 2 * np);
    std::vector<T> work5(n + 2 * np);

    bool userw = false;
    size_t k = 0;

    while (true) {
        onestp(
            y,
            np,
            ns,
            nt,
            nl,
            isdeg,
            itdeg,
            ildeg,
            nsjump,
            ntjump,
            nljump,
            ni,
            userw,
            rw,
            season,
            trend,
            work1,
            work2,
            work3,
            work4,
            work5
        );
        k += 1;
        if (k > no) {
            break;
        }
        for (size_t i = 0; i < n; i++) {
            work1.at(i) = trend.at(i) + season.at(i);
        }
        rwts(y, work1, rw);
        userw = true;
    }

    if (no <= 0) {
        for (size_t i = 0; i < n; i++) {
            rw.at(i) = 1.0;
        }
    }
}

template<typename T>
double var(const std::vector<T>& series) {
    double mean = std::accumulate(series.begin(), series.end(), 0.0)
        / static_cast<double>(series.size());
    double sum = 0.0;
    for (auto v : series) {
        double diff = v - mean;
        sum += diff * diff;
    }
    return sum / static_cast<double>(series.size() - 1);
}

template<typename T>
double strength(const std::vector<T>& component, const std::vector<T>& remainder) {
    std::vector<T> sr;
    sr.reserve(remainder.size());
    for (size_t i = 0; i < remainder.size(); i++) {
        sr.push_back(component.at(i) + remainder.at(i));
    }
    return std::max(0.0, 1.0 - var(remainder) / var(sr));
}

} // namespace detail

/// A set of STL parameters.
struct StlParams {
    /// Sets the length of the seasonal smoother.
    std::optional<size_t> seasonal_length = std::nullopt;
    /// Sets the length of the trend smoother.
    std::optional<size_t> trend_length = std::nullopt;
    /// Sets the length of the low-pass filter.
    std::optional<size_t> low_pass_length = std::nullopt;
    /// Sets the degree of locally-fitted polynomial in seasonal smoothing.
    int seasonal_degree = 0;
    /// Sets the degree of locally-fitted polynomial in trend smoothing.
    int trend_degree = 1;
    /// Sets the degree of locally-fitted polynomial in low-pass smoothing.
    std::optional<int> low_pass_degree = std::nullopt;
    /// Sets the skipping value for seasonal smoothing.
    std::optional<size_t> seasonal_jump = std::nullopt;
    /// Sets the skipping value for trend smoothing.
    std::optional<size_t> trend_jump = std::nullopt;
    /// Sets the skipping value for low-pass smoothing.
    std::optional<size_t> low_pass_jump = std::nullopt;
    /// Sets the number of loops for updating the seasonal and trend components.
    std::optional<size_t> inner_loops = std::nullopt;
    /// Sets the number of iterations of robust fitting.
    std::optional<size_t> outer_loops = std::nullopt;
    /// Sets whether robustness iterations are to be used.
    bool robust = false;
};

/// Seasonal-trend decomposition using Loess (STL).
template<typename T = float>
class Stl {
  public:
    /// Decomposes a time series from a vector.
    Stl(const std::vector<T>& series, size_t period, const StlParams& params = StlParams());

    /// Decomposes a time series from a span.
    Stl(std::span<const T> series, size_t period, const StlParams& params = StlParams());

    /// Returns the seasonal component.
    const std::vector<T>& seasonal() const {
        return seasonal_;
    }

    /// Returns the trend component.
    const std::vector<T>& trend() const {
        return trend_;
    }

    /// Returns the remainder.
    const std::vector<T>& remainder() const {
        return remainder_;
    }

    /// Returns the weights.
    const std::vector<T>& weights() const {
        return weights_;
    }

    /// Returns the seasonal strength.
    double seasonal_strength() const {
        return detail::strength(seasonal_, remainder_);
    }

    /// Returns the trend strength.
    double trend_strength() const {
        return detail::strength(trend_, remainder_);
    }

  private:
    std::vector<T> seasonal_;
    std::vector<T> trend_;
    std::vector<T> remainder_;
    std::vector<T> weights_;
};

template<typename T>
Stl<T>::Stl(std::span<const T> series, size_t period, const StlParams& params) {
    std::span<const T> y = series;
    size_t np = period;
    size_t n = series.size();

    if (n / 2 < np) {
        throw std::invalid_argument{"series has less than two periods"};
    }

    size_t ns = params.seasonal_length.value_or(np);

    int isdeg = params.seasonal_degree;
    int itdeg = params.trend_degree;

    std::vector<T> seasonal(n);
    std::vector<T> trend(n);
    std::vector<T> remainder;
    std::vector<T> weights(n);

    int ildeg = params.low_pass_degree.value_or(itdeg);
    size_t newns = std::max(ns, static_cast<size_t>(3));
    if (newns % 2 == 0) {
        newns += 1;
    }

    size_t newnp = std::max(np, static_cast<size_t>(2));
    auto nt = static_cast<size_t>(
        std::ceil((1.5 * static_cast<float>(newnp)) / (1.0 - 1.5 / static_cast<float>(newns)))
    );
    nt = params.trend_length.value_or(nt);
    nt = std::max(nt, static_cast<size_t>(3));
    if (nt % 2 == 0) {
        nt += 1;
    }

    size_t nl = params.low_pass_length.value_or(newnp);
    if (nl % 2 == 0 && !params.low_pass_length.has_value()) {
        nl += 1;
    }

    size_t ni = params.inner_loops.value_or(params.robust ? 1 : 2);
    size_t no = params.outer_loops.value_or(params.robust ? 15 : 0);

    size_t nsjump = params.seasonal_jump.value_or(
        static_cast<size_t>(std::ceil(static_cast<float>(newns) / 10.0))
    );
    size_t ntjump = params.trend_jump.value_or(
        static_cast<size_t>(std::ceil(static_cast<float>(nt) / 10.0))
    );
    size_t nljump = params.low_pass_jump.value_or(
        static_cast<size_t>(std::ceil(static_cast<float>(nl) / 10.0))
    );

    detail::stl(
        y,
        newnp,
        newns,
        nt,
        nl,
        isdeg,
        itdeg,
        ildeg,
        nsjump,
        ntjump,
        nljump,
        ni,
        no,
        weights,
        seasonal,
        trend
    );

    remainder.reserve(n);
    // TODO use std::views::zip for C++23
    for (size_t i = 0; i < y.size(); i++) {
        remainder.push_back(detail::span_at(y, i) - seasonal.at(i) - trend.at(i));
    }

    seasonal_ = std::move(seasonal);
    trend_ = std::move(trend);
    remainder_ = std::move(remainder);
    weights_ = std::move(weights);
}

template<typename T>
Stl<T>::Stl(const std::vector<T>& series, size_t period, const StlParams& params) :
    Stl(std::span{series}, period, params) {}

/// A set of MSTL parameters.
struct MstlParams {
    /// Sets the number of iterations.
    size_t iterations = 2;
    /// Sets lambda for Box-Cox transformation.
    std::optional<float> lambda = std::nullopt;
    /// Sets the lengths of the seasonal smoothers.
    std::optional<std::vector<size_t>> seasonal_lengths = std::nullopt;
    /// Sets the STL parameters.
    StlParams stl_params = StlParams();
};

/// Multiple seasonal-trend decomposition using Loess (MSTL).
template<typename T = float>
class Mstl {
  public:
    /// Decomposes a time series from a vector.
    Mstl(
        const std::vector<T>& series,
        const std::vector<size_t>& periods,
        const MstlParams& params = MstlParams()
    );

    /// Decomposes a time series from a span.
    Mstl(
        std::span<const T> series,
        std::span<const size_t> periods,
        const MstlParams& params = MstlParams()
    );

    /// Returns the seasonal component.
    const std::vector<std::vector<T>>& seasonal() const {
        return seasonal_;
    }

    /// Returns the trend component.
    const std::vector<T>& trend() const {
        return trend_;
    }

    /// Returns the remainder.
    const std::vector<T>& remainder() const {
        return remainder_;
    }

    /// Returns the seasonal strength.
    std::vector<double> seasonal_strength() const {
        std::vector<double> res;
        res.reserve(seasonal_.size());
        for (const auto& s : seasonal_) {
            res.push_back(detail::strength(s, remainder_));
        }
        return res;
    }

    /// Returns the trend strength.
    double trend_strength() const {
        return detail::strength(trend_, remainder_);
    }

  private:
    std::vector<std::vector<T>> seasonal_;
    std::vector<T> trend_;
    std::vector<T> remainder_;
};

namespace detail {

template<typename T>
std::vector<T> box_cox(std::span<const T> y, float lambda) {
    std::vector<T> res;
    res.reserve(y.size());
    if (lambda != 0.0) {
        for (auto yi : y) {
            res.push_back(static_cast<T>(std::pow(yi, lambda) - 1.0) / lambda);
        }
    } else {
        for (auto yi : y) {
            res.push_back(std::log(yi));
        }
    }
    return res;
}

template<typename T>
std::tuple<std::vector<T>, std::vector<T>, std::vector<std::vector<T>>> mstl(
    std::span<const T> x,
    std::span<const size_t> seas_ids,
    size_t iterate,
    std::optional<float> lambda,
    const std::optional<std::vector<size_t>>& swin,
    const StlParams& stl_params
) {
    // keep track of indices instead of sorting seas_ids
    // so order is preserved with seasonality
    std::vector<size_t> indices(seas_ids.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::ranges::sort(indices, [&seas_ids](size_t a, size_t b) {
        return span_at(seas_ids, a) < span_at(seas_ids, b);
    });

    if (seas_ids.size() == 1) {
        iterate = 1;
    }

    std::vector<std::vector<T>> seasonality;
    seasonality.reserve(seas_ids.size());
    std::vector<T> trend;

    std::vector<T> deseas = lambda.has_value()
        ? box_cox(x, lambda.value())
        : std::vector<T>(x.begin(), x.end());

    if (!seas_ids.empty()) {
        for (size_t i = 0; i < seas_ids.size(); i++) {
            seasonality.push_back(std::vector<T>());
        }

        for (size_t j = 0; j < iterate; j++) {
            for (size_t i = 0; i < indices.size(); i++) {
                size_t idx = indices.at(i);

                if (j > 0) {
                    for (size_t ii = 0; ii < deseas.size(); ii++) {
                        deseas.at(ii) += seasonality.at(idx).at(ii);
                    }
                }

                StlParams params = stl_params;
                if (swin) {
                    params.seasonal_length = swin.value().at(idx);
                } else if (!stl_params.seasonal_length.has_value()) {
                    params.seasonal_length = 7 + 4 * (i + 1);
                }
                Stl<T> fit{deseas, span_at(seas_ids, idx), params};

                seasonality.at(idx) = fit.seasonal();
                trend = fit.trend();

                for (size_t ii = 0; ii < deseas.size(); ii++) {
                    deseas.at(ii) -= seasonality.at(idx).at(ii);
                }
            }
        }
    } else {
        // TODO use Friedman's Super Smoother for trend
        throw std::invalid_argument{"periods must not be empty"};
    }

    std::vector<T> remainder;
    remainder.reserve(x.size());
    for (size_t i = 0; i < x.size(); i++) {
        remainder.push_back(deseas.at(i) - trend.at(i));
    }

    return std::make_tuple(trend, remainder, seasonality);
}

} // namespace detail

template<typename T>
Mstl<T>::Mstl(
    std::span<const T> series,
    std::span<const size_t> periods,
    const MstlParams& params
) {
    // return error to be consistent with stl
    // and ensure seasonal is always same length as periods
    for (auto v : periods) {
        if (v < 2) {
            throw std::invalid_argument{"periods must be at least 2"};
        }
    }

    // return error to be consistent with stl
    // and ensure seasonal is always same length as periods
    for (auto v : periods) {
        if (series.size() < v * 2) {
            throw std::invalid_argument{"series has less than two periods"};
        }
    }

    if (params.lambda.has_value()) {
        float lambda = params.lambda.value();
        if (lambda < 0 || lambda > 1) {
            throw std::invalid_argument{"lambda must be between 0 and 1"};
        }
    }

    if (params.seasonal_lengths.has_value()) {
        if (params.seasonal_lengths.value().size() != periods.size()) {
            throw std::invalid_argument{"seasonal_lengths must have the same length as periods"};
        }
    }

    auto [trend, remainder, seasonal] = detail::mstl(
        series,
        periods,
        params.iterations,
        params.lambda,
        params.seasonal_lengths,
        params.stl_params
    );

    seasonal_ = std::move(seasonal);
    trend_ = std::move(trend);
    remainder_ = std::move(remainder);
}

template<typename T>
Mstl<T>::Mstl(
    const std::vector<T>& series,
    const std::vector<size_t>& periods,
    const MstlParams& params
) :
    Mstl(std::span{series}, std::span{periods}, params) {}

} // namespace stl
