#include "signal_processor.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <vector>
#include <deque>

namespace LightSensor {

// MovingAverageFilter Implementation
MovingAverageFilter::MovingAverageFilter(uint8_t window_size)
    : window_size_(window_size), sum_(0.0f) {
    // deque doesn't have reserve, but we can pre-allocate if needed
}

float MovingAverageFilter::process(float input) {
    buffer_.push_back(input);
    sum_ += input;
    
    if (buffer_.size() > window_size_) {
        sum_ -= buffer_.front();
        buffer_.pop_front();
    }
    
    return sum_ / buffer_.size();
}

void MovingAverageFilter::reset() {
    buffer_.clear();
    sum_ = 0.0f;
}

// LowPassFilter Implementation
LowPassFilter::LowPassFilter(float cutoff_freq, float sample_rate)
    : cutoff_freq_(cutoff_freq), sample_rate_(sample_rate), 
      prev_output_(0.0f) {
    
    // Calculate alpha for first-order low-pass filter
    float rc = 1.0f / (2.0f * M_PI * cutoff_freq_);
    float dt = 1.0f / sample_rate_;
    alpha_ = dt / (rc + dt);
}

float LowPassFilter::process(float input) {
    prev_output_ = alpha_ * input + (1.0f - alpha_) * prev_output_;
    return prev_output_;
}

void LowPassFilter::reset() {
    prev_output_ = 0.0f;
}

// MedianFilter Implementation
MedianFilter::MedianFilter(uint8_t window_size)
    : window_size_(window_size) {
    // deque doesn't have reserve, but we can pre-allocate if needed
    sorted_buffer_.reserve(window_size_);
}

float MedianFilter::process(float input) {
    buffer_.push_back(input);
    
    if (buffer_.size() > window_size_) {
        buffer_.pop_front();
    }
    
    if (buffer_.size() < 3) {
        return input; // Not enough data for median
    }
    
    // Copy to sorted buffer and sort
    sorted_buffer_.assign(buffer_.begin(), buffer_.end());
    std::sort(sorted_buffer_.begin(), sorted_buffer_.end());
    
    size_t size = sorted_buffer_.size();
    if (size % 2 == 0) {
        return (sorted_buffer_[size / 2 - 1] + sorted_buffer_[size / 2]) / 2.0f;
    } else {
        return sorted_buffer_[size / 2];
    }
}

void MedianFilter::reset() {
    buffer_.clear();
    sorted_buffer_.clear();
}

// AdaptiveFilter Implementation
AdaptiveFilter::AdaptiveFilter(float adaptation_rate, float noise_floor)
    : adaptation_rate_(adaptation_rate), noise_floor_(noise_floor),
      filter_coefficient_(0.5f), prev_output_(0.0f), error_variance_(0.0f) {
}

float AdaptiveFilter::process(float input) {
    float error = input - prev_output_;
    error_variance_ = (1.0f - adaptation_rate_) * error_variance_ + 
                     adaptation_rate_ * error * error;
    
    // Adapt filter coefficient based on signal characteristics
    if (error_variance_ > noise_floor_) {
        filter_coefficient_ = std::min(0.9f, filter_coefficient_ + adaptation_rate_ * 0.1f);
    } else {
        filter_coefficient_ = std::max(0.1f, filter_coefficient_ - adaptation_rate_ * 0.1f);
    }
    
    prev_output_ = filter_coefficient_ * input + (1.0f - filter_coefficient_) * prev_output_;
    return prev_output_;
}

void AdaptiveFilter::reset() {
    filter_coefficient_ = 0.5f;
    prev_output_ = 0.0f;
    error_variance_ = 0.0f;
}

void AdaptiveFilter::updateParameters(float adaptation_rate, float noise_floor) {
    adaptation_rate_ = adaptation_rate;
    noise_floor_ = noise_floor;
}

// OutlierDetector Implementation
OutlierDetector::OutlierDetector(float threshold)
    : threshold_(threshold) {
}

bool OutlierDetector::isOutlier(float value, const std::vector<float>& recent_values) {
    if (recent_values.size() < 3) {
        return false; // Not enough data for outlier detection
    }
    
    float mean = calculateMean(recent_values);
    float std_dev = calculateStdDev(recent_values, mean);
    
    if (std_dev == 0.0f) {
        return false; // No variation in data
    }
    
    float z_score = std::abs(value - mean) / std_dev;
    return z_score > threshold_;
}

void OutlierDetector::setThreshold(float threshold) {
    threshold_ = threshold;
}

float OutlierDetector::calculateMean(const std::vector<float>& values) const {
    if (values.empty()) {
        return 0.0f;
    }
    
    float sum = std::accumulate(values.begin(), values.end(), 0.0f);
    return sum / values.size();
}

float OutlierDetector::calculateStdDev(const std::vector<float>& values, float mean) const {
    if (values.size() < 2) {
        return 0.0f;
    }
    
    float sum_squared_diff = 0.0f;
    for (float value : values) {
        float diff = value - mean;
        sum_squared_diff += diff * diff;
    }
    
    return std::sqrt(sum_squared_diff / (values.size() - 1));
}

// PeakDetector Implementation
PeakDetector::PeakDetector(float threshold)
    : threshold_(threshold), prev_value_(0.0f), rising_(false) {
}

bool PeakDetector::isPeak(float value, const std::vector<float>& recent_values) {
    if (recent_values.size() < 3) {
        return false; // Not enough data for peak detection
    }
    
    bool current_rising = value > prev_value_;
    
    // Detect peak: was rising, now falling
    bool is_peak = rising_ && !current_rising;
    
    // Check if the change is significant enough
    if (is_peak) {
        float change = std::abs(value - prev_value_);
        float avg_value = std::accumulate(recent_values.begin(), recent_values.end(), 0.0f) / recent_values.size();
        is_peak = change > (avg_value * threshold_);
    }
    
    rising_ = current_rising;
    prev_value_ = value;
    
    return is_peak;
}

void PeakDetector::setThreshold(float threshold) {
    threshold_ = threshold;
}

// TrendAnalyzer Implementation
TrendAnalyzer::TrendAnalyzer(uint8_t window_size)
    : window_size_(window_size) {
    // deque doesn't have reserve, but we can pre-allocate if needed
}

TrendAnalyzer::TrendResult TrendAnalyzer::analyzeTrend(float value) {
    buffer_.push_back(value);
    
    if (buffer_.size() > window_size_) {
        buffer_.pop_front();
    }
    
    TrendResult result = {0.0f, 0.0f, false, false};
    
    if (buffer_.size() < 3) {
        return result; // Not enough data for trend analysis
    }
    
    // Prepare data for linear regression
    std::vector<float> x_values, y_values;
    for (size_t i = 0; i < buffer_.size(); ++i) {
        x_values.push_back(static_cast<float>(i));
        y_values.push_back(buffer_[i]);
    }
    
    auto [slope, correlation] = linearRegression(x_values, y_values);
    
    result.slope = slope;
    result.confidence = std::abs(correlation);
    result.is_increasing = slope > 0.0f;
    result.is_decreasing = slope < 0.0f;
    
    return result;
}

void TrendAnalyzer::setWindowSize(uint8_t window_size) {
    window_size_ = window_size;
    buffer_.clear();
}

std::pair<float, float> TrendAnalyzer::linearRegression(const std::vector<float>& x_values, 
                                                       const std::vector<float>& y_values) const {
    if (x_values.size() != y_values.size() || x_values.size() < 2) {
        return {0.0f, 0.0f};
    }
    
    size_t n = x_values.size();
    
    // Calculate means
    float x_mean = std::accumulate(x_values.begin(), x_values.end(), 0.0f) / n;
    float y_mean = std::accumulate(y_values.begin(), y_values.end(), 0.0f) / n;
    
    // Calculate slope and correlation
    float numerator = 0.0f;
    float x_denominator = 0.0f;
    float y_denominator = 0.0f;
    
    for (size_t i = 0; i < n; ++i) {
        float x_diff = x_values[i] - x_mean;
        float y_diff = y_values[i] - y_mean;
        
        numerator += x_diff * y_diff;
        x_denominator += x_diff * x_diff;
        y_denominator += y_diff * y_diff;
    }
    
    float slope = (x_denominator == 0.0f) ? 0.0f : numerator / x_denominator;
    float correlation = (x_denominator == 0.0f || y_denominator == 0.0f) ? 
                       0.0f : numerator / std::sqrt(x_denominator * y_denominator);
    
    return {slope, correlation};
}

// SignalProcessor Implementation
SignalProcessor::SignalProcessor(const SignalConfig& config)
    : config_(config), outlier_detector_(config.outlier_threshold),
      peak_detector_(config.peak_threshold), trend_analyzer_(config.trend_window),
      noise_level_estimate_(0.0f), signal_quality_(50) {
    
    // recent_values_ is a deque, no reserve needed
    initializeFilters();
}

SignalAnalysis SignalProcessor::processReading(const SensorReading& reading) {
    SignalAnalysis analysis;
    
    // Store recent values for analysis
    recent_values_.push_back(reading.lux_value);
    if (recent_values_.size() > 20) {
        recent_values_.pop_front();
    }
    
    // Apply filters
    analysis.filtered_value = applyFilters(reading.lux_value);
    
    // Update noise estimate
    updateNoiseEstimate(analysis.filtered_value, reading.lux_value);
    
    // Outlier detection
    if (config_.enable_outlier_removal) {
        analysis.is_outlier = outlier_detector_.isOutlier(reading.lux_value, 
            std::vector<float>(recent_values_.begin(), recent_values_.end()));
    } else {
        analysis.is_outlier = false;
    }
    
    // Peak detection
    if (config_.enable_peak_detection) {
        analysis.is_peak = peak_detector_.isPeak(reading.lux_value, 
            std::vector<float>(recent_values_.begin(), recent_values_.end()));
    } else {
        analysis.is_peak = false;
    }
    
    // Trend analysis
    if (config_.enable_trend_detection) {
        auto trend_result = trend_analyzer_.analyzeTrend(reading.lux_value);
        analysis.trend_slope = trend_result.slope;
        analysis.trend_confidence = trend_result.confidence;
    } else {
        analysis.trend_slope = 0.0f;
        analysis.trend_confidence = 0.0f;
    }
    
    // Calculate signal quality metrics
    analysis.noise_level = noise_level_estimate_;
    analysis.signal_to_noise_ratio = (analysis.filtered_value > 0.0f) ? 
        analysis.filtered_value / std::max(noise_level_estimate_, 0.001f) : 0.0f;
    
    analysis.quality_score = calculateSignalQuality(analysis);
    signal_quality_ = analysis.quality_score;
    
    return analysis;
}

void SignalProcessor::configure(const SignalConfig& config) {
    config_ = config;
    outlier_detector_.setThreshold(config.outlier_threshold);
    peak_detector_.setThreshold(config.peak_threshold);
    trend_analyzer_.setWindowSize(config.trend_window);
    
    reset();
    initializeFilters();
}

void SignalProcessor::reset() {
    for (auto& filter : filters_) {
        filter->reset();
    }
    
    recent_values_.clear();
    noise_level_estimate_ = 0.0f;
    signal_quality_ = 50;
}

uint8_t SignalProcessor::getSignalQuality() const {
    return signal_quality_;
}

float SignalProcessor::getNoiseLevel() const {
    return noise_level_estimate_;
}

void SignalProcessor::setFilterEnabled(FilterType filter_type, bool enable) {
    // This would require modifying the filter application logic
    // For now, we'll implement a simple approach
    // In a more sophisticated implementation, you'd track which filters are enabled
}

void SignalProcessor::initializeFilters() {
    filters_.clear();
    
    // Add moving average filter
    if (config_.moving_average_window > 1) {
        filters_.push_back(std::make_unique<MovingAverageFilter>(config_.moving_average_window));
    }
    
    // Add median filter
    if (config_.enable_median_filter && config_.median_window > 1) {
        filters_.push_back(std::make_unique<MedianFilter>(config_.median_window));
    }
    
    // Add low-pass filter
    if (config_.low_pass_cutoff > 0.0f) {
        // Assuming 1Hz sample rate for now
        filters_.push_back(std::make_unique<LowPassFilter>(config_.low_pass_cutoff, 1.0f));
    }
    
    // Add adaptive filter
    if (config_.enable_adaptive_filter) {
        filters_.push_back(std::make_unique<AdaptiveFilter>(config_.adaptation_rate, config_.noise_floor));
    }
}

float SignalProcessor::applyFilters(float input) {
    float filtered_value = input;
    
    for (auto& filter : filters_) {
        filtered_value = filter->process(filtered_value);
    }
    
    return filtered_value;
}

void SignalProcessor::updateNoiseEstimate(float filtered_value, float raw_value) {
    // Simple noise estimation based on difference between raw and filtered
    float noise = std::abs(raw_value - filtered_value);
    
    // Update noise level estimate using exponential moving average
    float alpha = 0.1f; // Learning rate
    noise_level_estimate_ = (1.0f - alpha) * noise_level_estimate_ + alpha * noise;
}

uint8_t SignalProcessor::calculateSignalQuality(const SignalAnalysis& analysis) const {
    uint8_t quality = 100;
    
    // Reduce quality based on noise level
    if (analysis.signal_to_noise_ratio < 1.0f) {
        quality -= 30;
    } else if (analysis.signal_to_noise_ratio < 2.0f) {
        quality -= 15;
    }
    
    // Reduce quality if outlier detected
    if (analysis.is_outlier) {
        quality -= 20;
    }
    
    // Reduce quality based on trend confidence
    if (analysis.trend_confidence < 0.5f) {
        quality -= 10;
    }
    
    // Ensure quality is within bounds
    return std::max(0, std::min(100, static_cast<int>(quality)));
}

} // namespace LightSensor
