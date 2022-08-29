/*!
 * dist.h v0.3.0
 * https://github.com/ankane/dist.h
 * Unlicense OR MIT License
 */

#pragma once

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

#ifdef M_SQRT2
#define DIST_SQRT2 M_SQRT2
#else
#define DIST_SQRT2 1.41421356237309504880
#endif

double normal_pdf(double x, double mean, double std_dev) {
    if (std_dev <= 0) {
        return NAN;
    }

    double n = (x - mean) / std_dev;
    return (1.0 / (std_dev * sqrt(2.0 * DIST_PI))) * pow(DIST_E, -0.5 * n * n);
}

double normal_cdf(double x, double mean, double std_dev) {
    if (std_dev <= 0) {
        return NAN;
    }

    return 0.5 * (1.0 + erf((x - mean) / (std_dev * DIST_SQRT2)));
}

// Wichura, M. J. (1988).
// Algorithm AS 241: The Percentage Points of the Normal Distribution.
// Journal of the Royal Statistical Society. Series C (Applied Statistics), 37(3), 477-484.
double normal_ppf(double p, double mean, double std_dev) {
    if (p < 0 || p > 1 || std_dev <= 0 || isnan(mean) || isnan(std_dev)) {
        return NAN;
    }

    if (p == 0) {
        return -INFINITY;
    }

    if (p == 1) {
        return INFINITY;
    }

    double q = p - 0.5;
    if (fabs(q) < 0.425) {
        double r = 0.180625 - q * q;
        return mean + std_dev * q *
            (((((((2.5090809287301226727e3 * r + 3.3430575583588128105e4) * r + 6.7265770927008700853e4) * r + 4.5921953931549871457e4) * r + 1.3731693765509461125e4) * r + 1.9715909503065514427e3) * r + 1.3314166789178437745e2) * r + 3.3871328727963666080e0) /
            (((((((5.2264952788528545610e3 * r + 2.8729085735721942674e4) * r + 3.9307895800092710610e4) * r + 2.1213794301586595867e4) * r + 5.3941960214247511077e3) * r + 6.8718700749205790830e2) * r + 4.2313330701600911252e1) * r + 1);
    } else {
        double r = q < 0 ? p : 1 - p;
        r = sqrt(-log(r));
        double sign = q < 0 ? -1 : 1;
        if (r < 5) {
            r -= 1.6;
            return mean + std_dev * sign *
                (((((((7.74545014278341407640e-4 * r + 2.27238449892691845833e-2) * r + 2.41780725177450611770e-1) * r + 1.27045825245236838258e0) * r + 3.64784832476320460504e0) * r + 5.76949722146069140550e0) * r + 4.63033784615654529590e0) * r + 1.42343711074968357734e0) /
                (((((((1.05075007164441684324e-9 * r + 5.47593808499534494600e-4) * r + 1.51986665636164571966e-2) * r + 1.48103976427480074590e-1) * r + 6.89767334985100004550e-1) * r + 1.67638483018380384940e0) * r + 2.05319162663775882187e0) * r + 1);
        } else {
            r -= 5;
            return mean + std_dev * sign *
                (((((((2.01033439929228813265e-7 * r + 2.71155556874348757815e-5) * r + 1.24266094738807843860e-3) * r + 2.65321895265761230930e-2) * r + 2.96560571828504891230e-1) * r + 1.78482653991729133580e0) * r + 5.46378491116411436990e0) * r + 6.65790464350110377720e0) /
                (((((((2.04426310338993978564e-15 * r + 1.42151175831644588870e-7) * r + 1.84631831751005468180e-5) * r + 7.86869131145613259100e-4) * r + 1.48753612908506148525e-2) * r + 1.36929880922735805310e-1) * r + 5.99832206555887937690e-1) * r + 1);
        }
    }
}

double students_t_pdf(double x, double n) {
    if (n <= 0) {
        return NAN;
    }

    if (n == INFINITY) {
        return normal_pdf(x, 0, 1);
    }

    return tgamma((n + 1.0) / 2.0) / (sqrt(n * DIST_PI) * tgamma(n / 2.0)) * pow(1.0 + x * x / n, -(n + 1.0) / 2.0);
}

// Hill, G. W. (1970).
// Algorithm 395: Student's t-distribution.
// Communications of the ACM, 13(10), 617-619.
double students_t_cdf(double x, double n) {
    if (n < 1) {
        return NAN;
    }

    if (isnan(x)) {
        return NAN;
    }

    if (!isfinite(x)) {
        return x < 0 ? 0 : 1;
    }

    if (n == INFINITY) {
        return normal_cdf(x, 0, 1);
    }

    double start = x < 0 ? 0 : 1;
    double sign = x < 0 ? 1 : -1;

    double z = 1.0;
    double t = x * x;
    double y = t / n;
    double b = 1.0 + y;

    if (n > floor(n) || (n >= 20 && t < n) || n > 200) {
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

    // make n int
    // n is int between 1 and 200 if made it here
    n = (int) n;

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
double students_t_ppf(double p, double n) {
    if (p < 0 || p > 1 || n < 1) {
        return NAN;
    }

    if (n == INFINITY) {
        return normal_ppf(p, 0, 1);
    }

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
