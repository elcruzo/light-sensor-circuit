#pragma once

#include "light_sensor.h"
#include <vector>
#include <deque>
#include <array>
#include <algorithm>
#include <cmath>

namespace LightSensor {

/**
 * @brief Signal processing configuration
 */
struct SignalConfig {
    // Filtering parameters
    uint8_t moving_average_window;    // Moving average window size
    float low_pass_cutoff;            // Low-pass filter cutoff frequency (Hz)
    float high_pass_cutoff;           // High-pass filter cutoff frequency (Hz)
    bool enable_median_filter;        // Enable median filtering
    uint8_t median_window;            // Median filter window size
    
    // Noise reduction
    float noise_threshold;            // Noise threshold for outlier detection
    bool enable_outlier_removal;      // Enable outlier removal
    float outlier_threshold;          // Outlier detection threshold (standard deviations)
    
    // Signal analysis
    bool enable_trend_detection;      // Enable trend detection
    uint8_t trend_window;             // Window size for trend analysis
    bool enable_peak_detection;       // Enable peak detection
    float peak_threshold;             // Peak detection threshold
    
    // Adaptive filtering
    bool enable_adaptive_filter;      // Enable adaptive filtering
    float adaptation_rate;            // Filter adaptation rate
    float noise_floor;                // Estimated noise floor
};

/**
 * @brief Signal analysis results
 */
struct SignalAnalysis {
    float filtered_value;             // Filtered signal value
    float noise_level;                // Estimated noise level
    float signal_to_noise_ratio;      // Signal-to-noise ratio
    bool is_outlier;                  // Outlier detection flag
    bool is_peak;                     // Peak detection flag
    float trend_slope;                // Trend slope (change per sample)
    float trend_confidence;           // Trend confidence (0-1)
    uint8_t quality_score;            // Overall signal quality (0-100)
};

/**
 * @brief Digital filter types
 */
enum class FilterType {
    MOVING_AVERAGE,       // Simple moving average
    LOW_PASS,            // Low-pass filter
    HIGH_PASS,           // High-pass filter
    MEDIAN,              // Median filter
    ADAPTIVE             // Adaptive filter
};

/**
 * @brief Abstract base class for digital filters
 */
class IDigitalFilter {
public:
    virtual ~IDigitalFilter() = default;
    
    /**
     * @brief Process input sample
     * @param input Input sample
     * @return Filtered output
     */
    virtual float process(float input) = 0;
    
    /**
     * @brief Reset filter state
     */
    virtual void reset() = 0;
    
    /**
     * @brief Get filter type
     * @return Filter type
     */
    virtual FilterType getType() const = 0;
};

/**
 * @brief Moving average filter implementation
 */
class MovingAverageFilter : public IDigitalFilter {
public:
    explicit MovingAverageFilter(uint8_t window_size);
    ~MovingAverageFilter() override = default;
    
    float process(float input) override;
    void reset() override;
    FilterType getType() const override { return FilterType::MOVING_AVERAGE; }
    
private:
    uint8_t window_size_;
    std::deque<float> buffer_;
    float sum_;
};

/**
 * @brief Low-pass filter implementation
 */
class LowPassFilter : public IDigitalFilter {
public:
    LowPassFilter(float cutoff_freq, float sample_rate);
    ~LowPassFilter() override = default;
    
    float process(float input) override;
    void reset() override;
    FilterType getType() const override { return FilterType::LOW_PASS; }
    
private:
    float cutoff_freq_;
    float sample_rate_;
    float alpha_;
    float prev_output_;
};

/**
 * @brief Median filter implementation
 */
class MedianFilter : public IDigitalFilter {
public:
    explicit MedianFilter(uint8_t window_size);
    ~MedianFilter() override = default;
    
    float process(float input) override;
    void reset() override;
    FilterType getType() const override { return FilterType::MEDIAN; }
    
private:
    uint8_t window_size_;
    std::deque<float> buffer_;
    std::vector<float> sorted_buffer_;
};

/**
 * @brief Adaptive filter implementation
 */
class AdaptiveFilter : public IDigitalFilter {
public:
    AdaptiveFilter(float adaptation_rate, float noise_floor);
    ~AdaptiveFilter() override = default;
    
    float process(float input) override;
    void reset() override;
    FilterType getType() const override { return FilterType::ADAPTIVE; }
    
    /**
     * @brief Update adaptation parameters
     * @param adaptation_rate New adaptation rate
     * @param noise_floor New noise floor estimate
     */
    void updateParameters(float adaptation_rate, float noise_floor);
    
private:
    float adaptation_rate_;
    float noise_floor_;
    float filter_coefficient_;
    float prev_output_;
    float error_variance_;
};

/**
 * @brief Outlier detector
 */
class OutlierDetector {
public:
    explicit OutlierDetector(float threshold = 2.0f);
    
    /**
     * @brief Check if value is an outlier
     * @param value Value to check
     * @param recent_values Recent values for context
     * @return true if value is an outlier
     */
    bool isOutlier(float value, const std::vector<float>& recent_values);
    
    /**
     * @brief Update threshold
     * @param threshold New threshold in standard deviations
     */
    void setThreshold(float threshold);
    
private:
    float threshold_;
    
    /**
     * @brief Calculate mean of values
     * @param values Values to calculate mean of
     * @return Mean value
     */
    float calculateMean(const std::vector<float>& values) const;
    
    /**
     * @brief Calculate standard deviation of values
     * @param values Values to calculate standard deviation of
     * @param mean Mean value
     * @return Standard deviation
     */
    float calculateStdDev(const std::vector<float>& values, float mean) const;
};

/**
 * @brief Peak detector
 */
class PeakDetector {
public:
    explicit PeakDetector(float threshold = 0.1f);
    
    /**
     * @brief Check if value represents a peak
     * @param value Current value
     * @param recent_values Recent values for context
     * @return true if value is a peak
     */
    bool isPeak(float value, const std::vector<float>& recent_values);
    
    /**
     * @brief Update threshold
     * @param threshold New peak detection threshold
     */
    void setThreshold(float threshold);
    
private:
    float threshold_;
    float prev_value_;
    bool rising_;
};

/**
 * @brief Trend analyzer
 */
class TrendAnalyzer {
public:
    explicit TrendAnalyzer(uint8_t window_size = 10);
    
    /**
     * @brief Analyze trend in data
     * @param value Current value
     * @return Trend analysis results
     */
    struct TrendResult {
        float slope;        // Trend slope
        float confidence;   // Trend confidence (0-1)
        bool is_increasing; // Is trend increasing
        bool is_decreasing; // Is trend decreasing
    };
    
    TrendResult analyzeTrend(float value);
    
    /**
     * @brief Update window size
     * @param window_size New window size
     */
    void setWindowSize(uint8_t window_size);
    
private:
    uint8_t window_size_;
    std::deque<float> buffer_;
    
    /**
     * @brief Calculate linear regression
     * @param x_values X values
     * @param y_values Y values
     * @return Slope and correlation coefficient
     */
    std::pair<float, float> linearRegression(const std::vector<float>& x_values, 
                                            const std::vector<float>& y_values) const;
};

/**
 * @brief Main signal processor class
 */
class SignalProcessor {
public:
    explicit SignalProcessor(const SignalConfig& config);
    ~SignalProcessor() = default;
    
    /**
     * @brief Process sensor reading
     * @param reading Input reading
     * @return Processed analysis results
     */
    SignalAnalysis processReading(const SensorReading& reading);
    
    /**
     * @brief Configure signal processing
     * @param config New configuration
     */
    void configure(const SignalConfig& config);
    
    /**
     * @brief Reset all filters and analyzers
     */
    void reset();
    
    /**
     * @brief Get current signal quality
     * @return Signal quality score (0-100)
     */
    uint8_t getSignalQuality() const;
    
    /**
     * @brief Get noise level estimate
     * @return Estimated noise level
     */
    float getNoiseLevel() const;
    
    /**
     * @brief Enable/disable specific filter
     * @param filter_type Filter type to enable/disable
     * @param enable Enable flag
     */
    void setFilterEnabled(FilterType filter_type, bool enable);
    
private:
    SignalConfig config_;
    std::vector<std::unique_ptr<IDigitalFilter>> filters_;
    OutlierDetector outlier_detector_;
    PeakDetector peak_detector_;
    TrendAnalyzer trend_analyzer_;
    
    std::deque<float> recent_values_;
    float noise_level_estimate_;
    uint8_t signal_quality_;
    
    /**
     * @brief Initialize filters based on configuration
     */
    void initializeFilters();
    
    /**
     * @brief Apply all enabled filters
     * @param input Input value
     * @return Filtered value
     */
    float applyFilters(float input);
    
    /**
     * @brief Update noise level estimate
     * @param filtered_value Filtered signal value
     * @param raw_value Raw signal value
     */
    void updateNoiseEstimate(float filtered_value, float raw_value);
    
    /**
     * @brief Calculate signal quality
     * @param analysis Analysis results
     * @return Quality score (0-100)
     */
    uint8_t calculateSignalQuality(const SignalAnalysis& analysis) const;
};

} // namespace LightSensor
