#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

#include "transform/x86_filter.h"

namespace {

void append_rel32(std::vector<uint8_t>& stream, uint8_t opcode, int32_t displacement) {
    const uint32_t value = static_cast<uint32_t>(displacement);
    stream.push_back(opcode);
    stream.push_back(static_cast<uint8_t>(value & 0xffu));
    stream.push_back(static_cast<uint8_t>((value >> 8) & 0xffu));
    stream.push_back(static_cast<uint8_t>((value >> 16) & 0xffu));
    stream.push_back(static_cast<uint8_t>((value >> 24) & 0xffu));
}

std::vector<uint8_t> make_elf64_header(uint16_t elf_type, uint16_t machine) {
    std::vector<uint8_t> header(64, 0);
    header[0] = 0x7f;
    header[1] = 'E';
    header[2] = 'L';
    header[3] = 'F';
    header[4] = 2;
    header[5] = 1;
    header[16] = static_cast<uint8_t>(elf_type & 0xffu);
    header[17] = static_cast<uint8_t>((elf_type >> 8) & 0xffu);
    header[18] = static_cast<uint8_t>(machine & 0xffu);
    header[19] = static_cast<uint8_t>((machine >> 8) & 0xffu);
    return header;
}

void test_round_trip_on_relative_branches() {
    std::cout << "Test 1: rel32 branch round trip..." << std::endl;

    std::vector<uint8_t> input = {
        0x48, 0x89, 0xe5,
        0x90
    };
    append_rel32(input, 0xE8, 0x12345678);
    input.push_back(0x90);
    append_rel32(input, 0xE9, -0x1020304);
    input.push_back(0xC3);

    std::vector<uint8_t> encoded = X86Filter::encode(input);
    std::vector<uint8_t> decoded = X86Filter::decode(encoded);

    assert(encoded != input);
    assert(decoded == input);

    std::cout << "  ✓ Passed\n" << std::endl;
}

void test_round_trip_with_base_offset_and_trailing_bytes() {
    std::cout << "Test 2: base offset and trailing bytes..." << std::endl;

    std::vector<uint8_t> input = {0x90, 0x90};
    append_rel32(input, 0xE8, -16);
    input.push_back(0xE9);
    input.push_back(0xAA);
    input.push_back(0xBB);
    input.push_back(0xCC);

    std::vector<uint8_t> encoded = X86Filter::encode(input, 0x1000u);
    std::vector<uint8_t> decoded = X86Filter::decode(encoded, 0x1000u);

    assert(decoded == input);
    assert(encoded[input.size() - 4] == 0xE9);
    assert(encoded[input.size() - 3] == 0xAA);
    assert(encoded[input.size() - 2] == 0xBB);
    assert(encoded[input.size() - 1] == 0xCC);

    std::cout << "  ✓ Passed\n" << std::endl;
}

void test_elf_detector_for_x86_64_executable() {
    std::cout << "Test 3: ELF x86-64 detector..." << std::endl;

    std::vector<uint8_t> header = make_elf64_header(2, 62);
    ElfX86Info info = X86Filter::inspect_elf_header(header);

    assert(info.is_elf);
    assert(info.is_64_bit);
    assert(info.is_little_endian);
    assert(info.is_x86_64);
    assert(info.is_executable);
    assert(X86Filter::is_x86_64_elf_executable(header));

    std::cout << "  ✓ Passed\n" << std::endl;
}

void test_elf_detector_rejects_non_x86_or_non_elf() {
    std::cout << "Test 4: ELF detector rejects non-matching inputs..." << std::endl;

    std::vector<uint8_t> arm_header = make_elf64_header(2, 183);
    ElfX86Info arm_info = X86Filter::inspect_elf_header(arm_header);
    assert(arm_info.is_elf);
    assert(!arm_info.is_x86_64);
    assert(!X86Filter::is_x86_64_elf_executable(arm_header));

    std::vector<uint8_t> non_elf(64, 0);
    ElfX86Info non_elf_info = X86Filter::inspect_elf_header(non_elf);
    assert(!non_elf_info.is_elf);
    assert(!X86Filter::is_x86_64_elf_executable(non_elf));

    std::cout << "  ✓ Passed\n" << std::endl;
}

} // namespace

int main() {
    std::cout << "=== X86 Filter Tests ===\n" << std::endl;

    test_round_trip_on_relative_branches();
    test_round_trip_with_base_offset_and_trailing_bytes();
    test_elf_detector_for_x86_64_executable();
    test_elf_detector_rejects_non_x86_or_non_elf();

    std::cout << "All x86 filter tests passed." << std::endl;
    return 0;
}
