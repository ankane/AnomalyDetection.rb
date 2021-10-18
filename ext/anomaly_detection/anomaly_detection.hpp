#pragma once

#include <string>
#include <vector>

namespace anomaly_detection {

std::vector<size_t> anomalies(const std::vector<float>& x, int period, float k, float alpha, const std::string& direction);

}
