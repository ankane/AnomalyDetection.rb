#pragma once

#include <string>
#include <vector>

namespace anomaly_detection {

enum Direction { Positive, Negative, Both };

std::vector<size_t> anomalies(const std::vector<float>& x, int period, float k, float alpha, Direction direction, bool verbose, std::function<void()> interrupt);

}
