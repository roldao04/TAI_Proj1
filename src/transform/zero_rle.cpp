#include "transform/zero_rle.h"

#include <algorithm>
#include <stdexcept>

std::vector<uint8_t> ZeroRunLengthEncoder::encode(const std::vector<uint8_t>& input,
                                                  size_t min_run_length) {
    std::vector<uint8_t> output;
    output.reserve(input.size());

    size_t idx = 0;
    while (idx < input.size()) {
        if (input[idx] == 0) {
            size_t run_length = 1;
            while (idx + run_length < input.size() && input[idx + run_length] == 0) {
                run_length++;
            }

            if (run_length >= min_run_length) {
                size_t remaining = run_length;
                while (remaining > 0) {
                    size_t chunk = std::min<size_t>(remaining, 255);
                    output.push_back(MARKER);
                    output.push_back(static_cast<uint8_t>(chunk));
                    remaining -= chunk;
                }
            } else {
                output.insert(output.end(), run_length, 0);
            }

            idx += run_length;
            continue;
        }

        if (input[idx] == MARKER) {
            output.push_back(MARKER);
            output.push_back(0);
        } else {
            output.push_back(input[idx]);
        }
        idx++;
    }

    return output;
}

std::vector<uint8_t> ZeroRunLengthEncoder::decode(const std::vector<uint8_t>& input) {
    std::vector<uint8_t> output;
    output.reserve(input.size());

    for (size_t idx = 0; idx < input.size(); idx++) {
        uint8_t byte = input[idx];
        if (byte != MARKER) {
            output.push_back(byte);
            continue;
        }

        if (idx + 1 >= input.size()) {
            throw std::runtime_error("Invalid zero-run stream: missing marker payload");
        }

        uint8_t payload = input[++idx];
        if (payload == 0) {
            output.push_back(MARKER);
        } else {
            output.insert(output.end(), payload, 0);
        }
    }

    return output;
}
