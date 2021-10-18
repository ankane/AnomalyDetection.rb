#include "anomaly_detection.hpp"
#include <rice/rice.hpp>
#include <rice/stl.hpp>

extern "C"
void Init_ext() {
  auto rb_mAnomalyDetection = Rice::define_module("AnomalyDetection");

  rb_mAnomalyDetection
    .define_singleton_function(
      "_detect",
      [](std::vector<float> x, int period, float k, float alpha, const std::string& direction) {
        auto res = anomaly_detection::anomalies(x, period, k, alpha, direction);

        auto a = Rice::Array();
        for (auto v : res) {
          a.push(v);
        }
        return a;
      });
}
