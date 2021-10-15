library(AnomalyDetection)

x <- c(
  5.0, 9.0, 2.0, 9.0, 0.0, 6.0, 3.0, 8.0, 5.0, 18.0,
  7.0, 8.0, 8.0, 0.0, 2.0, -5.0, 0.0, 5.0, 6.0, 7.0,
  3.0, 6.0, 1.0, 4.0, 4.0, 4.0, 30.0, 7.0, 5.0, 8.0
)

print("direction both")
AnomalyDetectionVec(x, max_anoms=0.2, period=7, direction="both", only_last=FALSE, plot=FALSE)$anoms$index - 1

print("direction pos")
AnomalyDetectionVec(x, max_anoms=0.2, period=7, direction="pos", only_last=FALSE, plot=FALSE)$anoms$index - 1

print("direction neg")
AnomalyDetectionVec(x, max_anoms=0.2, period=7, direction="neg", only_last=FALSE, plot=FALSE)$anoms$index - 1

print("alpha")
AnomalyDetectionVec(x, max_anoms=0.2, period=7, direction="both", alpha=0.5, only_last=FALSE, plot=FALSE)$anoms$index - 1
