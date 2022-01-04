# AnomalyDetection.rb

:fire: Time series [AnomalyDetection](https://github.com/twitter/AnomalyDetection) for Ruby

Learn [how it works](https://blog.twitter.com/engineering/en_us/a/2015/introducing-practical-and-robust-anomaly-detection-in-a-time-series)

[![Build Status](https://github.com/ankane/AnomalyDetection.rb/workflows/build/badge.svg?branch=master)](https://github.com/ankane/AnomalyDetection.rb/actions)

## Installation

Add this line to your application’s Gemfile:

```ruby
gem "anomaly_detection"
```

## Getting Started

Detect anomalies in a time series

```ruby
series = {
  Date.parse("2020-01-01") => 100,
  Date.parse("2020-01-02") => 150,
  Date.parse("2020-01-03") => 136,
  # ...
}

AnomalyDetection.detect(series, period: 7)
```

Works great with [Groupdate](https://github.com/ankane/groupdate)

```ruby
series = User.group_by_day(:created_at).count
AnomalyDetection.detect(series, period: 7)
```

Series can also be an array without times (the index is returned)

```ruby
series = [100, 150, 136, ...]
AnomalyDetection.detect(series, period: 7)
```

## Options

Pass options

```ruby
AnomalyDetection.detect(
  series,
  period: 7,            # number of observations in a single period
  alpha: 0.05,          # level of statistical significance
  max_anoms: 0.1,       # maximum number of anomalies as percent of data
  direction: "both",    # pos, neg, or both
  verbose: false        # show progress
)
```

## Plotting

Add [Vega](https://github.com/ankane/vega) to your application’s Gemfile:

```ruby
gem "vega"
```

And use:

```ruby
AnomalyDetection.plot(series, anomalies)
```

## Credits

This library was ported from the [AnomalyDetection](https://github.com/twitter/AnomalyDetection) R package and is available under the same license. It uses [stl-cpp](https://github.com/ankane/stl-cpp) for seasonal-trend decomposition and [dist.h](https://github.com/ankane/dist.h) for the quantile function.

## References

- [Automatic Anomaly Detection in the Cloud Via Statistical Learning](https://arxiv.org/abs/1704.07706)

## History

View the [changelog](https://github.com/ankane/AnomalyDetection.rb/blob/master/CHANGELOG.md)

## Contributing

Everyone is encouraged to help improve this project. Here are a few ways you can help:

- [Report bugs](https://github.com/ankane/AnomalyDetection.rb/issues)
- Fix bugs and [submit pull requests](https://github.com/ankane/AnomalyDetection.rb/pulls)
- Write, clarify, or fix documentation
- Suggest or add new features

To get started with development:

```sh
git clone https://github.com/ankane/AnomalyDetection.rb.git
cd AnomalyDetection.rb
bundle install
bundle exec rake compile
bundle exec rake test
```
