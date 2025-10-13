#pragma once

#include "light_sensor.h"
#include <cstdint>
#include <cmath>
#include <algorithm>

namespace LightSensor {

// Maximum buffer sizes for ESP32 memory constraints
static const size_t MAX_FILTER_WINDOW = 16;
static const size_t MAX_RECENT_VALUES = 20;

/**
 * @brief Signal processing configuration
 */
struct SignalConfig {
    // Filtering parameters
    uint8_t moving_average_window;
    float low_pass_cutoff;
    float high_pass_cutoff;
    bool enable_median_filter;
    uint8_t median_window;
    
    // Noise reduction
    float noise_threshold;
    bool enable_outlier_removal;
    float outlier_threshold;
    
    // Signal analysis
    bool enable_trend_detection;
    uint8_t trend_window;
    bool enable_peak_detection;
    float peak_threshold;
    
    // Adaptive filtering
    bool enable_adaptive_filter;
    float adaptation_rate;
    float noise_floor;
};

/**
 * @brief Signal analysis results
 */
struct SignalAnalysis {
    float filtered_value;
    float noise_level;
    float signal_to_noise_ratio;
    bool is_outlier;
    bool is_peak;
    float trend_slope;
    float trend_confidence;
    uint8_t quality_score;
};

/**
 * @brief Digital filter types
 */
enum class FilterType {
    MOVING_AVERAGE,
    LOW_PASS,
    HIGH_PASS,
    MEDIAN,
    ADAPTIVE
};

/**
 * @brief Moving average filter (fixed-size buffer)
 */
class MovingAverageFilter {
public:
    explicit MovingAverageFilter(uint8_t window_size);
    float process(float input);
    void reset();
    
private:
    uint8_t window_size_;
    float buffer_[MAX_FILTER_WINDOW];
    size_t buffer_index_;
    size_t buffer_count_;
    float sum_;
};

/**
 * @brief Low-pass filter
 */
class LowPassFilter {
public:
    LowPassFilter(float cutoff_freq, float sample_rate);
    float process(float input);
    void reset();
    
private:
    float alpha_;
    float prev_output_;
};

/**
 * @brief Median filter (fixed-size buffer)
 */
class MedianFilter {
public:
    explicit MedianFilter(uint8_t window_size);
    float process(float input);
    void reset();
    
private:
    uint8_t window_size_;
    float buffer_[MAX_FILTER_WINDOW];
    float sorted_buffer_[MAX_FILTER_WINDOW];
    size_t buffer_index_;
    size_t buffer_count_;
};

/**
 * @brief Adaptive filter
 */
class AdaptiveFilter {
public:
    AdaptiveFilter(float adaptation_rate, float noise_floor);
    float process(float input);
    void reset();
    void updateParameters(float adaptation_rate, float noise_floor);
    
private:
    float adaptation_rate_;
    float noise_floor_;
    float filter_coefficient_;
    float prev_output_;
    float error_variance_;
};

/**
 * @brief Trend analyzer result
 */
struct TrendResult {
    float slope;
    float confidence;
    bool is_increasing;
    bool is_decreasing;
};

/**
 * @brief Trend analyzer (fixed-size buffer)
 */
class TrendAnalyzer {
public:
    explicit TrendAnalyzer(uint8_t window_size = 10);
    TrendResult analyzeTrend(float value);
    void setWindowSize(uint8_t window_size);
    void reset();
    
private:
    uint8_t window_size_;
    float buffer_[MAX_FILTER_WINDOW];
    size_t buffer_index_;
    size_t buffer_count_;
};

/**
 * @brief Main signal processor for ESP32
 */
class SignalProcessor {
public:
    explicit SignalProcessor(const SignalConfig& config);
    ~SignalProcessor() = default;
    
    SignalAnalysis processReading(const SensorReading& reading);
    void configure(const SignalConfig& config);
    void reset();
    uint8_t getSignalQuality() const;
    float getNoiseLevel() const;
    void setFilterEnabled(FilterType filter_type, bool enable);
    
private:
    SignalConfig config_;
    
    // Fixed filter instances
    MovingAverageFilter ma_filter_;
    LowPassFilter lp_filter_;
    MedianFilter median_filter_;
    AdaptiveFilter adaptive_filter_;
    TrendAnalyzer trend_analyzer_;
    
    // Filter enable flags
    bool ma_enabled_;
    bool lp_enabled_;
    bool median_enabled_;
    bool adaptive_enabled_;
    
    // Recent values buffer
    float recent_values_[MAX_RECENT_VALUES];
    size_t recent_index_;
    size_t recent_count_;
    
    float noise_level_estimate_;
    uint8_t signal_quality_;
    
    // Peak detection state
    float prev_value_;
    bool rising_;
    
    void initializeFilters();
    float applyFilters(float input);
    void updateNoiseEstimate(float filtered_value, float raw_value);
    uint8_t calculateSignalQuality(const SignalAnalysis& analysis) const;
    bool isOutlier(float value) const;
    bool isPeak(float value);
    float calculateMean() const;
    float calculateStdDev(float mean) const;
};

}  // namespace LightSensor
