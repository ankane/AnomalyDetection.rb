# extensions
require_relative "anomaly_detection/ext"

# modules
require_relative "anomaly_detection/version"

module AnomalyDetection
  class << self
    def detect(series, period:, max_anoms: 0.1, alpha: 0.05, direction: "both", plot: false, verbose: false)
      period = determine_period(series) if period == :auto

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

    # determine period based on time keys
    # in future, could use an approach that looks at values
    # like https://stats.stackexchange.com/a/1214
    def determine_period(series)
      unless series.is_a?(Hash)
        raise ArgumentError, "series must be a hash for :auto period"
      end

      times = series.keys.map(&:to_time)

      day = times.all? { |t| t.hour == 0 && t.min == 0 && t.sec == 0 && t.nsec == 0 }

      period =
        if day
          7
        else
          1
        end

      if series.size < period * 2
        1
      else
        period
      end
    end
  end
end
