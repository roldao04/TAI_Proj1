#ifndef X86_FILTER_H
#define X86_FILTER_H

#include <cstdint>
#include <vector>

/*
 * Small reversible x86 branch filter.
 *
 * This normalizes x86/x86-64 rel32 CALL/JMP targets (E8/E9) into absolute
 * stream offsets so repeated code addresses compress better. The transform is
 * fully reversible and intentionally conservative: it only touches complete
 * 5-byte instructions with these two opcodes.
 */

struct ElfX86Info {
    bool is_elf = false;
    bool is_64_bit = false;
    bool is_little_endian = false;
    bool is_x86_64 = false;
    bool is_executable = false;
};

class X86Filter {
public:
    static std::vector<uint8_t> encode(const std::vector<uint8_t>& input,
                                       uint32_t base_offset = 0);

    static std::vector<uint8_t> decode(const std::vector<uint8_t>& input,
                                       uint32_t base_offset = 0);

    static ElfX86Info inspect_elf_header(const std::vector<uint8_t>& input);

    static bool is_x86_64_elf_executable(const std::vector<uint8_t>& input);
};

#endif // X86_FILTER_H
