#include "model/frequency_model.h"
#include <algorithm>
#include <stdexcept>

FrequencyModel::FrequencyModel() : total_freq(0) {
    frequencies.fill(0);
    cumulative_freq.fill(0);
}

static void count_frequencies(const uint8_t* __restrict__ data,
                               size_t n,
                               uint32_t* __restrict__ freq) {
    for (size_t i = 0; i < n; i++) {
        freq[data[i]]++;
    }
}

void FrequencyModel::build_from_data(const std::vector<uint8_t>& data) {
    frequencies.fill(0);

    count_frequencies(data.data(), data.size(), frequencies.data());

    frequencies[EOF_SYMBOL] = 1;

    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (frequencies[i] == 0) {
            frequencies[i] = 1;
        }
    }

    build_cumulative_freq();
}

void FrequencyModel::build_cumulative_freq() {
    cumulative_freq[0] = 0;
    for (int i = 1; i < ALPHABET_SIZE; i++) {
        cumulative_freq[i] = cumulative_freq[i - 1] + frequencies[i - 1];
    }
    total_freq = cumulative_freq[ALPHABET_SIZE - 1] + frequencies[ALPHABET_SIZE - 1];
}

void FrequencyModel::get_symbol_range(int symbol, uint32_t& cum_freq_low, uint32_t& cum_freq_high) const {
    if (symbol < 0 || symbol >= ALPHABET_SIZE) {
        throw std::out_of_range("Symbol out of range");
    }
    cum_freq_low = cumulative_freq[symbol];
    cum_freq_high = cumulative_freq[symbol] + frequencies[symbol];
}

int FrequencyModel::find_symbol(uint32_t cum_freq) const {
    int low = 0;
    int high = ALPHABET_SIZE - 1;

    while (low < high) {
        int mid = (low + high + 1) / 2;
        if (cumulative_freq[mid] > cum_freq) {
            high = mid - 1;
        } else {
            low = mid;
        }
    }

    return low;
}

void FrequencyModel::set_frequencies(const std::array<uint32_t, ALPHABET_SIZE>& freq) {
    frequencies = freq;
    build_cumulative_freq();
}
