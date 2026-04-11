#include "transform/x86_filter.h"

#include <cstddef>

namespace {

constexpr uint8_t ELF_MAG0 = 0x7f;
constexpr uint8_t ELF_MAG1 = 'E';
constexpr uint8_t ELF_MAG2 = 'L';
constexpr uint8_t ELF_MAG3 = 'F';
constexpr uint8_t ELFCLASS64 = 2;
constexpr uint8_t ELFDATA2LSB = 1;

constexpr uint16_t ET_EXEC = 2;
constexpr uint16_t ET_DYN = 3;
constexpr uint16_t EM_X86_64 = 62;

constexpr uint8_t X86_CALL_REL32 = 0xE8;
constexpr uint8_t X86_JMP_REL32 = 0xE9;

uint16_t read_le16(const std::vector<uint8_t>& input, size_t offset) {
    return static_cast<uint16_t>(input[offset]) |
           (static_cast<uint16_t>(input[offset + 1]) << 8);
}

uint32_t read_le32(const std::vector<uint8_t>& input, size_t offset) {
    return static_cast<uint32_t>(input[offset]) |
           (static_cast<uint32_t>(input[offset + 1]) << 8) |
           (static_cast<uint32_t>(input[offset + 2]) << 16) |
           (static_cast<uint32_t>(input[offset + 3]) << 24);
}

void write_le32(std::vector<uint8_t>& output, size_t offset, uint32_t value) {
    output[offset] = static_cast<uint8_t>(value & 0xffu);
    output[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xffu);
    output[offset + 2] = static_cast<uint8_t>((value >> 16) & 0xffu);
    output[offset + 3] = static_cast<uint8_t>((value >> 24) & 0xffu);
}

std::vector<uint8_t> transform_rel32(const std::vector<uint8_t>& input, uint32_t base_offset, bool encode) {
    std::vector<uint8_t> output = input;

    size_t i = 0;
    while (i + 4 < output.size()) {
        const uint8_t opcode = output[i];
        if (opcode == X86_CALL_REL32 || opcode == X86_JMP_REL32) {
            const uint32_t operand = read_le32(output, i + 1);
            const uint32_t instruction_end = base_offset + static_cast<uint32_t>(i) + 5u;
            const uint32_t transformed = encode ? (operand + instruction_end)
                                                : (operand - instruction_end);

            write_le32(output, i + 1, transformed);
            i += 5;
            continue;
        }

        ++i;
    }

    return output;
}

} // namespace

std::vector<uint8_t> X86Filter::encode(const std::vector<uint8_t>& input, uint32_t base_offset) {
    return transform_rel32(input, base_offset, true);
}

std::vector<uint8_t> X86Filter::decode(const std::vector<uint8_t>& input, uint32_t base_offset) {
    return transform_rel32(input, base_offset, false);
}

ElfX86Info X86Filter::inspect_elf_header(const std::vector<uint8_t>& input) {
    ElfX86Info info;

    if (input.size() < 20) {
        return info;
    }

    info.is_elf = input[0] == ELF_MAG0 && input[1] == ELF_MAG1 &&
                  input[2] == ELF_MAG2 && input[3] == ELF_MAG3;
    if (!info.is_elf) {
        return info;
    }

    info.is_64_bit = input[4] == ELFCLASS64;
    info.is_little_endian = input[5] == ELFDATA2LSB;

    if (!info.is_little_endian) {
        return info;
    }

    const uint16_t elf_type = read_le16(input, 16);
    const uint16_t machine = read_le16(input, 18);

    info.is_x86_64 = machine == EM_X86_64;
    info.is_executable = elf_type == ET_EXEC || elf_type == ET_DYN;

    return info;
}

bool X86Filter::is_x86_64_elf_executable(const std::vector<uint8_t>& input) {
    const ElfX86Info info = inspect_elf_header(input);
    return info.is_elf && info.is_64_bit && info.is_little_endian &&
           info.is_x86_64 && info.is_executable;
}
