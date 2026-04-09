#include "model/context_mixer.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <iostream>

// ============================================================================
// ContextMixer Implementation
// ============================================================================

ContextMixer::ContextMixer(int n_models, double lr)
    : num_models(n_models),
      learning_rate_fixed(PredictionUtils::double_to_fixed(lr)),
      total_updates(0),
      fast_stretch(&PredictionUtils::global_fast_stretch) {

    // Initialize all weights to 1.0 (equal weighting initially) in fixed-point
    weights_fixed.resize(n_models, PredictionUtils::FIXED_POINT_SCALE);  // 1.0 in fixed-point
    model_contributions.resize(n_models, 0);
}

double ContextMixer::mix(const std::vector<Prediction>& predictions) {
    if (predictions.size() != (size_t)num_models) {
        std::cerr << "Warning: Expected " << num_models << " predictions, got "
                  << predictions.size() << std::endl;
    }

    // Use FIXED-POINT arithmetic for deterministic mixing
    int64_t sum_stretched = 0;  // Sum of weighted stretched values
    int64_t sum_weights = 0;    // Sum of effective weights
    int valid_count = 0;

    // Mix in stretched (log-odds) space using fixed-point arithmetic
    for (size_t i = 0; i < predictions.size() && i < (size_t)num_models; i++) {
        if (predictions[i].valid && predictions[i].confidence > 0.0) {
            // Convert probability to fixed-point [0, PROB_SCALE]
            int32_t prob_fixed = (int32_t)(predictions[i].probability * PredictionUtils::PROB_SCALE);
            prob_fixed = std::clamp(prob_fixed, 1, (int32_t)PredictionUtils::PROB_SCALE - 1);

            // Stretch probability to log-odds using fixed-point lookup table
            int32_t stretched = fast_stretch->stretch_fixed(prob_fixed);

            // Convert confidence to fixed-point [0, FIXED_POINT_SCALE]
            int64_t confidence_fixed = (int64_t)(predictions[i].confidence * PredictionUtils::FIXED_POINT_SCALE);

            // Apply weight and confidence (all fixed-point multiplication)
            // effective_weight = weight * confidence / FIXED_POINT_SCALE
            int64_t effective_weight = (weights_fixed[i] * confidence_fixed) / PredictionUtils::FIXED_POINT_SCALE;

            // Accumulate: sum += effective_weight * stretched
            sum_stretched += effective_weight * stretched;
            sum_weights += effective_weight;
            valid_count++;
        }
    }

    // If no valid predictions, return 0.5 (maximum uncertainty)
    if (sum_weights == 0 || valid_count == 0) {
        return 0.5;
    }

    // Compute weighted average in stretched space (fixed-point division)
    int32_t mixed_stretched = (int32_t)(sum_stretched / sum_weights);

    // Squash back to probability using fixed-point lookup table
    int32_t prob_fixed = fast_stretch->squash_fixed(mixed_stretched);

    // Convert back to double for return
    double mixed_probability = (double)prob_fixed / PredictionUtils::PROB_SCALE;

    return mixed_probability;
}

void ContextMixer::update(const std::vector<Prediction>& predictions, bool actual_bit) {
    // FIXED-POINT VERSION: Use integer arithmetic for deterministic updates
    // Convert actual bit to target probability (0 or PROB_SCALE)
    int32_t target_fixed = actual_bit ? PredictionUtils::PROB_SCALE : 0;

    // Update each model's weight based on its prediction error
    for (size_t i = 0; i < predictions.size() && i < (size_t)num_models; i++) {
        // Use epsilon comparison instead of direct floating-point comparison
        if (predictions[i].valid && predictions[i].confidence > 1e-9) {
            // Convert probability to fixed-point using DETERMINISTIC ROUNDING
            // std::lround() ensures encoder and decoder produce identical integers
            int32_t prob_fixed = (int32_t)std::lround(predictions[i].probability * PredictionUtils::PROB_SCALE);
            prob_fixed = std::clamp(prob_fixed, 1, (int32_t)PredictionUtils::PROB_SCALE - 1);

            // Calculate error in fixed-point: error = target - prob
            int32_t error_fixed = target_fixed - prob_fixed;

            // Stretch prediction to log-odds using fixed-point lookup table
            int32_t stretched = fast_stretch->stretch_fixed(prob_fixed);

            // Convert confidence to fixed-point using DETERMINISTIC ROUNDING
            int64_t confidence_fixed = (int64_t)std::lround(predictions[i].confidence * PredictionUtils::FIXED_POINT_SCALE);

            // Gradient descent update (all fixed-point arithmetic):
            // update = learning_rate * stretched * error * confidence
            // All values scaled appropriately to prevent overflow
            int64_t update = (learning_rate_fixed * stretched * error_fixed * confidence_fixed)
                           / (PredictionUtils::STRETCH_SCALE * PredictionUtils::PROB_SCALE * PredictionUtils::FIXED_POINT_SCALE);

            weights_fixed[i] += update;

            // Keep weights in valid range: [0.001, 3.0]
            weights_fixed[i] = std::max((int64_t)65, weights_fixed[i]);  // Min 0.001
            weights_fixed[i] = std::min((int64_t)(3 * PredictionUtils::FIXED_POINT_SCALE), weights_fixed[i]);  // Max 3.0

            // Track which models are contributing
            if (std::abs(update) > 10) {  // Threshold adjusted for fixed-point
                model_contributions[i]++;
            }
        }
    }

    // Periodically normalize weights to prevent overflow
    total_updates++;
    if (total_updates % 1000 == 0) {  // More frequent normalization for better balance
        normalize_weights();
    }
}

void ContextMixer::reset_weights() {
    std::fill(weights_fixed.begin(), weights_fixed.end(), PredictionUtils::FIXED_POINT_SCALE);  // 1.0 in fixed-point
    std::fill(model_contributions.begin(), model_contributions.end(), 0);
    total_updates = 0;
}

void ContextMixer::normalize_weights() {
    // Fixed-point normalization: scale all weights so sum = num_models * FIXED_POINT_SCALE
    int64_t sum = std::accumulate(weights_fixed.begin(), weights_fixed.end(), (int64_t)0);
    if (sum > 0) {
        int64_t target_sum = num_models * PredictionUtils::FIXED_POINT_SCALE;  // Target sum
        // Scale = target_sum / sum (in fixed-point)
        for (int64_t& w : weights_fixed) {
            w = (w * target_sum) / sum;
        }
    }
}

// ============================================================================
// HierarchicalMixer Implementation
// ============================================================================

HierarchicalMixer::HierarchicalMixer(const std::vector<int>& primary_sizes, double learning_rate)
    : num_primary_mixers(primary_sizes.size()) {

    // Create primary mixers
    for (int size : primary_sizes) {
        primary_mixers.push_back(new ContextMixer(size, learning_rate));
    }

    // Create secondary mixer (mixes outputs of primary mixers)
    secondary_mixer = new ContextMixer(num_primary_mixers, learning_rate);
}

HierarchicalMixer::~HierarchicalMixer() {
    for (ContextMixer* mixer : primary_mixers) {
        delete mixer;
    }
    delete secondary_mixer;
}

double HierarchicalMixer::mix(const std::vector<std::vector<Prediction>>& predictions) {
    if (predictions.size() != (size_t)num_primary_mixers) {
        std::cerr << "Warning: Expected " << num_primary_mixers << " prediction groups, got "
                  << predictions.size() << std::endl;
        return 0.5;
    }

    // First stage: Mix each group of predictions
    std::vector<Prediction> primary_outputs;
    for (size_t i = 0; i < predictions.size(); i++) {
        double mixed_prob = primary_mixers[i]->mix(predictions[i]);

        // Calculate average confidence from this group
        double avg_confidence = 0.0;
        int valid_count = 0;
        for (const auto& pred : predictions[i]) {
            if (pred.valid) {
                avg_confidence += pred.confidence;
                valid_count++;
            }
        }
        if (valid_count > 0) {
            avg_confidence /= valid_count;
        }

        primary_outputs.emplace_back(mixed_prob, avg_confidence, true, i);
    }

    // Second stage: Mix primary outputs
    return secondary_mixer->mix(primary_outputs);
}

void HierarchicalMixer::update(const std::vector<std::vector<Prediction>>& predictions,
                               bool actual_bit) {
    if (predictions.size() != (size_t)num_primary_mixers) {
        return;
    }

    // Update primary mixers
    std::vector<Prediction> primary_outputs;
    for (size_t i = 0; i < predictions.size(); i++) {
        primary_mixers[i]->update(predictions[i], actual_bit);

        // Recreate primary outputs for secondary mixer update
        double mixed_prob = primary_mixers[i]->mix(predictions[i]);
        double avg_confidence = 0.0;
        int valid_count = 0;
        for (const auto& pred : predictions[i]) {
            if (pred.valid) {
                avg_confidence += pred.confidence;
                valid_count++;
            }
        }
        if (valid_count > 0) {
            avg_confidence /= valid_count;
        }
        primary_outputs.emplace_back(mixed_prob, avg_confidence, true, i);
    }

    // Update secondary mixer
    secondary_mixer->update(primary_outputs, actual_bit);
}

void HierarchicalMixer::reset_all_weights() {
    for (ContextMixer* mixer : primary_mixers) {
        mixer->reset_weights();
    }
    secondary_mixer->reset_weights();
}
