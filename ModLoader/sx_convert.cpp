// ===========================================================================
//  sx_convert.cpp  -  command-line .PS2SX <-> .PCSX compiled-script converter
// ===========================================================================
//
// Converts Ultimate Spider-Man compiled scripts between the PS2-beta layout
// (.PS2SX: script_executable 0x60, vm_executable 0x28) and the final PC layout
// (.PCSX: script_executable 0x5C, vm_executable 0x24). The conversion is
// lossless and byte-exact in both directions (a round-trip reproduces the
// original file). See sx_transcode.h for the format details.
//
// Build (MSVC):   cl /EHsc /O2 /std:c++17 sx_convert.cpp
// Build (gcc):    g++ -std=c++17 -O2 sx_convert.cpp -o sx_convert
//
// Usage:
//   sx_convert <in> <out>            auto-detect direction
//   sx_convert --to-ps2 <in> <out>   force PC .PCSX  -> PS2 .PS2SX
//   sx_convert --to-pc  <in> <out>   force PS2 .PS2SX -> PC .PCSX

#include "sx_transcode.h"

#include <cstdio>
#include <cstring>
#include <vector>
#include <string>

static bool read_file(const char *path, std::vector<uint8_t> &out) {
    FILE *f = std::fopen(path, "rb");
    if (!f) return false;
    std::fseek(f, 0, SEEK_END);
    long n = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (n < 0) { std::fclose(f); return false; }
    out.resize((size_t)n);
    size_t got = std::fread(out.data(), 1, (size_t)n, f);
    std::fclose(f);
    return got == (size_t)n;
}

static bool write_file(const char *path, const std::vector<uint8_t> &data) {
    FILE *f = std::fopen(path, "wb");
    if (!f) return false;
    size_t put = std::fwrite(data.data(), 1, data.size(), f);
    std::fclose(f);
    return put == data.size();
}

int main(int argc, char **argv) {
    const char *in_path = nullptr, *out_path = nullptr;
    int forced = 0; // 0 = auto, 1 = to-ps2, 2 = to-pc

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--to-ps2") == 0)      forced = 1;
        else if (std::strcmp(argv[i], "--to-pc") == 0)  forced = 2;
        else if (!in_path)                              in_path = argv[i];
        else if (!out_path)                             out_path = argv[i];
    }
    if (!in_path || !out_path) {
        std::fprintf(stderr,
            "usage: %s [--to-ps2|--to-pc] <in> <out>\n"
            "  default: auto-detect direction by layout\n", argv[0]);
        return 2;
    }

    std::vector<uint8_t> in;
    if (!read_file(in_path, in)) {
        std::fprintf(stderr, "error: cannot read '%s'\n", in_path);
        return 1;
    }

    const bool is_pc   = sx::walk_ok(in.data(), in.size(), sx::SE_PC,   sx::VME_PC);
    const bool is_beta = sx::walk_ok(in.data(), in.size(), sx::SE_BETA, sx::VME_BETA);

    int dir = forced;
    if (dir == 0) {
        if (is_pc && !is_beta)      dir = 1; // PC -> PS2
        else if (is_beta && !is_pc) dir = 2; // PS2 -> PC
        else {
            std::fprintf(stderr,
                "error: cannot auto-detect layout (pc=%d beta=%d). "
                "Use --to-ps2 or --to-pc.\n", is_pc, is_beta);
            return 1;
        }
    }

    std::vector<uint8_t> out;
    bool ok = false;
    const char *desc = "";
    if (dir == 1) { ok = sx::transcode_pc_to_beta(in.data(), in.size(), out); desc = "PC .PCSX -> PS2 .PS2SX"; }
    else          { ok = sx::transcode_beta_to_pc(in.data(), in.size(), out); desc = "PS2 .PS2SX -> PC .PCSX"; }

    if (!ok) {
        std::fprintf(stderr, "error: transcode failed (input not a valid script mash for this direction)\n");
        return 1;
    }

    // verify the result parses in the target layout
    const bool out_ok = (dir == 1)
        ? sx::walk_ok(out.data(), out.size(), sx::SE_BETA, sx::VME_BETA)
        : sx::walk_ok(out.data(), out.size(), sx::SE_PC,   sx::VME_PC);
    if (!out_ok) {
        std::fprintf(stderr, "error: produced output failed validation\n");
        return 1;
    }

    if (!write_file(out_path, out)) {
        std::fprintf(stderr, "error: cannot write '%s'\n", out_path);
        return 1;
    }

    std::printf("%s: %zu -> %zu bytes  (%s)\n",
                desc, in.size(), out.size(), out_path);
    return 0;
}
#include "pch.h"