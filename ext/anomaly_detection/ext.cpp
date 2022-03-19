#include <rice/rice.hpp>
#include <rice/stl.hpp>

#include "anomaly_detection.hpp"

using anomaly_detection::Direction;

extern "C"
void Init_ext() {
  auto rb_mAnomalyDetection = Rice::define_module("AnomalyDetection");

  rb_mAnomalyDetection
    .define_singleton_function(
      "_detect",
      [](std::vector<float> series, int period, float k, float alpha, const std::string& direction, bool verbose) {
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

        auto res = anomaly_detection::params()
          .max_anoms(k)
          .alpha(alpha)
          .direction(dir)
          .verbose(verbose)
          .callback(rb_thread_check_ints)
          .fit(series, period);

        auto a = Rice::Array();
        for (auto v : res.anomalies) {
          a.push(v);
        }
        return a;
      });
}
