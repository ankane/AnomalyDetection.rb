/*!
 * dist.h v0.1.1
 * https://github.com/ankane/dist.h
 * Unlicense OR MIT License
 */

#pragma once

#include <assert.h>
#include <math.h>

#ifdef M_E
#define DIST_E M_E
#else
#define DIST_E 2.71828182845904523536
#endif

#ifdef M_PI
#define DIST_PI M_PI
#else
#define DIST_PI 3.14159265358979323846
#endif

// Winitzki, S. (2008).
// A handy approximation for the error function and its inverse.
// https://drive.google.com/file/d/0B2Mt7luZYBrwZlctV3A3eF82VGM/view?resourcekey=0-UQpPhwZgzP0sF4LHBDlLtg
// from https://sites.google.com/site/winitzki
double erf(double x) {
    double sign = x < 0 ? -1.0 : 1.0;
    x = x < 0 ? -x : x;

    double a = 0.14;
    double x2 = x * x;
    return sign * sqrt(1.0 - exp(-x2 * (4.0 / DIST_PI + a * x2) / (1.0 + a * x2)));
}

// Winitzki, S. (2008).
// A handy approximation for the error function and its inverse.
// https://drive.google.com/file/d/0B2Mt7luZYBrwZlctV3A3eF82VGM/view?resourcekey=0-UQpPhwZgzP0sF4LHBDlLtg
// from https://sites.google.com/site/winitzki
double inverse_erf(double x) {
    double sign = x < 0 ? -1.0 : 1.0;
    x = x < 0 ? -x : x;

    double a = 0.147;
    double ln = log(1.0 - x * x);
    double f1 = 2.0 / (DIST_PI * a);
    double f2 = ln / 2.0;
    double f3 = f1 + f2;
    double f4 = 1.0 / a * ln;
    return sign * sqrt(-f1 - f2 + sqrt(f3 * f3 - f4));
}

double normal_pdf(double x, double mean, double std_dev) {
    double var = std_dev * std_dev;
    return (1.0 / (var * sqrt(2.0 * DIST_PI))) * pow(DIST_E, -0.5 * pow((x - mean) / var, 2));
}

double normal_cdf(double x, double mean, double std_dev) {
    return 0.5 * (1.0 + erf((x - mean) / (std_dev * std_dev * sqrt(2))));
}

double normal_ppf(double p, double mean, double std_dev) {
    assert(p >= 0 && p <= 1);

    return mean + (std_dev * std_dev) * sqrt(2) * inverse_erf(2.0 * p - 1.0);
}

double students_t_pdf(double x, unsigned int n) {
    assert(n >= 1);

    return tgamma((n + 1.0) / 2.0) / (sqrt(n * DIST_PI) * tgamma(n / 2.0)) * pow(1.0 + x * x / n, -(n + 1.0) / 2.0);
}

// Hill, G. W. (1970).
// Algorithm 395: Student's t-distribution.
// Communications of the ACM, 13(10), 617-619.
double students_t_cdf(double x, unsigned int n) {
    assert(n >= 1);

    double start = x < 0 ? 0 : 1;
    double sign = x < 0 ? 1 : -1;

    double z = 1.0;
    double t = x * x;
    double y = t / n;
    double b = 1.0 + y;

    if ((n >= 20 && t < n) || n > 200) {
        // asymptotic series for large or noninteger n
        if (y > 10e-6) {
            y = log(b);
        }
        double a = n - 0.5;
        b = 48.0 * a * a;
        y = a * y;
        y = (((((-0.4 * y - 3.3) * y - 24.0) * y - 85.5) / (0.8 * y * y + 100.0 + b) + y + 3.0) / b + 1.0) * sqrt(y);
        return start + sign * normal_cdf(-y, 0.0, 1.0);
    }

    if (n < 20 && t < 4.0) {
        // nested summation of cosine series
        y = sqrt(y);
        double a = y;
        if (n == 1) {
            a = 0.0;
        }

        // loop
        if (n > 1) {
            n -= 2;
            while (n > 1) {
                a = (n - 1) / (b * n) * a + y;
                n -= 2;
            }
        }
        a = n == 0 ? a / sqrt(b) : (atan(y) + a / b) * (2.0 / DIST_PI);
        return start + sign * (z - a) / 2;
    }

    // tail series expanation for large t-values
    double a = sqrt(b);
    y = a * n;
    int j = 0;
    while (a != z) {
        j += 2;
        z = a;
        y = y * (j - 1) / (b * j);
        a = a + y / (n + j);
    }
    z = 0.0;
    y = 0.0;
    a = -a;

    // loop (without n + 2 and n - 2)
    while (n > 1) {
        a = (n - 1) / (b * n) * a + y;
        n -= 2;
    }
    a = n == 0 ? a / sqrt(b) : (atan(y) + a / b) * (2.0 / DIST_PI);
    return start + sign * (z - a) / 2;
}

// Hill, G. W. (1970).
// Algorithm 396: Student's t-quantiles.
// Communications of the ACM, 13(10), 619-620.
double students_t_ppf(double p, unsigned int n) {
    assert(p >= 0 && p <= 1);
    assert(n >= 1);

    // distribution is symmetric
    double sign = p < 0.5 ? -1 : 1;
    p = p < 0.5 ? 1 - p : p;

    // two-tail to one-tail
    p = 2.0 * (1.0 - p);

    if (n == 2) {
        return sign * sqrt(2.0 / (p * (2.0 - p)) - 2.0);
    }

    double half_pi = DIST_PI / 2.0;

    if (n == 1) {
        p = p * half_pi;
        return sign * cos(p) / sin(p);
    }

    double a = 1.0 / (n - 0.5);
    double b = 48.0 / (a * a);
    double c = ((20700.0 * a / b - 98.0) * a - 16.0) * a + 96.36;
    double d = ((94.5 / (b + c) - 3.0) / b + 1.0) * sqrt(a * half_pi) * n;
    double x = d * p;
    double y = pow(x, 2.0 / n);
    if (y > 0.05 + a) {
        // asymptotic inverse expansion about normal
        x = normal_ppf(p * 0.5, 0.0, 1.0);
        y = x * x;
        if (n < 5) {
            c += 0.3 * (n - 4.5) * (x + 0.6);
        }
        c = (((0.05 * d * x - 5.0) * x - 7.0) * x - 2.0) * x + b + c;
        y = (((((0.4 * y + 6.3) * y + 36.0) * y + 94.5) / c - y - 3.0) / b + 1.0) * x;
        y = a * y * y;
        y = y > 0.002 ? exp(y) - 1.0 : 0.5 * y * y + y;
    } else {
        y = ((1.0 / (((n + 6.0) / (n * y) - 0.089 * d - 0.822) * (n + 2.0) * 3.0) + 0.5 / (n + 4.0)) * y - 1.0) * (n + 1.0) / (n + 2.0) + 1.0 / y;
    }
    return sign * sqrt(n * y);
}
