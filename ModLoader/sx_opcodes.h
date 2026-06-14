#pragma once

// ===========================================================================
//  sx_opcodes.h  -  Ultimate Spider-Man chuck-VM opcode reference (complete)
// ===========================================================================
//
// Completes usm_src/src/chuck/vm/opcodes.h, which left opcodes 47..60 unnamed.
// These names/semantics were recovered by comparing the PS2 beta interpreter
// (SLUS ELF vm_thread::run) against the final PC interpreter (vm_thread.cpp):
// the two dispatch tables are IDENTICAL for every opcode the games use - same
// numbers, same behaviour. The PC build only *added* opcodes 53..60 on top of
// the PS2 set (which ended at 0x34/52).
//
// Consequence for transcoding: a compiled-script bytecode image (sx_exe_image)
// is byte-compatible between .PS2SX and .PCSX. It is copied VERBATIM by the
// struct transcoders in sx_transcode.h; no opcode or operand rewrite is needed.
// (The two builds differ only in operand *values* - library/string-table
// indices - which are self-consistent within each file and travel with it.)
//
// Instruction encoding (16-bit little-endian words):
//   opword = (opcode << 8) | argtype
//   if (opword & 0x0080) a 16-bit data-size word follows the opword
//   then argtype operand words follow (see sx_opcode_arg_words()).

#include <cstdint>

namespace sx {

inline constexpr uint16_t SX_OP_DSIZE_FLAG   = 0x0080u;
inline constexpr uint16_t SX_OP_ARGTYPE_MASK = 0x007Fu;

enum sx_opcode_t : uint8_t {
    SX_ADD = 0,   // a + b
    SX_AND = 1,   // a & b
    SX_BF  = 2,   // if (!a) branch (PCR)
    SX_BRA = 3,   // branch (PCR)
    SX_BSL = 4,   // call library function (LFR)
    SX_BSR = 5,   // branch to subroutine (SFR)
    SX_BST = 6,
    SX_BTH = 7,
    SX_DEC = 8,   // --
    SX_DIV = 9,   // a / b
    SX_DUP = 10,  // duplicate top
    SX_EQ  = 11,  // ==
    SX_GE  = 12,  // >=
    SX_GT  = 13,  // >
    SX_INC = 14,  // ++
    SX_KIL = 15,  // kill
    SX_LE  = 16,  // <=
    SX_LNT = 18,  // logical not
    SX_LT  = 20,  // <
    SX_MOD = 21,  // %
    SX_MUL = 22,  // *
    SX_NE  = 23,  // !=
    SX_NEG = 24,  // negate
    SX_NOP = 25,  // nop
    SX_NOT = 26,  // ~
    SX_OR  = 27,  // |
    SX_POP = 28,  // pop
    SX_PSH = 29,  // push
    SX_RET = 30,  // return (WORD = bytes of args to drop)
    SX_SHL = 31,  // <<
    SX_SHR = 32,  // >>
    SX_SPA = 33,  // stack-pointer adjust (WORD)
    SX_SUB = 34,  // a - b
    SX_XOR = 35,  // ^
    SX_STR_EQ = 37, // string ==
    SX_STR_NE = 38, // string !=
    SX_ECB = 43,  // create_event_callback
    SX_ESB = 44,  // create_static_event_callback
    SX_ECO = 45,  // create_event_callback   (alias group of ECB on PS2)
    SX_SCO = 46,  // create_static_event_callback (alias group of ESB on PS2)
    // ---- recovered (were unnamed in opcodes.h) ----
    SX_RSE = 47,  // raise_event
    SX_RAE = 48,  // raise_all_event
    SX_KTH = 49,  // kill_thread (SFR; preceded by PSH SPR)
    SX_BFF = 50,  // if (top_num == 0.0) branch (PCR)  -- float form of BF
    SX_ITOA = 51, // int   -> temp string (WORD)
    SX_FTOA = 52, // float -> temp string (WORD)
    SX_SCAT = 53, // strcat top two strings (WORD)        [PC-only, >PS2 0x34]
    SX_MAS = 54,  // massacre_threads (NULL or SFR)       [PC-only]
    SX_MS2 = 55,  // massacre_threads + kill self (SFR)   [PC-only]
    SX_MSP = 56,  // move SP (WORD)                       [PC-only]
    SX_DEF = 57,  // deferred SP op (WORD)                [PC-only]
    SX_LDX = 58,  // indexed load from instance buffer    [PC-only]
    SX_LDS = 59,  // indexed load from static data (SDR)  [PC-only]
    SX_W0  = 60,  // word no-op (WORD)                    [PC-only]
};

enum sx_opcode_arg_t : uint8_t {
    SX_ARG_NULL = 0,
    SX_ARG_NUM  = 1,   // 4-byte immediate (often float)
    SX_ARG_NUMR = 2,   // 4-byte immediate, reversed operands
    SX_ARG_STR  = 3,   // 4-byte string-table reference
    SX_ARG_WORD = 4,   // 2-byte immediate
    SX_ARG_PCR  = 5,   // 2-byte PC-relative offset
    SX_ARG_SPR  = 6,   // 2-byte SP-relative offset
    SX_ARG_POPO = 7,   // 2-byte: pop address + offset
    SX_ARG_SDR  = 8,   // 4-byte static-data-member reference
    SX_ARG_SFR  = 9,   // 4-byte script-function reference
    SX_ARG_LFR  = 10,  // 4-byte library-function reference
    SX_ARG_CLV  = 11,  // 4-byte class-value reference
    SX_ARG_SIG  = 15,  // 4-byte signal
    SX_ARG_PSIG = 16,  // 4-byte parameterised signal
    SX_ARG_A17  = 17,  // 4-byte (same width as the references above)
};

// Number of 16-bit operand words that follow an opword with this argtype.
// (0 = none, 1 = WORD/PCR/SPR/POPO, 2 = NUM/STR/reference classes.)
// Returns 0xFF for an invalid argtype.
inline uint8_t sx_opcode_arg_words(uint8_t argtype) {
    switch (argtype) {
        case SX_ARG_NULL: return 0;
        case SX_ARG_WORD: case SX_ARG_PCR: case SX_ARG_SPR: case SX_ARG_POPO: return 1;
        case SX_ARG_NUM:  case SX_ARG_NUMR: case SX_ARG_STR:
        case SX_ARG_SDR:  case SX_ARG_SFR:  case SX_ARG_LFR:  case SX_ARG_CLV:
        case SX_ARG_SIG:  case SX_ARG_PSIG: case SX_ARG_A17:  return 2;
        default: return 0xFF;
    }
}

// True if `opcode` is dispatched by both the PS2-beta and final PC VMs
// (0..52 shared; 53..60 are PC-only; 61..65 are accepted as no-ops on PC).
inline bool sx_opcode_is_shared(uint8_t opcode) { return opcode <= 52; }
inline bool sx_opcode_is_valid_pc(uint8_t opcode) { return opcode <= 65; }

} // namespace sx
