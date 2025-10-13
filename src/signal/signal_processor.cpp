#include "signal_processor.h"
#include <Arduino.h>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace LightSensor {

// MovingAverageFilter Implementation
MovingAverageFilter::MovingAverageFilter(uint8_t window_size)
    : window_size_(window_size < MAX_FILTER_WINDOW ? window_size : MAX_FILTER_WINDOW),
      buffer_index_(0), buffer_count_(0), sum_(0.0f) {
    for (size_t i = 0; i < MAX_FILTER_WINDOW; ++i) {
        buffer_[i] = 0.0f;
    }
}

float MovingAverageFilter::process(float input) {
    // Remove old value from sum if buffer is full
    if (buffer_count_ >= window_size_) {
        sum_ -= buffer_[buffer_index_];
    }
    
    // Add new value
    buffer_[buffer_index_] = input;
    sum_ += input;
    
    buffer_index_ = (buffer_index_ + 1) % window_size_;
    if (buffer_count_ < window_size_) {
        buffer_count_++;
    }
    
    return sum_ / buffer_count_;
}

void MovingAverageFilter::reset() {
    for (size_t i = 0; i < MAX_FILTER_WINDOW; ++i) {
        buffer_[i] = 0.0f;
    }
    buffer_index_ = 0;
    buffer_count_ = 0;
    sum_ = 0.0f;
}

// LowPassFilter Implementation
LowPassFilter::LowPassFilter(float cutoff_freq, float sample_rate)
    : prev_output_(0.0f) {
    float rc = 1.0f / (2.0f * M_PI * cutoff_freq);
    float dt = 1.0f / sample_rate;
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
    : window_size_(window_size < MAX_FILTER_WINDOW ? window_size : MAX_FILTER_WINDOW),
      buffer_index_(0), buffer_count_(0) {
    for (size_t i = 0; i < MAX_FILTER_WINDOW; ++i) {
        buffer_[i] = 0.0f;
        sorted_buffer_[i] = 0.0f;
    }
}

float MedianFilter::process(float input) {
    buffer_[buffer_index_] = input;
    buffer_index_ = (buffer_index_ + 1) % window_size_;
    if (buffer_count_ < window_size_) {
        buffer_count_++;
    }
    
    if (buffer_count_ < 3) {
        return input;
    }
    
    // Copy to sorted buffer
    for (size_t i = 0; i < buffer_count_; ++i) {
        sorted_buffer_[i] = buffer_[i];
    }
    
    // Simple bubble sort (efficient for small arrays)
    for (size_t i = 0; i < buffer_count_ - 1; ++i) {
        for (size_t j = 0; j < buffer_count_ - i - 1; ++j) {
            if (sorted_buffer_[j] > sorted_buffer_[j + 1]) {
                float temp = sorted_buffer_[j];
                sorted_buffer_[j] = sorted_buffer_[j + 1];
                sorted_buffer_[j + 1] = temp;
            }
        }
    }
    
    if (buffer_count_ % 2 == 0) {
        return (sorted_buffer_[buffer_count_ / 2 - 1] + sorted_buffer_[buffer_count_ / 2]) / 2.0f;
    } else {
        return sorted_buffer_[buffer_count_ / 2];
    }
}

void MedianFilter::reset() {
    for (size_t i = 0; i < MAX_FILTER_WINDOW; ++i) {
        buffer_[i] = 0.0f;
        sorted_buffer_[i] = 0.0f;
    }
    buffer_index_ = 0;
    buffer_count_ = 0;
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

// TrendAnalyzer Implementation
TrendAnalyzer::TrendAnalyzer(uint8_t window_size)
    : window_size_(window_size < MAX_FILTER_WINDOW ? window_size : MAX_FILTER_WINDOW),
      buffer_index_(0), buffer_count_(0) {
    for (size_t i = 0; i < MAX_FILTER_WINDOW; ++i) {
        buffer_[i] = 0.0f;
    }
}

TrendResult TrendAnalyzer::analyzeTrend(float value) {
    buffer_[buffer_index_] = value;
    buffer_index_ = (buffer_index_ + 1) % window_size_;
    if (buffer_count_ < window_size_) {
        buffer_count_++;
    }
    
    TrendResult result = {0.0f, 0.0f, false, false};
    
    if (buffer_count_ < 3) {
        return result;
    }
    
    // Calculate means
    float x_mean = (buffer_count_ - 1) / 2.0f;
    float y_mean = 0.0f;
    for (size_t i = 0; i < buffer_count_; ++i) {
        y_mean += buffer_[i];
    }
    y_mean /= buffer_count_;
    
    // Calculate slope and correlation using linear regression
    float numerator = 0.0f;
    float x_denominator = 0.0f;
    float y_denominator = 0.0f;
    
    for (size_t i = 0; i < buffer_count_; ++i) {
        float x_diff = static_cast<float>(i) - x_mean;
        float y_diff = buffer_[(buffer_index_ + i) % window_size_] - y_mean;
        
        numerator += x_diff * y_diff;
        x_denominator += x_diff * x_diff;
        y_denominator += y_diff * y_diff;
    }
    
    if (x_denominator > 0.001f) {
        result.slope = numerator / x_denominator;
        
        if (y_denominator > 0.001f) {
            result.confidence = fabsf(numerator / sqrtf(x_denominator * y_denominator));
        }
    }
    
    result.is_increasing = result.slope > 0.0f && result.confidence > 0.5f;
    result.is_decreasing = result.slope < 0.0f && result.confidence > 0.5f;
    
    return result;
}

void TrendAnalyzer::setWindowSize(uint8_t window_size) {
    window_size_ = window_size < MAX_FILTER_WINDOW ? window_size : MAX_FILTER_WINDOW;
    reset();
}

void TrendAnalyzer::reset() {
    for (size_t i = 0; i < MAX_FILTER_WINDOW; ++i) {
        buffer_[i] = 0.0f;
    }
    buffer_index_ = 0;
    buffer_count_ = 0;
}

// SignalProcessor Implementation
SignalProcessor::SignalProcessor(const SignalConfig& config)
    : config_(config),
      ma_filter_(config.moving_average_window),
      lp_filter_(config.low_pass_cutoff > 0 ? config.low_pass_cutoff : 0.5f, 1.0f),
      median_filter_(config.median_window),
      adaptive_filter_(config.adaptation_rate, config.noise_floor),
      trend_analyzer_(config.trend_window),
      ma_enabled_(config.moving_average_window > 1),
      lp_enabled_(config.low_pass_cutoff > 0),
      median_enabled_(config.enable_median_filter),
      adaptive_enabled_(config.enable_adaptive_filter),
      recent_index_(0), recent_count_(0),
      noise_level_estimate_(0.0f), signal_quality_(50),
      prev_value_(0.0f), rising_(false) {
    
    for (size_t i = 0; i < MAX_RECENT_VALUES; ++i) {
        recent_values_[i] = 0.0f;
    }
}

SignalAnalysis SignalProcessor::processReading(const SensorReading& reading) {
    SignalAnalysis analysis;
    
    // Store recent values
    recent_values_[recent_index_] = reading.lux_value;
    recent_index_ = (recent_index_ + 1) % MAX_RECENT_VALUES;
    if (recent_count_ < MAX_RECENT_VALUES) {
        recent_count_++;
    }
    
    // Apply filters
    analysis.filtered_value = applyFilters(reading.lux_value);
    
    // Update noise estimate
    updateNoiseEstimate(analysis.filtered_value, reading.lux_value);
    
    // Outlier detection
    analysis.is_outlier = config_.enable_outlier_removal && isOutlier(reading.lux_value);
    
    // Peak detection
    analysis.is_peak = config_.enable_peak_detection && isPeak(reading.lux_value);
    
    // Trend analysis
    if (config_.enable_trend_detection) {
        TrendResult trend = trend_analyzer_.analyzeTrend(reading.lux_value);
        analysis.trend_slope = trend.slope;
        analysis.trend_confidence = trend.confidence;
    } else {
        analysis.trend_slope = 0.0f;
        analysis.trend_confidence = 0.0f;
    }
    
    // Calculate signal quality metrics
    analysis.noise_level = noise_level_estimate_;
    analysis.signal_to_noise_ratio = (analysis.filtered_value > 0.001f) ? 
        analysis.filtered_value / std::max(noise_level_estimate_, 0.001f) : 0.0f;
    
    analysis.quality_score = calculateSignalQuality(analysis);
    signal_quality_ = analysis.quality_score;
    
    return analysis;
}

void SignalProcessor::configure(const SignalConfig& config) {
    config_ = config;
    
    ma_enabled_ = config.moving_average_window > 1;
    lp_enabled_ = config.low_pass_cutoff > 0;
    median_enabled_ = config.enable_median_filter;
    adaptive_enabled_ = config.enable_adaptive_filter;
    
    trend_analyzer_.setWindowSize(config.trend_window);
    adaptive_filter_.updateParameters(config.adaptation_rate, config.noise_floor);
    
    reset();
}

void SignalProcessor::reset() {
    ma_filter_.reset();
    lp_filter_.reset();
    median_filter_.reset();
    adaptive_filter_.reset();
    trend_analyzer_.reset();
    
    for (size_t i = 0; i < MAX_RECENT_VALUES; ++i) {
        recent_values_[i] = 0.0f;
    }
    recent_index_ = 0;
    recent_count_ = 0;
    
    noise_level_estimate_ = 0.0f;
    signal_quality_ = 50;
    prev_value_ = 0.0f;
    rising_ = false;
}

uint8_t SignalProcessor::getSignalQuality() const {
    return signal_quality_;
}

float SignalProcessor::getNoiseLevel() const {
    return noise_level_estimate_;
}

void SignalProcessor::setFilterEnabled(FilterType filter_type, bool enable) {
    switch (filter_type) {
        case FilterType::MOVING_AVERAGE:
            ma_enabled_ = enable;
            break;
        case FilterType::LOW_PASS:
            lp_enabled_ = enable;
            break;
        case FilterType::MEDIAN:
            median_enabled_ = enable;
            break;
        case FilterType::ADAPTIVE:
            adaptive_enabled_ = enable;
            break;
        default:
            break;
    }
}

void SignalProcessor::initializeFilters() {
    ma_filter_.reset();
    lp_filter_.reset();
    median_filter_.reset();
    adaptive_filter_.reset();
}

float SignalProcessor::applyFilters(float input) {
    float value = input;
    
    if (ma_enabled_) {
        value = ma_filter_.process(value);
    }
    
    if (median_enabled_) {
        value = median_filter_.process(value);
    }
    
    if (lp_enabled_) {
        value = lp_filter_.process(value);
    }
    
    if (adaptive_enabled_) {
        value = adaptive_filter_.process(value);
    }
    
    return value;
}

void SignalProcessor::updateNoiseEstimate(float filtered_value, float raw_value) {
    float noise = fabsf(raw_value - filtered_value);
    float alpha = 0.1f;
    noise_level_estimate_ = (1.0f - alpha) * noise_level_estimate_ + alpha * noise;
}

uint8_t SignalProcessor::calculateSignalQuality(const SignalAnalysis& analysis) const {
    int quality = 100;
    
    // Reduce quality based on SNR
    if (analysis.signal_to_noise_ratio < 1.0f) {
        quality -= 30;
    } else if (analysis.signal_to_noise_ratio < 2.0f) {
        quality -= 15;
    }
    
    // Reduce quality if outlier
    if (analysis.is_outlier) {
        quality -= 20;
    }
    
    // Reduce quality based on trend confidence
    if (analysis.trend_confidence < 0.5f) {
        quality -= 10;
    }
    
    return static_cast<uint8_t>(std::max(0, std::min(100, quality)));
}

bool SignalProcessor::isOutlier(float value) const {
    if (recent_count_ < 3) {
        return false;
    }
    
    float mean = calculateMean();
    float std_dev = calculateStdDev(mean);
    
    if (std_dev < 0.001f) {
        return false;
    }
    
    float z_score = fabsf(value - mean) / std_dev;
    return z_score > config_.outlier_threshold;
}

bool SignalProcessor::isPeak(float value) {
    bool current_rising = value > prev_value_;
    bool is_peak = rising_ && !current_rising;
    
    if (is_peak && recent_count_ > 0) {
        float avg = calculateMean();
        float change = fabsf(value - prev_value_);
        is_peak = change > (avg * config_.peak_threshold);
    }
    
    rising_ = current_rising;
    prev_value_ = value;
    
    return is_peak;
}

float SignalProcessor::calculateMean() const {
    if (recent_count_ == 0) {
        return 0.0f;
    }
    
    float sum = 0.0f;
    for (size_t i = 0; i < recent_count_; ++i) {
        sum += recent_values_[i];
    }
    return sum / recent_count_;
}

float SignalProcessor::calculateStdDev(float mean) const {
    if (recent_count_ < 2) {
        return 0.0f;
    }
    
    float sum_squared_diff = 0.0f;
    for (size_t i = 0; i < recent_count_; ++i) {
        float diff = recent_values_[i] - mean;
        sum_squared_diff += diff * diff;
    }
    
    return sqrtf(sum_squared_diff / (recent_count_ - 1));
}

}  // namespace LightSensor
