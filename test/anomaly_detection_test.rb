require_relative "test_helper"

class AnomalyDetectionTest < Minitest::Test
  def test_hash
    series = time_series
    assert_equal [9, 15, 26].map { |i| series.keys[0] + i }, AnomalyDetection.detect(series, period: 7, max_anoms: 0.2)
  end

  def test_array
    assert_equal [9, 15, 26], AnomalyDetection.detect(series, period: 7, max_anoms: 0.2)
  end

  def test_direction_pos
    assert_equal [9, 26], AnomalyDetection.detect(series, period: 7, direction: "pos")
  end

  def test_direction_neg
    assert_equal [15], AnomalyDetection.detect(series, period: 7, direction: "neg")
  end

  def test_alpha
    assert_equal [1, 4, 9, 15, 26], AnomalyDetection.detect(series, period: 7, max_anoms: 0.2, alpha: 0.5)
  end

  def test_nan
    series = [1] * 30
    series[15] = Float::NAN
    error = assert_raises(ArgumentError) do
      AnomalyDetection.detect(series, period: 7, max_anoms: 0.2)
    end
    assert_equal "series contains NANs", error.message
  end

  def test_empty_data
    error = assert_raises(ArgumentError) do
      AnomalyDetection.detect([], period: 7)
    end
    assert_equal "series must contain at least 2 periods", error.message
  end

  def test_direction_bad
    error = assert_raises(ArgumentError) do
      AnomalyDetection.detect(series, period: 7, direction: "bad")
    end
    assert_equal "direction must be pos, neg, or both", error.message
  end

  def test_max_anoms_zero
    assert_empty AnomalyDetection.detect(series, period: 7, max_anoms: 0)
  end

  def test_plot_hash
    today = Date.today
    series = self.series.map.with_index.to_h { |v, i| [today + i, v] }
    anomalies = AnomalyDetection.detect(series, period: 7, max_anoms: 0.2)
    assert_kind_of Vega::LiteChart, AnomalyDetection.plot(series, anomalies)
  end

  def test_plot_array
    anomalies = AnomalyDetection.detect(series, period: 7, max_anoms: 0.2)
    assert_kind_of Vega::LiteChart, AnomalyDetection.plot(series, anomalies)
  end

  def series
    [
      5.0, 9.0, 2.0, 9.0, 0.0, 6.0, 3.0, 8.0, 5.0, 18.0,
      7.0, 8.0, 8.0, 0.0, 2.0, -5.0, 0.0, 5.0, 6.0, 7.0,
      3.0, 6.0, 1.0, 4.0, 4.0, 4.0, 30.0, 7.0, 5.0, 8.0
    ]
  end

  def time_series
    today = Date.today
    self.series.map.with_index.to_h { |v, i| [today + i, v] }
  end
end
