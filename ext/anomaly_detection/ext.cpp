#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

#include <rice/rice.hpp>

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
      [](Rice::Array rb_series, size_t period, float k, float alpha, Rice::String rb_direction, bool verbose) {
        std::vector<float> series = rb_series.to_vector<float>();
        std::string direction = rb_direction.str();

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
