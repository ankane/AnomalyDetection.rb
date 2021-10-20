# extensions
require "anomaly_detection/ext"

# modules
require "anomaly_detection/version"

module AnomalyDetection
  class << self
    def detect(series, period:, max_anoms: 0.1, alpha: 0.05, direction: "both", plot: false, verbose: false)
      raise ArgumentError, "series must contain at least 2 periods" if series.size < period * 2

      if series.is_a?(Hash)
        sorted = series.sort_by { |k, _| k }
        x = sorted.map(&:last)
      else
        x = series
      end

      res = _detect(x, period, max_anoms, alpha, direction, verbose)
      res.map! { |i| sorted[i][0] } if series.is_a?(Hash)
      res
    end

    # TODO add tooltips
    def plot(series, anomalies)
      require "vega"

      data =
        if series.is_a?(Hash)
          series.map { |k, v| {x: iso8601(k), y: v, anomaly: anomalies.include?(k)} }
        else
          series.map.with_index { |v, i| {x: i, y: v, anomaly: anomalies.include?(i)} }
        end

      if series.is_a?(Hash)
        x = {field: "x", type: "temporal"}
        x["scale"] = {type: "utc"} if series.keys.first.is_a?(Date)
      else
        x = {field: "x", type: "quantitative"}
      end

      Vega.lite
        .data(data)
        .layer([
          {
            mark: {type: "line"},
            encoding: {
              x: x,
              y: {field: "y", type: "quantitative", scale: {zero: false}},
              color: {value: "#fa9088"}
            }
          },
          {
            transform: [{"filter": "datum.anomaly == true"}],
            mark: {type: "point", size: 200},
            encoding: {
              x: x,
              y: {field: "y", type: "quantitative"},
              color: {value: "#19c7ca"}
            }
          }
        ])
        .config(axis: {title: nil, labelFontSize: 12})
    end

    private

    def iso8601(v)
      if v.is_a?(Date)
        v.strftime("%Y-%m-%d")
      else
        v.strftime("%Y-%m-%dT%H:%M:%S.%L%z")
      end
    end
  end
end
