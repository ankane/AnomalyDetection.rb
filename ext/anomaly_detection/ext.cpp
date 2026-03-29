#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

#include <rice/rice.hpp>
#include <rice/stl.hpp>

#include "anomaly_detection.hpp"

using anomaly_detection::AnomalyDetection;
using anomaly_detection::AnomalyDetectionParams;
using anomaly_detection::Direction;

extern "C"
void Init_ext() {
  auto rb_mAnomalyDetection = Rice::define_module("AnomalyDetection");

  rb_mAnomalyDetection
    .define_singleton_function(
      "_detect",
      [](const std::vector<float>& series, size_t period, float k, float alpha, const std::string& direction, bool verbose) {
        Direction dir;
        if (direction == "pos") {
          dir = Direction::Positive;
        } else if (direction == "neg") {
          dir = Direction::Negative;
        } else if (direction == "both") {
          dir = Direction::Both;
        } else {
          throw std::invalid_argument("direction must be pos, neg, or both");
        }

        AnomalyDetectionParams params{
          .alpha = alpha,
          .max_anoms = k,
          .direction = dir,
          .verbose = verbose,
          .callback = rb_thread_check_ints
        };
        AnomalyDetection res{series, period, params};

        Rice::Array a;
        for (auto v : res.anomalies()) {
          a.push(v, false);
        }
        return a;
      });
}
