#pragma once
#include "Mod.h"
#include "sx_transcode.h"

#include <string_view>

namespace fs = std::filesystem;

static inline int mod_resource_type_from_extension(const std::string& extLower)
{
    for (int type = 0; type < 70; ++type) {
        const char* knownExt = resource_key_type_ext[PLATFORM][type];
        if (knownExt && transformToLower(knownExt) == extLower)
            return type;
    }

    // The texture hook can still feed TGA data through the DDS/TM2 path.
    if (extLower == ".tga")
        return 6;

    return 0;
}

static inline bool is_pcmesh_ext(const char* ext)
{
    return ext && transformToUpper(std::string(ext)) == ".PCMESH";
}

static inline void* game_tlMemAlloc(size_t size, size_t alignment, unsigned int flags)
{
    using tlMemAlloc_t = void* (__cdecl*)(size_t, size_t, unsigned int);
    return reinterpret_cast<tlMemAlloc_t>(0x0074A5C0)(size, alignment, flags);
}

#if 0
static void enumerate_mods() {
    fs::path modsDir = fs::current_path() / "mods";
    if (!fs::is_directory(modsDir))
        return;

    for (const auto& entry : fs::directory_iterator(modsDir)) {
        if (entry.is_regular_file()) {
            const fs::path& path = entry.path();
            std::vector<uint8_t> fileData = read_file(path);

            int resType = 0;
            std::string ext = transformToLower(path.extension().string());
            if (ext == ".dds" || ext == ".tga")
                resType = 1;    // @todo
            auto hash = to_hash(path.stem().string().c_str());
            Mods[hash] = Mod{ path, resType, std::move(fileData) };
            printf(__FUNCTION__ ": found name = %s\nhash = 0x%08X\n", path.stem().string().c_str(), hash);
        }
    }
}
#else
static void enumerate_mods() {
    namespace fs = std::filesystem;

    const fs::path modsDir = fs::current_path() / "mods";
    if (!fs::is_directory(modsDir))
        return;

    const fs::path modsDirNorm = modsDir.lexically_normal();

    for (const auto& entry : fs::recursive_directory_iterator(modsDir)) {
        if (!entry.is_regular_file())
            continue;

        const fs::path& path = entry.path();
        const fs::path parent = path.parent_path();

        uint32_t dirHash;
        if (parent.lexically_normal() == modsDirNorm)
            dirHash = rootDir;
        else
            dirHash = to_hash(parent.filename().string().c_str());

        std::vector<uint8_t> fileData = read_file(path);

        const std::string ext = transformToLower(path.extension().string());
        const int resType = mod_resource_type_from_extension(ext);

        const uint32_t nameHash = to_hash(path.stem().string().c_str());
        const uint64_t pathHash = make_key(nameHash, dirHash);

        Mods[pathHash].push_back(Mod{ path, resType, std::move(fileData) });
        printf(__FUNCTION__ ": found %s as 0x%08X\\0x%08X%s (%zu bytes)\n",
            path.string().c_str(), dirHash, nameHash, ext.c_str(), Mods[pathHash].back().Data.size());

    }
}


inline fs::path redirect_to_mods(std::string_view inPath)
{
    fs::path input(inPath);
    fs::path base = fs::current_path();
    fs::path mods = base / "mods";

    if (!input.is_absolute()) {
        return (fs::path("mods") / input).lexically_normal();
    }

    fs::path input_can, base_can;
    try {
        input_can = fs::weakly_canonical(input);
        base_can = fs::weakly_canonical(base);
    }
    catch (...) {
        return (mods / input.filename()).lexically_normal();
    }

    fs::path rel = fs::relative(input_can, base_can);

    bool is_under_base = true;
    for (const auto& part : rel) {
        if (part == "..") {
            is_under_base = false;
            break;
        }
    }

    if (is_under_base && !rel.empty())
        return (mods / rel).lexically_normal();
    return (mods / input_can.filename()).lexically_normal();
}
#endif

// texture reads
typedef bool(__cdecl* nglLoadTextureTM2_t)(char* tex, uint8_t* a2);
nglLoadTextureTM2_t nglLoadTextureTM2;

bool hk_nglLoadTextureTM2(char* tex, uint8_t* a2)
{
    uint32_t hash = *(uint32_t*)((char*)(tex + 0x60));
    resource_key key;
    key.m_hash = hash;
    key.m_type = 6;
    printf(__FUNCTION__ ": searching for 0x%08X\\0x%08X%s\n", current_pack, key.m_hash, resource_key_type_ext[PLATFORM][key.m_type]);

    if (auto mod = getModOfType(&key, current_pack)) {
        printf(__FUNCTION__ ": found %s\n", mod->Path.string().c_str());
        a2 = mod->Data.data();
    }

    return nglLoadTextureTM2(tex, a2);
}


typedef unsigned __int8* (__cdecl* get_resource_t)(const resource_key*, int*, resource_pack_slot**);
get_resource_t get_resource_orig;

unsigned __int8* __cdecl hk_get_resource(const resource_key* resource_id, int* size, resource_pack_slot** slot) {
    const uint32_t req_hash = resource_id->m_hash;
    const char* req_ext = resource_key_type_ext[PLATFORM][resource_id->m_type];

    uint8_t* ret = get_resource_orig(resource_id, size, slot);

    printf(__FUNCTION__ ": searching for 0x%08X\\0x%08X%s\n", current_pack, req_hash, req_ext);
    auto mod = getModOfType(resource_id, current_pack);
    if (mod) {
        printf(__FUNCTION__ ": found %s\n", mod->Path.string().c_str());

        if (size) *size = static_cast<int>(mod->Data.size());
        return mod->Data.data();
    }
    auto mod2 = getModOfType2(resource_id, current_pack);
    if (mod2) {
        printf(__FUNCTION__ ": found %s\n", mod2->Path.string().c_str());

        if (size) *size = static_cast<int>(mod2->Data.size());
        return mod2->Data.data();
    }
    if (Mod* mod3 = getModOfType_beta_xbox(resource_id, current_pack)) {
        printf(__FUNCTION__ ": found %s\n", mod3->Path.string().c_str());

        if (size) *size = static_cast<int>(mod3->Data.size());
        return mod3->Data.data();
    }
    if (Mod* mod6 = getModOfType_build_xbox(resource_id, current_pack)) {
        printf(__FUNCTION__ ": found %s\n", mod6->Path.string().c_str());

        if (size) *size = static_cast<int>(mod6->Data.size());
        return mod6->Data.data();
    }
    return ret;
}

typedef unsigned __int8* (__thiscall* get_resource_dir_t)(resource_directory*, const resource_key*, int*, resource_pack_slot**);
get_resource_dir_t get_resource_dir_orig;

unsigned __int8* __fastcall hk_get_resource_dir(resource_directory* self, void* edx, const resource_key* resource_id, int* size, resource_pack_slot** slot)
{
    uint32_t req_hash = resource_id->m_hash;
    uint32_t type = resource_id->m_type;
    const char* req_ext = resource_key_type_ext[PLATFORM][type];

    uint8_t* ret = get_resource_dir_orig(self, resource_id, size, slot);

    printf(__FUNCTION__ ": searching for 0x%08X\\0x%08X%s\n", current_pack, req_hash, req_ext);
    if (Mod* mod = getModOfType(resource_id, current_pack)) {
        printf(__FUNCTION__ ": found %s\n", mod->Path.string().c_str());

        if (size) *size = static_cast<int>(mod->Data.size());
        return mod->Data.data();
    }
    if (Mod* mod2 = getModOfType2(resource_id, current_pack)) {
        printf(__FUNCTION__ ": found %s\n", mod2->Path.string().c_str());

        if (size) *size = static_cast<int>(mod2->Data.size());
        return mod2->Data.data();
    }
    if (Mod* mod3 = getModOfType_beta_xbox(resource_id, current_pack)) {
        printf(__FUNCTION__ ": found %s\n", mod3->Path.string().c_str());

        if (size) *size = static_cast<int>(mod3->Data.size());
        return mod3->Data.data();
    }
    if (Mod* mod6 = getModOfType_build_xbox(resource_id, current_pack)) {
        printf(__FUNCTION__ ": found %s\n", mod6->Path.string().c_str());

        if (size) *size = static_cast<int>(mod6->Data.size());
        return mod6->Data.data();
    }
    return ret;
}



typedef bool(__cdecl* nglLoadMeshFileInternal_t)(const tlFixedString* name, nglMeshFile* meshFile, const char* ext);
nglLoadMeshFileInternal_t nglLoadMeshFileInternal_orig;

static bool load_mod_pcmesh_into_filebuf(const tlFixedString* name, nglMeshFile* meshFile, const char* ext)
{
    if (!name || !meshFile || !is_pcmesh_ext(ext))
        return false;

    Mod* mod = findModByExt(name->m_hash, ext);
    if (!mod)
        mod = getModForFormat(name->m_hash, 21, current_pack); // RESOURCE_KEY_TYPE_MESH_FILE_STRUCT / .PCMESH
    if (!mod || mod->Data.empty())
        return false;

    uint8_t* meshCopy = static_cast<uint8_t*>(game_tlMemAlloc(mod->Data.size(), 4u, 0));
    if (!meshCopy)
        return false;

    std::memcpy(meshCopy, mod->Data.data(), mod->Data.size());
    meshFile->FileBuf.Buf = meshCopy;
    meshFile->FileBuf.Size = static_cast<uint32_t>(mod->Data.size());
    meshFile->FileBuf.UserData = 0;

    printf(__FUNCTION__ ": using %s for 0x%08X%s (%zu bytes)\n",
        mod->Path.string().c_str(), name->m_hash, ext, mod->Data.size());
    return true;
}

bool __cdecl hk_nglLoadMeshFileInternal(const tlFixedString* name, nglMeshFile* meshFile, const char* ext)
{
    if (is_pcmesh_ext(ext) && name) {
        printf(__FUNCTION__ ": searching for 0x%08X%s (%s)\n",
            name->m_hash, ext, name->c_str());
        load_mod_pcmesh_into_filebuf(name, meshFile, ext);
    }

    return nglLoadMeshFileInternal_orig(name, meshFile, ext);
}


typedef void(__cdecl* set_active_resource_context_t)(resource_pack_slot* pack_slot);
set_active_resource_context_t set_active_resource_context_orig;

void hk_set_active_resource_context(resource_pack_slot* pack_slot)
{
    set_active_resource_context_orig(pack_slot);
    if (pack_slot)
        current_pack = pack_slot->field_4.m_hash;
}


typedef int(__cdecl* nflopenfile_t)(int, const char*);
nflopenfile_t nflopenfile_orig;

int hk_nflopenfile(int type, const char* str)
{
    auto modPath = redirect_to_mods(str);
    printf("searching for %s\n", modPath.string().c_str());
    if (fs::exists(modPath)) {
        printf("found %s\n", modPath.string().c_str());
        return nflopenfile_orig(type, modPath.string().c_str());
    }
    return nflopenfile_orig(type, str);
}