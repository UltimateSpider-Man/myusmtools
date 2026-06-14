#pragma once

#include <unordered_map>
#include <filesystem>

#include "common.h"
#include "sx_transcode.h"


static uint32_t current_pack = -1;

constexpr uint32_t rootDir = 0xFFFFFFFF;



struct Mod {
    std::filesystem::path Path;
    int Type;
    std::vector<uint8_t> Data;
};

static std::unordered_map<uint64_t, std::vector<Mod>> Mods;



static inline uint64_t make_key(uint32_t nameHash, uint32_t dirHash = rootDir) {
    return (uint64_t(dirHash) << 32) | nameHash;
}



[[maybe_unused]] static std::string transformToLower(const std::string& name)
{
    std::string res = name;
    std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c) { return std::tolower(c); });
    return res;
}

[[maybe_unused]] static std::string transformToUpper(const std::string& name)
{
    std::string res = name;
    std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c) { return std::toupper(c); });
    return res;
}

static std::vector<uint8_t> read_file(const std::filesystem::path& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file)
        return std::vector<uint8_t>();

    file.seekg(0, std::ios::end);
    std::streamsize sz = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(sz);
    buffer.resize(sz); buffer.reserve(sz);
    file.read(reinterpret_cast<char*>(buffer.data()), sz);
    return buffer;
}


inline constexpr unsigned char to_lower(unsigned char c) {
    constexpr auto delta = 'a' - 'A';

    if (c >= 'A' && c <= 'Z') {
        return (c + delta);
    }

    return c;
}

inline constexpr bool is_alpha(unsigned char c) {
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
        return true;
    }

    return false;
}

constexpr inline std::uint32_t to_hash(const char* str) {
    std::uint32_t res = 0;

    for (int c = *str; c != '\0'; ++str, c = *str) {
        int ch_lower = [](auto c) -> int {
            if (is_alpha(c)) {
                return to_lower(c);
            }

            return c;
            }(c);

        res = ch_lower + 33 * res;
    }

    return res;
}

Mod* findModOfExt(uint64_t hash, const char* expected_ext) {

    auto it = Mods.find(hash);
    if (it == Mods.end()) return nullptr;
    for (auto& mod : it->second) {
        if (transformToUpper(mod.Path.extension().string()) == transformToUpper(std::string(expected_ext)))
            return &mod;
    }
    return nullptr;
}


static inline bool is_moddable_resource_type(uint32_t type)
{
    switch (type) {
    case 1:   // .PCANIM animation     (self-sizing; via formats.h hook)
    case 2:   // .PCSKEL skeleton      (self-sizing; via formats.h hook)
    case 6:   // .DDS    texture
    case 7:   // .DDSMP  texture (mip-mapped)
    case 15:  // .PCSX   compiled script executable (self-sizing mash)
    case 21:  // .PCMESH mesh          (self-sizing; via formats.h hook)
        return true;
    default:
        return false;
    }
}

Mod* getModOfType2(const resource_key* resource_id, uint32_t dirHash)
{
    const uint32_t type = resource_id->m_type;

    // Never substitute resource types that aren't safe to override loose.
    if (!is_moddable_resource_type(type))
        return nullptr;

    const uint32_t nameHash = resource_id->m_hash;

    // External extensions accepted for this type, in priority order. The
    // engine asks for a compiled script as .PCSX (PC pack extension), but the
    // loose external file is typically the PS2 beta's .ps2sx; accept either so
    // a dropped-in VENOM.PS2SX matches the engine's VENOM script request.
    const char* cand[1];
    int ncand = 0;
    if (type == 15) {                 // RESOURCE_KEY_TYPE_SCRIPT (.PCSX / .PS2SX)
        cand[ncand++] = ".PCSX";
    }
    else {
        const char* e = resource_key_type_ext[PLATFORM][type];
        if (e) cand[ncand++] = e;
    }

    for (int i = 0; i < ncand; ++i) {
        Mod* m = findModOfExt(make_key(nameHash, dirHash), cand[i]);
        if (!m)  // fallback to root dir
            m = findModOfExt(make_key(nameHash), cand[i]);
        if (!m)
            continue;

        return m;
    }
    return nullptr;
}

Mod* getModOfType_build_xbox(const resource_key* resource_id, uint32_t dirHash)
{
    const uint32_t type = resource_id->m_type;

    // Never substitute resource types that aren't safe to override loose.
    if (!is_moddable_resource_type(type))
        return nullptr;

    const uint32_t nameHash = resource_id->m_hash;

    // External extensions accepted for this type, in priority order. The
    // engine asks for a compiled script as .PCSX (PC pack extension), but the
    // loose external file is typically the PS2 beta's .ps2sx; accept either so
    // a dropped-in VENOM.PS2SX matches the engine's VENOM script request.
    const char* cand[1];
    int ncand = 0;
    if (type == 15) {                 // RESOURCE_KEY_TYPE_SCRIPT (.XBSX)
        cand[ncand++] = ".XBOXSX";
    }
    else {
        const char* e = resource_key_type_ext[PLATFORM][type];
        if (e) cand[ncand++] = e;
    }

    for (int i = 0; i < ncand; ++i) {
        Mod* m = findModOfExt(make_key(nameHash, dirHash), cand[i]);
        if (!m)  // fallback to root dir
            m = findModOfExt(make_key(nameHash), cand[i]);
        if (!m)
            continue;


        if (type == 15 &&
            m->Data.size() > (sx::OBJ_OFF + sx::SE_BETA) &&
            sx::rd32(m->Data.data(), 0x6C) == sx::SE_EXTRA_FILL &&
            sx::walk_ok(m->Data.data(), m->Data.size(), sx::SE_BETA, sx::VME_PC)) {
            std::vector<uint8_t> pc;

            if (sx::transcode_xbox_build_to_pc(m->Data.data(), m->Data.size(), pc))
                m->Data.swap(pc);
        }

        return m;
    }
    return nullptr;
}

Mod* getModOfType_beta_xbox(const resource_key* resource_id, uint32_t dirHash)
{
    const uint32_t type = resource_id->m_type;

    // Never substitute resource types that aren't safe to override loose.
    if (!is_moddable_resource_type(type))
        return nullptr;

    const uint32_t nameHash = resource_id->m_hash;

    // External extensions accepted for this type, in priority order. The
    // engine asks for a compiled script as .PCSX (PC pack extension), but the
    // loose external file is typically the PS2 beta's .ps2sx; accept either so
    // a dropped-in VENOM.PS2SX matches the engine's VENOM script request.
    const char* cand[1];
    int ncand = 0;
    if (type == 15) {                 // RESOURCE_KEY_TYPE_SCRIPT (.XBSX)
        cand[ncand++] = ".XBSX";
    }
    else {
        const char* e = resource_key_type_ext[PLATFORM][type];
        if (e) cand[ncand++] = e;
    }

    for (int i = 0; i < ncand; ++i) {
        Mod* m = findModOfExt(make_key(nameHash, dirHash), cand[i]);
        if (!m)  // fallback to root dir
            m = findModOfExt(make_key(nameHash), cand[i]);
        if (!m)
            continue;

        // Compiled scripts: the loose external for the Xbox path is the Xbox
        // BETA build's .XBSX (script_executable 0x60 + 0xA1A1A1A1 marker,
        // vm_executable 0x24). The engine's un_mash expects PC .PCSX layout
        // (script_executable 0x5C, no marker); fed raw, the un_mash cursor lands
        // on the marker and dereferences an unrelocated placeholder -> crash.
        // Convert Xbox-beta -> PC in place once, before the engine sees it,
        // remapping the global+local SLC indices to PC (XBETA_* tables). The
        // marker test (script_executable extra @obj+0x5C = file+0x6C) gates the
        // conversion, and the (0x60,0x24) walk confirms it is genuinely an Xbox
        // build: a dropped real .PCSX (no marker) or an already-converted buffer
        // (re-request of the same mod) is a no-op, so we never double-transcode.
        // See sx_transcode.h.
        if (type == 15 &&
            m->Data.size() > (sx::OBJ_OFF + sx::SE_BETA) &&
            sx::rd32(m->Data.data(), 0x6C) == sx::SE_EXTRA_FILL &&
            sx::walk_ok(m->Data.data(), m->Data.size(), sx::SE_BETA, sx::VME_PC)) {
            std::vector<uint8_t> pc;
            if (sx::transcode_xbox_beta_build_to_pc(m->Data.data(), m->Data.size(), pc))
                m->Data.swap(pc);
        }

        return m;
    }
    return nullptr;
}

Mod* getModOfType(const resource_key* resource_id, uint32_t dirHash)
{
    const uint32_t type = resource_id->m_type;

    // Never substitute resource types that aren't safe to override loose.
    if (!is_moddable_resource_type(type))
        return nullptr;

    const uint32_t nameHash = resource_id->m_hash;

    // External extensions accepted for this type, in priority order. The
    // engine asks for a compiled script as .PCSX (PC pack extension), but the
    // loose external file is typically the PS2 beta's .ps2sx; accept either so
    // a dropped-in VENOM.PS2SX matches the engine's VENOM script request.
    const char* cand[1];
    int ncand = 0;
    if (type == 15) {                 // RESOURCE_KEY_TYPE_SCRIPT (.PCSX / .PS2SX)
        cand[ncand++] = ".PS2SX";
    }
    else {
        const char* e = resource_key_type_ext[PLATFORM][type];
        if (e) cand[ncand++] = e;
    }

    for (int i = 0; i < ncand; ++i) {
        Mod* m = findModOfExt(make_key(nameHash, dirHash), cand[i]);
        if (!m)  // fallback to root dir
            m = findModOfExt(make_key(nameHash), cand[i]);
        if (!m)
            continue;

        // Compiled scripts: a loose PS2-beta .PS2SX has a different struct
        // layout (script_executable +4, each vm_executable +4) than the final
        // PC .PCSX the engine's un_mash expects. Fed raw, the un_mash cursor
        // lands 4 bytes early and dereferences an unrelocated placeholder ->
        // crash. Convert it to PC layout in place once, before the engine sees
        // it. No-op (returns false) for files already in PC layout, so dropping
        // a real .PCSX still works. See sx_transcode.h.
        if (type == 15)
            sx::transcode_if_beta(m->Data);
        //  sx::transcode_to_beta_if_pc(m->Data);

        return m;
    }
    return nullptr;
}


static inline Mod* getModForFormat(uint32_t nameHash, uint32_t type, uint32_t dirHash)
{
    const char* ext = resource_key_type_ext[PLATFORM][type];
    if (!ext)
        return nullptr;

    // Active pack dir first, then fall back to the mods root.
    Mod* m = findModOfExt(make_key(nameHash, dirHash), ext);
    if (!m)
        m = findModOfExt(make_key(nameHash), ext);
    return m;
}
static inline Mod* findModByExt(uint32_t nameHash, const char* ext)
{
    Mod* m = findModOfExt(make_key(nameHash, current_pack), ext);
    if (!m)
        m = findModOfExt(make_key(nameHash), ext);
    return m;
}







