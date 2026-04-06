#ifndef CONTEXT_MIXER_H
#define CONTEXT_MIXER_H

#include <vector>
#include <cstdint>
#include <stdexcept>
#include "model/prediction_utils.h"

/**
 * Prediction from a single model
 */
struct Prediction {
    double probability;      // P(bit=0) or P(symbol)
    double confidence;       // How certain this model is (0.0 to 1.0)
    bool valid;             // Is this prediction valid?
    int model_id;           // Which model made this prediction
    uint32_t context_count; // Number of times this context was seen

    Prediction() : probability(0.5), confidence(0.0), valid(false), model_id(-1), context_count(0) {}

    Prediction(double prob, double conf, bool v = true, int id = -1, uint32_t count = 0)
        : probability(prob), confidence(conf), valid(v), model_id(id), context_count(count) {}
};

/**
 * Context Mixer - Combines predictions from multiple models using adaptive weights
 *
 * Based on PAQ's approach with FIXED-POINT arithmetic for deterministic encoder/decoder sync:
 * 1. Transform predictions to log-odds (stretched) domain using integer lookup tables
 * 2. Compute weighted average in stretched space using fixed-point arithmetic
 * 3. Transform back to probability (squashed) using integer lookup tables
 * 4. After seeing actual bit, update weights via fixed-point gradient descent
 *
 * All arithmetic uses integers to ensure bit-exact identical results on encoder and decoder.
 */
class ContextMixer {
private:
    int num_models;                        // Number of models being mixed
    int64_t learning_rate_fixed;           // Learning rate in fixed-point (FIXED_POINT_SCALE)
    std::vector<int64_t> weights_fixed;    // Weights in fixed-point (FIXED_POINT_SCALE)

    // Statistics for monitoring
    uint64_t total_updates;
    std::vector<uint64_t> model_contributions;

    // Fast stretch/squash with fixed-point support
    PredictionUtils::FastStretch* fast_stretch;

public:
    /**
     * Constructor
     * @param n_models Number of models to mix
     * @param lr Learning rate (default: 0.002)
     */
    explicit ContextMixer(int n_models, double lr = 0.002);

    /**
     * Mix predictions from multiple models
     * @param predictions Vector of predictions from each model
     * @return Mixed probability P(bit=0)
     */
    double mix(const std::vector<Prediction>& predictions);

    /**
     * Update weights based on actual outcome
     * @param predictions The predictions that were mixed
     * @param actual_bit The actual bit that occurred (0 or 1)
     */
    void update(const std::vector<Prediction>& predictions, bool actual_bit);

    /**
     * Get current weights (for debugging/analysis) - returns as doubles for compatibility
     */
    std::vector<double> get_weights() const {
        std::vector<double> weights_double;
        weights_double.reserve(weights_fixed.size());
        for (int64_t w : weights_fixed) {
            weights_double.push_back(PredictionUtils::fixed_to_double(w));
        }
        return weights_double;
    }

    /**
     * Get current weights as fixed-point integers (for blending)
     */
    const std::vector<int64_t>& get_weights_fixed() const { return weights_fixed; }

    /**
     * Reset all weights to default (1.0)
     */
    void reset_weights();

    /**
     * Set individual weight (for decoder initialization from file)
     * @param index Weight index (0 to num_models-1)
     * @param weight_fixed Weight in fixed-point format
     */
    void set_weight_fixed(size_t index, int64_t weight_fixed) {
        if (index < weights_fixed.size()) {
            weights_fixed[index] = weight_fixed;
        }
    }

    /**
     * Set all weights at once (for decoder initialization)
     * @param new_weights Vector of weights in fixed-point format
     */
    void set_all_weights_fixed(const std::vector<int64_t>& new_weights) {
        if (new_weights.size() == weights_fixed.size()) {
            weights_fixed = new_weights;
        } else {
            throw std::runtime_error("Weight vector size mismatch in set_all_weights_fixed");
        }
    }

    /**
     * Get statistics
     */
    uint64_t get_total_updates() const { return total_updates; }
    const std::vector<uint64_t>& get_model_contributions() const { return model_contributions; }

    /**
     * Normalize weights so they sum to num_models
     */
    void normalize_weights();
};

/**
 * Hierarchical Mixer - Multiple mixing stages for better compression
 *
 * Stage 1: Mix related models (e.g., all PPM orders together)
 * Stage 2: Mix outputs from stage 1
 * Stage 3: Final combination
 */
class HierarchicalMixer {
private:
    std::vector<ContextMixer*> primary_mixers;   // First level mixers
    ContextMixer* secondary_mixer;                // Second level mixer
    int num_primary_mixers;

public:
    /**
     * Constructor
     * @param primary_sizes Number of models in each primary mixer
     * @param learning_rate Learning rate for all mixers
     */
    HierarchicalMixer(const std::vector<int>& primary_sizes, double learning_rate = 0.002);

    ~HierarchicalMixer();

    /**
     * Mix predictions through hierarchy
     * @param predictions All predictions grouped by primary mixer
     * @return Final mixed probability
     */
    double mix(const std::vector<std::vector<Prediction>>& predictions);

    /**
     * Update all mixer weights
     * @param predictions All predictions that were mixed
     * @param actual_bit The actual bit value
     */
    void update(const std::vector<std::vector<Prediction>>& predictions, bool actual_bit);

    /**
     * Reset all weights
     */
    void reset_all_weights();

    /**
     * Get primary mixer at index
     */
    ContextMixer* get_primary_mixer(int index) {
        if (index >= 0 && index < num_primary_mixers) {
            return primary_mixers[index];
        }
        return nullptr;
    }

    /**
     * Get secondary mixer
     */
    ContextMixer* get_secondary_mixer() { return secondary_mixer; }
};

#endif // CONTEXT_MIXER_H
