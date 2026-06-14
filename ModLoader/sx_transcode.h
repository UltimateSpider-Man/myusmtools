#pragma once
#include <cstdint>
#include <cstring>
#include <vector>

namespace sx {


    static inline uint32_t gen_safety_key(uint32_t field_8, uint32_t field_4,
        uint16_t class_id, uint16_t field_E)
    {
        return ((field_8 + 0x7BADBA5Du - (field_4 & 0x0FFFFFFFu) + class_id + field_E)
            & 0x0FFFFFFFu) | 0x70000000u;
    }

    // Struct sizes.
    static constexpr uint32_t SE_BETA = 0x60, SE_PC = 0x5C; // script_executable
    static constexpr uint32_t VME_BETA = 0x28, VME_PC = 0x24; // vm_executable
    static constexpr uint32_t SO_SIZE = 0x34;                // script_object
    static constexpr uint32_t INFO_SIZE = 0x1C;               // script_executable::info_t
    static constexpr uint32_t HDR_SIZE = 0x10;                // generic_mash_header
    static constexpr uint32_t OBJ_OFF = 0x10;                // object base = after header


    static constexpr uint32_t SE_EXTRA_FILL = 0xA1A1A1A1u; // script_executable + 0x5C (PS2 mash ptr placeholder)
    static constexpr uint32_t VME_EXTRA_FILL = 0x00000000u; // vm_executable     + 0x1C (null ptr, before flags)
    static constexpr uint8_t  PAD_FILL = 0xE3u;       // PS2 heap-junk fill for rebase + tail padding
    static constexpr uint32_t TAIL_ALIGN = 16u;         // .ps2sx is padded to a 16-byte boundary

    // Sanity caps so a garbage buffer can't drive a pathological loop.
    static constexpr uint32_t MAX_SO = 4096, MAX_FUNCS = 65536;
    static constexpr uint32_t MAX_PERM = 262144, MAX_INFO = 65536;

    static inline uint32_t rd32(const uint8_t* d, size_t off) {
        uint32_t v; std::memcpy(&v, d + off, 4); return v;
    }
    static inline uint16_t rd16(const uint8_t* d, size_t off) {
        uint16_t v; std::memcpy(&v, d + off, 2); return v;
    }
    static inline uint32_t align_up(uint32_t pos, uint32_t a) {
        uint32_t v = a - (pos % a);
        return (v < a) ? pos + v : pos;
    }


    struct bsl_region { uint16_t lo, hi; int16_t delta; };

    struct bsl_slc_update_stats {
        size_t global_refs = 0;
        size_t local_refs = 0;
        size_t changed_global = 0;
        size_t changed_local = 0;
        size_t unchanged_global = 0;
        size_t unchanged_local = 0;

        size_t total_refs() const { return global_refs + local_refs; }
        size_t total_changed() const { return changed_global + changed_local; }
    };

    // PS2 beta BSL SLC remap tables.
    // group 0x0000 = global SLC, group 0x0007 = local SLC.
    // Deltas are applied in the PS2 beta -> PC direction; inverse remap uses
    // the same tables for PC -> PS2 beta.
    static constexpr bsl_region PS2_BETA_BSL_GLOBAL[] = {
        {0x0000,0x0011, 0},{0x0012,0x006B, 4},{0x006C,0x0086, 5},{0x0087,0x0095, 6},
        {0x0096,0x00E1, 4},{0x00E2,0x00EC, 5},{0x00ED,0x00EE, 8},{0x00EF,0x00FA, 9},
        {0x00FB,0x00FE,11},{0x00FF,0x011D,12},{0x011E,0x0129,13},{0x012A,0x0148,12},
        {0x0149,0x0152,13},{0x0153,0x0162,14},{0x0163,0x0191,15},{0x0192,0x01F0,16},
    };
    static constexpr bsl_region PS2_BETA_BSL_LOCAL[] = {
        {0x0000,0x0030,0},{0x0031,0x004F,1},{0x0050,0x0054,2},{0x0055,0x006C,3},
        {0x006D,0x00B5,2},{0x00B6,0x00BB,3},{0x00BC,0x00C2,4},{0x00C3,0x00ED,5},
        {0x00EE,0x01F0,6},
    };
    static constexpr size_t PS2_BETA_BSL_GLOBAL_N = sizeof(PS2_BETA_BSL_GLOBAL) / sizeof(PS2_BETA_BSL_GLOBAL[0]);
    static constexpr size_t PS2_BETA_BSL_LOCAL_N = sizeof(PS2_BETA_BSL_LOCAL) / sizeof(PS2_BETA_BSL_LOCAL[0]);

    // Backward-compatible names used by older code. These now explicitly mean
    // the PS2 beta local/global SLC tables above.
    static constexpr const bsl_region(&PS2_BSL_GLOBAL)[PS2_BETA_BSL_GLOBAL_N] = PS2_BETA_BSL_GLOBAL;
    static constexpr const bsl_region(&PS2_BSL_LOCAL)[PS2_BETA_BSL_LOCAL_N] = PS2_BETA_BSL_LOCAL;
    static constexpr size_t PS2_BSL_GLOBAL_N = PS2_BETA_BSL_GLOBAL_N;
    static constexpr size_t PS2_BSL_LOCAL_N = PS2_BETA_BSL_LOCAL_N;


    static constexpr bsl_region XB_BSL_GLOBAL[] = {
        {0x0000,0x0022, 0},{0x0023,0x0059, 1},{0x005A,0x005A, 0},{0x005B,0x0091, 1},
        {0x0092,0x0092, 0},{0x0093,0x00DB, 1},{0x00DC,0x00E6, 3},{0x00E7,0x00E7, 0},
        {0x00E8,0x00E9, 3},{0x00EA,0x00EB, 4},{0x00EC,0x00F4, 4},{0x00F5,0x010A, 6},
        {0x010B,0x010B, 0},{0x010C,0x0118, 6},{0x0119,0x0119, 0},{0x011A,0x0123, 6},
        {0x0124,0x0130, 7},{0x0131,0x01C4, 6},{0x01C5,0x01F0, 0},
    };
    static constexpr bsl_region XB_BSL_LOCAL[] = {
        {0x0000,0x00B6, 0},{0x00B7,0x00CC,-1},{0x00CD,0x00CD, 0},{0x00CE,0x00EC,-1},
        {0x00ED,0x00ED, 0},{0x00EE,0x00F3,-1},{0x00F4,0x00F4, 0},{0x00F5,0x01F0,-1},
    };
    static constexpr size_t XB_BSL_GLOBAL_N = sizeof(XB_BSL_GLOBAL) / sizeof(XB_BSL_GLOBAL[0]);
    static constexpr size_t XB_BSL_LOCAL_N = sizeof(XB_BSL_LOCAL) / sizeof(XB_BSL_LOCAL[0]);


    static constexpr bsl_region XBETA_BSL_GLOBAL[] = {
        {0x0000,0x0022, 0},{0x0023,0x0059, 0},{0x005A,0x005A, 0},{0x005B,0x0091, 0},
        {0x0092,0x0092, 0},{0x0093,0x00DB, 0},{0x00DC,0x00E6, 2},{0x00E7,0x00E7, 0},
        {0x00E8,0x00E9, 2},{0x00EA,0x00EB, 3},{0x00EC,0x00F4, 3},{0x00F5,0x010A, 5},
        {0x010B,0x010B, 0},{0x010C,0x0118, 5},{0x0119,0x0119, 0},{0x011A,0x0123, 5},
        {0x0124,0x0159, 6},{0x015A,0x01C4, 5},{0x01C5,0x01F0, 0},
    };
    static constexpr bsl_region XBETA_BSL_LOCAL[] = {
        {0x0000,0x00B6, 0},{0x00B7,0x00CC,-1},{0x00CD,0x00CD, 0},{0x00CE,0x00EC,-1},
        {0x00ED,0x00ED, 0},{0x00EE,0x00F3,-1},{0x00F4,0x00F4, 0},{0x00F5,0x01F0,-1},
    };
    static constexpr size_t XBETA_BSL_GLOBAL_N = sizeof(XBETA_BSL_GLOBAL) / sizeof(XBETA_BSL_GLOBAL[0]);
    static constexpr size_t XBETA_BSL_LOCAL_N = sizeof(XBETA_BSL_LOCAL) / sizeof(XBETA_BSL_LOCAL[0]);

    static inline uint16_t bsl_remap(uint16_t idx, const bsl_region* r, size_t n) {
        for (size_t k = 0; k < n; ++k)
            if (idx >= r[k].lo && idx <= r[k].hi) return (uint16_t)((idx + r[k].delta) & 0xFFFF);
        return idx;
    }
    static inline uint16_t bsl_inverse_remap(uint16_t pc, const bsl_region* r, size_t n) {
        for (size_t k = 0; k < n; ++k) {
            const uint16_t sx = (uint16_t)((pc - r[k].delta) & 0xFFFF);
            if (sx >= r[k].lo && sx <= r[k].hi) return sx;
        }
        return pc;
    }

    static inline void apply_bsl_slc_remap(std::vector<uint8_t>& buf,
        uint32_t code_start, uint32_t code_size,
        bool forward,
        const bsl_region* gtab = PS2_BETA_BSL_GLOBAL,
        size_t gn = PS2_BETA_BSL_GLOBAL_N,
        const bsl_region* ltab = PS2_BETA_BSL_LOCAL,
        size_t ln = PS2_BETA_BSL_LOCAL_N,
        bsl_slc_update_stats* stats = nullptr)
    {
        if ((uint64_t)code_start + code_size > buf.size()) return;
        const uint32_t code_end = code_start + code_size;
        uint8_t* d = buf.data();
        for (uint32_t i = code_start; i + 5 < code_end; ++i) {
            if (d[i] != 0x0A || d[i + 1] != 0x04) continue;
            const uint16_t grp = rd16(d, i + 2);
            if (grp != 0x0000 && grp != 0x0007) continue;
            const uint16_t idx = rd16(d, i + 4);
            const bool is_global = (grp == 0x0000);
            const bsl_region* r = is_global ? gtab : ltab;
            const size_t     n = is_global ? gn : ln;
            const uint16_t   nv = forward ? bsl_remap(idx, r, n) : bsl_inverse_remap(idx, r, n);

            if (stats) {
                if (is_global) ++stats->global_refs;
                else           ++stats->local_refs;

                if (nv != idx) {
                    if (is_global) ++stats->changed_global;
                    else           ++stats->changed_local;
                }
                else {
                    if (is_global) ++stats->unchanged_global;
                    else           ++stats->unchanged_local;
                }
            }

            if (nv != idx) { d[i + 4] = (uint8_t)nv; d[i + 5] = (uint8_t)(nv >> 8); }
        }
    }


    static inline bool walk_ok(const uint8_t* d, size_t n,
        uint32_t se_size, uint32_t vme_size,
        uint32_t tail_slack = 16)
    {
        if (n < OBJ_OFF + se_size) return false;

        const uint32_t sx_size = rd32(d, OBJ_OFF + 0x24);
        const uint32_t total_so = rd32(d, OBJ_OFF + 0x30);
        const uint32_t perm_size = rd32(d, OBJ_OFF + 0x3C);
        const uint32_t field_58 = rd32(d, OBJ_OFF + 0x58);

        if (total_so == 0 || total_so > MAX_SO) return false;
        if (perm_size > MAX_PERM) return false;
        if (field_58 > MAX_INFO) return false;
        if (sx_size > n) return false;

        uint64_t p = OBJ_OFF + se_size; // 64-bit to catch overflow before truncation
        auto need = [&](uint64_t bytes) -> bool { return p + bytes <= n; };
        auto reb = [&](uint32_t a) { p = align_up((uint32_t)p, a); };

        reb(4); if (!need(sx_size)) return false; p += sx_size;
        reb(4); if (!need((uint64_t)4 * total_so)) return false; p += 4u * total_so;

        for (uint32_t i = 0; i < total_so; ++i) {
            reb(8); reb(4);
            if (!need(SO_SIZE)) return false;
            const uint32_t so = (uint32_t)p;
            const uint32_t m_size = rd32(d, so + 0x10);
            const uint32_t total_funcs = rd32(d, so + 0x24);
            if (total_funcs > MAX_FUNCS) return false;
            p += SO_SIZE;
            reb(4); if (!need(m_size)) return false; p += m_size;
            reb(4); if (!need((uint64_t)4 * total_funcs)) return false; p += 4u * total_funcs;
            for (uint32_t j = 0; j < total_funcs; ++j) {
                reb(4); if (!need(vme_size)) return false;
                const uint32_t buf_off = rd32(d, (uint32_t)p + 0x10);
                if (buf_off >= sx_size) return false;        // must index the bytecode image
                p += vme_size;
            }
        }

        reb(4); if (!need((uint64_t)4 * total_so)) return false; p += 4u * total_so;
        reb(4); if (!need((uint64_t)4 * perm_size)) return false; p += 4u * perm_size;
        for (uint32_t i = 0; i < perm_size; ++i) {
            reb(4); if (!need(4)) return false;
            const uint32_t len = rd32(d, (uint32_t)p);
            p += 4;
            if (!need(len)) return false;
            p += len;
        }
        // system_string_table: not stored in the mash (built at link time)
        reb(4); reb(4);
        if (!need((uint64_t)INFO_SIZE * field_58)) return false;
        const uint32_t info = (uint32_t)p; p += INFO_SIZE * field_58;
        reb(4);
        for (uint32_t i = 0; i < field_58; ++i) {
            const int32_t f10 = (int32_t)rd32(d, info + i * INFO_SIZE + 0x10);
            if (f10 == -1) {
                reb(4); if (!need(vme_size)) return false; p += vme_size;
            }
        }

        return (n - p) <= tail_slack;
    }


    // Automatically locate the bytecode image in a .ps2sx/.sx buffer and update
    // every PS2 beta BSL SLC reference found in it.
    // group 0x0000 = global SLC, group 0x0007 = local SLC.
    // The default direction is PS2 beta -> PC.
    static inline bool auto_update_ps2sx_slc(std::vector<uint8_t>& data,
        bool ps2_to_pc = true,
        bsl_slc_update_stats* stats = nullptr)
    {
        if (data.size() < OBJ_OFF + SE_PC) return false;

        uint32_t se_size = 0;
        if (walk_ok(data.data(), data.size(), SE_BETA, VME_BETA))
            se_size = SE_BETA;
        else if (walk_ok(data.data(), data.size(), SE_PC, VME_PC))
            se_size = SE_PC;
        else
            return false;

        const uint32_t sx_size = rd32(data.data(), OBJ_OFF + 0x24);
        apply_bsl_slc_remap(data, align_up(OBJ_OFF + se_size, 4), sx_size,
            /*forward=*/ps2_to_pc,
            PS2_BETA_BSL_GLOBAL, PS2_BETA_BSL_GLOBAL_N,
            PS2_BETA_BSL_LOCAL, PS2_BETA_BSL_LOCAL_N,
            stats);
        return true;
    }

    // Explicit PS2 beta wrapper. Same behavior as auto_update_ps2sx_slc(), but
    // keeps call sites clear when multiple platform tables are present.
    static inline bool auto_update_ps2_beta_slc(std::vector<uint8_t>& data,
        bool ps2_beta_to_pc = true,
        bsl_slc_update_stats* stats = nullptr)
    {
        return auto_update_ps2sx_slc(data, ps2_beta_to_pc, stats);
    }

    static inline bool auto_update_ps2_beta_slc(const uint8_t* d, size_t n,
        std::vector<uint8_t>& out,
        bool ps2_beta_to_pc = true,
        bsl_slc_update_stats* stats = nullptr)
    {
        if (!d || n == 0) return false;
        out.assign(d, d + n);
        if (!auto_update_ps2_beta_slc(out, ps2_beta_to_pc, stats)) {
            out.clear();
            return false;
        }
        return true;
    }

    static inline bool auto_update_ps2sx_slc(const uint8_t* d, size_t n,
        std::vector<uint8_t>& out,
        bool ps2_to_pc = true,
        bsl_slc_update_stats* stats = nullptr)
    {
        if (!d || n == 0) return false;
        out.assign(d, d + n);
        if (!auto_update_ps2sx_slc(out, ps2_to_pc, stats)) {
            out.clear();
            return false;
        }
        return true;
    }


    static inline bool transcode_beta_to_pc(const uint8_t* d, size_t n,
        std::vector<uint8_t>& out,
        bool remap_slc = true)
    {
        if (n < OBJ_OFF + SE_BETA) return false;

        const uint32_t sx_size = rd32(d, OBJ_OFF + 0x24);
        const uint32_t total_so = rd32(d, OBJ_OFF + 0x30);
        const uint32_t perm_size = rd32(d, OBJ_OFF + 0x3C);
        const uint32_t field_58 = rd32(d, OBJ_OFF + 0x58);

        if (total_so == 0 || total_so > MAX_SO) return false;
        if (perm_size > MAX_PERM || field_58 > MAX_INFO) return false;
        if (sx_size > n) return false;

        out.clear();
        out.reserve(n);

        uint64_t ip = 0;
        auto in_need = [&](uint64_t b) -> bool { return ip + b <= n; };
        auto reb_in = [&](uint32_t a) { ip = align_up((uint32_t)ip, a); };
        auto reb_out = [&](uint32_t a) {
            uint32_t v = a - ((uint32_t)out.size() % a);
            if (v < a) out.insert(out.end(), v, (uint8_t)0);
            };
        auto reb = [&](uint32_t a) { reb_in(a); reb_out(a); };
        auto copy = [&](uint32_t bytes) -> bool {
            if (!in_need(bytes)) return false;
            out.insert(out.end(), d + ip, d + ip + bytes); ip += bytes; return true;
            };
        // beta vm_executable -> PC: keep [0x00,0x1C), drop [0x1C,0x20), keep [0x20,0x28).
        auto emit_vme = [&]() -> bool {
            if (!in_need(VME_BETA)) return false;
            const uint32_t base = (uint32_t)ip;
            out.insert(out.end(), d + base, d + base + 0x1C);
            out.insert(out.end(), d + base + 0x20, d + base + 0x28);
            ip += VME_BETA;
            return true;
            };

        if (!copy(HDR_SIZE)) return false;
        if (!copy(SE_PC)) return false;
        ip += (SE_BETA - SE_PC);                              // drop SE extra @obj+0x5C

        reb(4); if (!copy(sx_size)) return false;
        reb(4); if (!copy(4u * total_so)) return false;

        for (uint32_t i = 0; i < total_so; ++i) {
            reb(8); reb(4);
            if (!in_need(SO_SIZE)) return false;
            const uint32_t so = (uint32_t)ip;
            const uint32_t m_size = rd32(d, so + 0x10);
            const uint32_t total_funcs = rd32(d, so + 0x24);
            if (total_funcs > MAX_FUNCS) return false;
            if (!copy(SO_SIZE)) return false;
            reb(4); if (!copy(m_size)) return false;
            reb(4); if (!copy(4u * total_funcs)) return false;
            for (uint32_t j = 0; j < total_funcs; ++j) {
                reb(4); if (!emit_vme()) return false;
            }
        }

        reb(4); if (!copy(4u * total_so)) return false;
        reb(4); if (!copy(4u * perm_size)) return false;
        for (uint32_t i = 0; i < perm_size; ++i) {
            reb(4); if (!in_need(4)) return false;
            const uint32_t len = rd32(d, (uint32_t)ip);
            if (!copy(4)) return false;
            if (!copy(len)) return false;
        }

        reb(4); reb(4);
        if (!in_need((uint64_t)INFO_SIZE * field_58)) return false;
        const uint32_t info = (uint32_t)ip;
        if (!copy(INFO_SIZE * field_58)) return false;
        reb(4);
        for (uint32_t i = 0; i < field_58; ++i) {
            const int32_t f10 = (int32_t)rd32(d, info + i * INFO_SIZE + 0x10);
            if (f10 == -1) {
                reb(4); if (!emit_vme()) return false;
            }
        }

        if (out.size() < HDR_SIZE) return false;
        const uint32_t new_size = (uint32_t)out.size();
        const uint32_t field_4 = rd32(out.data(), 0x04);
        const uint16_t class_id = rd16(out.data(), 0x0C);
        const uint16_t field_E = rd16(out.data(), 0x0E);
        std::memcpy(out.data() + 0x08, &new_size, 4);
        const uint32_t key = gen_safety_key(new_size, field_4, class_id, field_E);
        std::memcpy(out.data() + 0x00, &key, 4);


        if (remap_slc)
            apply_bsl_slc_remap(out, align_up(OBJ_OFF + SE_PC, 4), sx_size, /*forward=*/true);

        return true;
    }

    static inline bool transcode_pc_to_beta(const uint8_t* d, size_t n,
        std::vector<uint8_t>& out,
        bool remap_slc = true)
    {
        if (n < OBJ_OFF + SE_PC) return false;

        const uint32_t sx_size = rd32(d, OBJ_OFF + 0x24);
        const uint32_t total_so = rd32(d, OBJ_OFF + 0x30);
        const uint32_t perm_size = rd32(d, OBJ_OFF + 0x3C);
        const uint32_t field_58 = rd32(d, OBJ_OFF + 0x58);

        if (total_so == 0 || total_so > MAX_SO) return false;
        if (perm_size > MAX_PERM || field_58 > MAX_INFO) return false;
        if (sx_size > n) return false;

        out.clear();
        out.reserve(n + 64);

        uint64_t ip = 0;
        auto in_need = [&](uint64_t b) -> bool { return ip + b <= n; };
        auto reb_in = [&](uint32_t a) { ip = align_up((uint32_t)ip, a); };
        auto reb_out = [&](uint32_t a) {                     // beta output padding = 0xE3
            uint32_t v = a - ((uint32_t)out.size() % a);
            if (v < a) out.insert(out.end(), v, PAD_FILL);
            };
        auto reb = [&](uint32_t a) { reb_in(a); reb_out(a); };
        auto copy = [&](uint32_t bytes) -> bool {
            if (!in_need(bytes)) return false;
            out.insert(out.end(), d + ip, d + ip + bytes); ip += bytes; return true;
            };
        auto put32 = [&](uint32_t v) {
            const uint8_t b[4] = { (uint8_t)v, (uint8_t)(v >> 8), (uint8_t)(v >> 16), (uint8_t)(v >> 24) };
            out.insert(out.end(), b, b + 4);
            };
        // PC vm_executable -> beta: pc[0x00,0x1C) + VME_EXTRA_FILL + pc[0x1C,0x24).
        auto emit_vme = [&]() -> bool {
            if (!in_need(VME_PC)) return false;
            const uint32_t base = (uint32_t)ip;
            out.insert(out.end(), d + base, d + base + 0x1C);
            put32(VME_EXTRA_FILL);
            out.insert(out.end(), d + base + 0x1C, d + base + 0x24);
            ip += VME_PC;
            return true;
            };

        if (!copy(HDR_SIZE)) return false;
        if (!copy(SE_PC)) return false;
        put32(SE_EXTRA_FILL);                                // re-insert SE extra @obj+0x5C

        reb(4); if (!copy(sx_size)) return false;
        reb(4); if (!copy(4u * total_so)) return false;

        for (uint32_t i = 0; i < total_so; ++i) {
            reb(8); reb(4);
            if (!in_need(SO_SIZE)) return false;
            const uint32_t so = (uint32_t)ip;
            const uint32_t m_size = rd32(d, so + 0x10);
            const uint32_t total_funcs = rd32(d, so + 0x24);
            if (total_funcs > MAX_FUNCS) return false;
            if (!copy(SO_SIZE)) return false;
            reb(4); if (!copy(m_size)) return false;
            reb(4); if (!copy(4u * total_funcs)) return false;
            for (uint32_t j = 0; j < total_funcs; ++j) {
                reb(4); if (!emit_vme()) return false;
            }
        }

        reb(4); if (!copy(4u * total_so)) return false;
        reb(4); if (!copy(4u * perm_size)) return false;
        for (uint32_t i = 0; i < perm_size; ++i) {
            reb(4); if (!in_need(4)) return false;
            const uint32_t len = rd32(d, (uint32_t)ip);
            if (!copy(4)) return false;
            if (!copy(len)) return false;
        }

        reb(4); reb(4);
        if (!in_need((uint64_t)INFO_SIZE * field_58)) return false;
        const uint32_t info = (uint32_t)ip;
        if (!copy(INFO_SIZE * field_58)) return false;
        reb(4);
        for (uint32_t i = 0; i < field_58; ++i) {
            const int32_t f10 = (int32_t)rd32(d, info + i * INFO_SIZE + 0x10);
            if (f10 == -1) {
                reb(4); if (!emit_vme()) return false;
            }
        }

        // pad tail to a 16-byte boundary with 0xE3, then fix header size + safety_key
        {
            uint32_t v = TAIL_ALIGN - ((uint32_t)out.size() % TAIL_ALIGN);
            if (v < TAIL_ALIGN) out.insert(out.end(), v, PAD_FILL);
        }
        if (out.size() < HDR_SIZE) return false;
        const uint32_t new_size = (uint32_t)out.size();
        const uint32_t field_4 = rd32(out.data(), 0x04);
        const uint16_t class_id = rd16(out.data(), 0x0C);
        const uint16_t field_E = rd16(out.data(), 0x0E);
        std::memcpy(out.data() + 0x08, &new_size, 4);
        const uint32_t key = gen_safety_key(new_size, field_4, class_id, field_E);
        std::memcpy(out.data() + 0x00, &key, 4);

        if (remap_slc)
            apply_bsl_slc_remap(out, align_up(OBJ_OFF + SE_BETA, 4), sx_size, /*forward=*/false);

        return true;
    }


    static inline bool transcode_pc_to_xbox_build(const uint8_t* d, size_t n,
        std::vector<uint8_t>& out,
        bool remap_slc = true,
        const bsl_region* gtab = XB_BSL_GLOBAL,
        size_t gn = XB_BSL_GLOBAL_N,
        const bsl_region* ltab = XB_BSL_LOCAL,
        size_t ln = XB_BSL_LOCAL_N)
    {
        if (n < OBJ_OFF + SE_PC) return false;

        const uint32_t sx_size = rd32(d, OBJ_OFF + 0x24);
        const uint32_t total_so = rd32(d, OBJ_OFF + 0x30);
        const uint32_t perm_size = rd32(d, OBJ_OFF + 0x3C);
        const uint32_t field_58 = rd32(d, OBJ_OFF + 0x58);

        if (total_so == 0 || total_so > MAX_SO) return false;
        if (perm_size > MAX_PERM || field_58 > MAX_INFO) return false;
        if (sx_size > n) return false;

        out.clear();
        out.reserve(n + 64);

        uint64_t ip = 0;
        auto in_need = [&](uint64_t b) -> bool { return ip + b <= n; };
        auto reb_in = [&](uint32_t a) { ip = align_up((uint32_t)ip, a); };
        auto reb_out = [&](uint32_t a) {                 // Xbox output padding = 0xE3
            uint32_t v = a - ((uint32_t)out.size() % a);
            if (v < a) out.insert(out.end(), v, PAD_FILL);
            };
        auto reb = [&](uint32_t a) { reb_in(a); reb_out(a); };
        auto copy = [&](uint32_t bytes) -> bool {
            if (!in_need(bytes)) return false;
            out.insert(out.end(), d + ip, d + ip + bytes); ip += bytes; return true;
            };
        auto put32 = [&](uint32_t v) {
            const uint8_t b[4] = { (uint8_t)v, (uint8_t)(v >> 8), (uint8_t)(v >> 16), (uint8_t)(v >> 24) };
            out.insert(out.end(), b, b + 4);
            };

        // 1. header + script_executable PC struct, then re-insert the SE marker
        //    (0x5C -> 0x60). vm_executable size is unchanged, so funcs copy verbatim.
        if (!copy(HDR_SIZE)) return false;
        if (!copy(SE_PC)) return false;
        put32(SE_EXTRA_FILL);

        reb(4); if (!copy(sx_size)) return false;
        reb(4); if (!copy(4u * total_so)) return false;

        for (uint32_t i = 0; i < total_so; ++i) {
            reb(8); reb(4);
            if (!in_need(SO_SIZE)) return false;
            const uint32_t so = (uint32_t)ip;
            const uint32_t m_size = rd32(d, so + 0x10);
            const uint32_t total_funcs = rd32(d, so + 0x24);
            if (total_funcs > MAX_FUNCS) return false;
            if (!copy(SO_SIZE)) return false;
            reb(4); if (!copy(m_size)) return false;
            reb(4); if (!copy(4u * total_funcs)) return false;
            for (uint32_t j = 0; j < total_funcs; ++j) {
                reb(4); if (!copy(VME_PC)) return false;     // vm_executable verbatim (0x24)
            }
        }

        reb(4); if (!copy(4u * total_so)) return false;
        reb(4); if (!copy(4u * perm_size)) return false;
        for (uint32_t i = 0; i < perm_size; ++i) {
            reb(4); if (!in_need(4)) return false;
            const uint32_t len = rd32(d, (uint32_t)ip);
            if (!copy(4)) return false;
            if (!copy(len)) return false;
        }

        reb(4); reb(4);
        if (!in_need((uint64_t)INFO_SIZE * field_58)) return false;
        const uint32_t info = (uint32_t)ip;
        if (!copy(INFO_SIZE * field_58)) return false;
        reb(4);
        for (uint32_t i = 0; i < field_58; ++i) {
            const int32_t f10 = (int32_t)rd32(d, info + i * INFO_SIZE + 0x10);
            if (f10 == -1) {
                reb(4); if (!copy(VME_PC)) return false;
            }
        }

        // pad tail to a 16-byte boundary with 0xE3, then fix header size + safety_key
        {
            uint32_t v = TAIL_ALIGN - ((uint32_t)out.size() % TAIL_ALIGN);
            if (v < TAIL_ALIGN) out.insert(out.end(), v, PAD_FILL);
        }
        if (out.size() < HDR_SIZE) return false;
        const uint32_t new_size = (uint32_t)out.size();
        const uint32_t field_4 = rd32(out.data(), 0x04);
        const uint16_t class_id = rd16(out.data(), 0x0C);
        const uint16_t field_E = rd16(out.data(), 0x0E);
        std::memcpy(out.data() + 0x08, &new_size, 4);
        const uint32_t key = gen_safety_key(new_size, field_4, class_id, field_E);
        std::memcpy(out.data() + 0x00, &key, 4);

        if (remap_slc)
            apply_bsl_slc_remap(out, align_up(OBJ_OFF + SE_BETA, 4), sx_size, /*forward=*/false,
                gtab, gn, ltab, ln);

        return true;
    }

    static inline bool transcode_pc_to_xbox_beta_build(const uint8_t* d, size_t n,
        std::vector<uint8_t>& out,
        bool remap_slc = true)
    {
        return transcode_pc_to_xbox_build(d, n, out, remap_slc,
            XBETA_BSL_GLOBAL, XBETA_BSL_GLOBAL_N,
            XBETA_BSL_LOCAL, XBETA_BSL_LOCAL_N);
    }


    static inline bool transcode_xbox_build_to_pc(const uint8_t* d, size_t n,
        std::vector<uint8_t>& out,
        bool remap_slc = true,
        const bsl_region* gtab = XB_BSL_GLOBAL,
        size_t gn = XB_BSL_GLOBAL_N,
        const bsl_region* ltab = XB_BSL_LOCAL,
        size_t ln = XB_BSL_LOCAL_N)
    {
        if (n < OBJ_OFF + SE_BETA) return false;

        const uint32_t sx_size = rd32(d, OBJ_OFF + 0x24);
        const uint32_t total_so = rd32(d, OBJ_OFF + 0x30);
        const uint32_t perm_size = rd32(d, OBJ_OFF + 0x3C);
        const uint32_t field_58 = rd32(d, OBJ_OFF + 0x58);

        if (total_so == 0 || total_so > MAX_SO) return false;
        if (perm_size > MAX_PERM || field_58 > MAX_INFO) return false;
        if (sx_size > n) return false;

        out.clear();
        out.reserve(n);

        uint64_t ip = 0;
        auto in_need = [&](uint64_t b) -> bool { return ip + b <= n; };
        auto reb_in = [&](uint32_t a) { ip = align_up((uint32_t)ip, a); };
        auto reb_out = [&](uint32_t a) {                 // PC output padding = 0xE3
            uint32_t v = a - ((uint32_t)out.size() % a);
            if (v < a) out.insert(out.end(), v, PAD_FILL);
            };
        auto reb = [&](uint32_t a) { reb_in(a); reb_out(a); };
        auto copy = [&](uint32_t bytes) -> bool {
            if (!in_need(bytes)) return false;
            out.insert(out.end(), d + ip, d + ip + bytes); ip += bytes; return true;
            };


        if (!copy(HDR_SIZE)) return false;
        if (!copy(SE_PC)) return false;
        ip += (SE_BETA - SE_PC);                         // skip 0xA1A1A1A1 marker

        reb(4); if (!copy(sx_size)) return false;
        reb(4); if (!copy(4u * total_so)) return false;

        for (uint32_t i = 0; i < total_so; ++i) {
            reb(8); reb(4);
            if (!in_need(SO_SIZE)) return false;
            const uint32_t so = (uint32_t)ip;
            const uint32_t m_size = rd32(d, so + 0x10);
            const uint32_t total_funcs = rd32(d, so + 0x24);
            if (total_funcs > MAX_FUNCS) return false;
            if (!copy(SO_SIZE)) return false;
            reb(4); if (!copy(m_size)) return false;
            reb(4); if (!copy(4u * total_funcs)) return false;
            for (uint32_t j = 0; j < total_funcs; ++j) {
                reb(4); if (!copy(VME_PC)) return false;     // vm_executable verbatim (0x24)
            }
        }

        reb(4); if (!copy(4u * total_so)) return false;
        reb(4); if (!copy(4u * perm_size)) return false;
        for (uint32_t i = 0; i < perm_size; ++i) {
            reb(4); if (!in_need(4)) return false;
            const uint32_t len = rd32(d, (uint32_t)ip);
            if (!copy(4)) return false;
            if (!copy(len)) return false;
        }

        reb(4); reb(4);
        if (!in_need((uint64_t)INFO_SIZE * field_58)) return false;
        const uint32_t info = (uint32_t)ip;
        if (!copy(INFO_SIZE * field_58)) return false;
        reb(4);
        for (uint32_t i = 0; i < field_58; ++i) {
            const int32_t f10 = (int32_t)rd32(d, info + i * INFO_SIZE + 0x10);
            if (f10 == -1) {
                reb(4); if (!copy(VME_PC)) return false;
            }
        }

        // NO tail pad: PC layout has none (matches transcode_beta_to_pc).
        if (out.size() < HDR_SIZE) return false;
        const uint32_t new_size = (uint32_t)out.size();
        const uint32_t field_4 = rd32(out.data(), 0x04);
        const uint16_t class_id = rd16(out.data(), 0x0C);
        const uint16_t field_E = rd16(out.data(), 0x0E);
        std::memcpy(out.data() + 0x08, &new_size, 4);
        const uint32_t key = gen_safety_key(new_size, field_4, class_id, field_E);
        std::memcpy(out.data() + 0x00, &key, 4);

        // Xbox -> PC SLC index remap (forward). PC-layout bytecode sits at
        // align4(header + script_executable) = 0x6C. Table set defaults to retail
        // XB; pass XBETA_* for the beta build.
        if (remap_slc)
            apply_bsl_slc_remap(out, align_up(OBJ_OFF + SE_PC, 4), sx_size, /*forward=*/true,
                gtab, gn, ltab, ln);

        return true;
    }


    static inline bool transcode_xbox_beta_build_to_pc(const uint8_t* d, size_t n,
        std::vector<uint8_t>& out,
        bool remap_slc = true)
    {
        return transcode_xbox_build_to_pc(d, n, out, remap_slc,
            XBETA_BSL_GLOBAL, XBETA_BSL_GLOBAL_N,
            XBETA_BSL_LOCAL, XBETA_BSL_LOCAL_N);
    }

    static inline bool transcode_if_beta(std::vector<uint8_t>& data)
    {
        const uint8_t* d = data.data();
        const size_t   n = data.size();

        if (n < OBJ_OFF + SE_PC) return false;
        if (walk_ok(d, n, SE_PC, VME_PC)) return false;       // already valid PC layout -> pass through

        std::vector<uint8_t> out;
        if (!transcode_beta_to_pc(d, n, out)) return false;               // not a beta SX we understand
        if (!walk_ok(out.data(), out.size(), SE_PC, VME_PC)) return false; // result must be valid PC

        data.swap(out);
        return true;
    }


    static inline bool transcode_to_beta_if_pc(std::vector<uint8_t>& data)
    {
        const uint8_t* d = data.data();
        const size_t   n = data.size();

        if (n < OBJ_OFF + SE_PC) return false;
        if (!walk_ok(d, n, SE_PC, VME_PC)) return false;        // not valid PC layout -> leave alone
        if (walk_ok(d, n, SE_BETA, VME_BETA)) return false;     // already parses as beta -> leave alone

        std::vector<uint8_t> out;
        if (!transcode_pc_to_beta(d, n, out)) return false;
        if (!walk_ok(out.data(), out.size(), SE_BETA, VME_BETA)) return false; // result must be valid beta

        data.swap(out);
        return true;
    }


    struct sx_sections {
        bool beta_layout = false;        // true: PS2 beta (SE 0x60); false: final PC (SE 0x5C)
        std::vector<uint8_t> head;
        std::vector<uint8_t> data1;
        std::vector<uint8_t> obj_ptrs;
        std::vector<uint8_t> remaining;
    };


    static inline bool split_sections(const uint8_t* d, size_t n, sx_sections& out)
    {
        uint32_t se_size;
        if (walk_ok(d, n, SE_PC, VME_PC)) { se_size = SE_PC;   out.beta_layout = false; }
        else if (walk_ok(d, n, SE_BETA, VME_BETA)) { se_size = SE_BETA; out.beta_layout = true; }
        else return false;

        const uint32_t sx_size = rd32(d, OBJ_OFF + 0x24);
        const uint32_t total_so = rd32(d, OBJ_OFF + 0x30);

        const uint32_t head_end = OBJ_OFF + se_size;       // always 4-aligned (0x6C / 0x70)
        const uint32_t d1_start = align_up(head_end, 4);
        const uint64_t d1_end = (uint64_t)d1_start + sx_size;
        const uint32_t op_start = align_up((uint32_t)d1_end, 4);
        const uint64_t op_end = (uint64_t)op_start + 4ull * total_so;
        if (op_end > n) return false;                      // walk_ok guarantees this; belt and suspenders

        out.head.assign(d, d + d1_start);
        out.data1.assign(d + d1_start, d + (size_t)d1_end);
        out.obj_ptrs.assign(d + op_start, d + (size_t)op_end);
        out.remaining.assign(d + (size_t)op_end, d + n);
        return true;
    }


    static inline bool assemble_sections(const sx_sections& s, std::vector<uint8_t>& out)
    {
        const size_t head_sz = s.head.size();
        if (head_sz != OBJ_OFF + SE_PC && head_sz != OBJ_OFF + SE_BETA) return false;
        if ((s.obj_ptrs.size() % 4) != 0) return false;

        out = s.head;                                       // header + script_executable struct

        const uint32_t sx_size = (uint32_t)s.data1.size();
        const uint32_t tso = (uint32_t)(s.obj_ptrs.size() / 4);
        std::memcpy(out.data() + OBJ_OFF + 0x24, &sx_size, 4); // sx_exe_image_size
        std::memcpy(out.data() + OBJ_OFF + 0x30, &tso, 4);     // total_script_objects

        out.insert(out.end(), s.data1.begin(), s.data1.end());
        while (out.size() % 4) out.push_back(PAD_FILL);     // rebase4 before obj_ptrs (0xE3)
        out.insert(out.end(), s.obj_ptrs.begin(), s.obj_ptrs.end());
        out.insert(out.end(), s.remaining.begin(), s.remaining.end());

        if (out.size() < HDR_SIZE) return false;
        const uint32_t nsz = (uint32_t)out.size();
        const uint32_t field_4 = rd32(out.data(), 0x04);
        const uint16_t class_id = rd16(out.data(), 0x0C);
        const uint16_t field_E = rd16(out.data(), 0x0E);
        std::memcpy(out.data() + 0x08, &nsz, 4);
        const uint32_t key = gen_safety_key(nsz, field_4, class_id, field_E);
        std::memcpy(out.data() + 0x00, &key, 4);
        return true;
    }

} // namespace sx



