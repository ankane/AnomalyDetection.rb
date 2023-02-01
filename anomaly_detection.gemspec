require_relative "lib/anomaly_detection/version"

Gem::Specification.new do |spec|
  spec.name          = "anomaly_detection"
  spec.version       = AnomalyDetection::VERSION
  spec.summary       = "Time series anomaly detection for Ruby"
  spec.homepage      = "https://github.com/ankane/AnomalyDetection.rb"
  spec.license       = "GPL-3.0-or-later"

  spec.author        = "Andrew Kane"
  spec.email         = "andrew@ankane.org"

  spec.files         = Dir["*.{md,txt}", "{ext,lib,licenses}/**/*"]
  spec.require_path  = "lib"
  spec.extensions    = ["ext/anomaly_detection/extconf.rb"]

  spec.required_ruby_version = ">= 2.7"

  spec.add_dependency "rice", ">= 4.0.2"
end
