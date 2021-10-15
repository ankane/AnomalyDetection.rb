# extensions
require "anomaly_detection/ext"

# modules
require "anomaly_detection/version"

module AnomalyDetection
  def self.detect(series, period:, max_anoms: 0.1, alpha: 0.05, direction: "both")
    raise ArgumentError, "series must contain at least 2 periods" if series.size < period * 2

    if series.is_a?(Hash)
      sorted = series.sort_by { |k, _| k }
      x = sorted.map(&:last)
    else
      x = series
    end

    res = _detect(x, period, max_anoms, alpha, direction)
    res.map! { |i| sorted[i][0] } if series.is_a?(Hash)
    res
  end
end
