# extensions
require_relative "anomaly_detection/ext"

# modules
require_relative "anomaly_detection/version"

module AnomalyDetection
  class << self
    def detect(series, period:, max_anoms: 0.1, alpha: 0.05, direction: "both", plot: false, verbose: false)
      if period == :auto
        period = determine_period(series)
        puts "Set period to #{period}" if verbose
      elsif period.nil?
        period = 1
      end

      raise ArgumentError, "series must contain at least 2 periods" if series.size < period * 2

      if series.is_a?(Hash)
        sorted = series.sort_by { |k, _| k }
        x = sorted.map(&:last)
      else
        x = series
      end

      # flush Ruby output since std::endl flushes C++ output
      $stdout.flush if verbose

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

    # determine period based on time keys (experimental)
    # in future, could use an approach that looks at values
    # like https://stats.stackexchange.com/a/1214
    def determine_period(series)
      unless series.is_a?(Hash)
        raise ArgumentError, "series must be a hash for :auto period"
      end

      times = series.keys.map(&:to_time)

      second = times.all? { |t| t.nsec == 0 }
      minute = second && times.all? { |t| t.sec == 0 }
      hour = minute && times.all? { |t| t.min == 0 }
      day = hour && times.all? { |t| t.hour == 0 }
      week = day && times.map { |k| k.wday }.uniq.size == 1
      month = day && times.all? { |k| k.day == 1 }
      quarter = month && times.all? { |k| k.month % 3 == 1 }
      year = quarter && times.all? { |k| k.month == 1 }

      period =
        if year
          1
        elsif quarter
          4
        elsif month
          12
        elsif week
          52
        elsif day
          7
        elsif hour
          24 # or 24 * 7
        elsif minute
          60 # or 60 * 24
        elsif second
          60 # or 60 * 60
        end

      if series.size < period * 2
        1
      else
        period
      end
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
