// rice
#include <rice/rice.hpp>
#include <rice/stl.hpp>

std::vector<size_t> anomalies(const std::vector<float>& x, int period, float k, float alpha, const std::string& direction);

extern "C"
void Init_ext() {
  auto rb_mAnomalyDetection = Rice::define_module("AnomalyDetection");

  rb_mAnomalyDetection
    .define_singleton_function(
      "_detect",
      [](std::vector<float> x, int period, float k, float alpha, const std::string& direction) {
        auto res = anomalies(x, period, k, alpha, direction);

        auto a = Rice::Array();
        for (auto v : res) {
          a.push(v);
        }
        return a;
      });
}
