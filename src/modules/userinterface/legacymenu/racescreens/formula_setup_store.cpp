#include "formula_setup_store.h"
#include <cjson/cJSON.h>
#include <rhash.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <set>
#include <sstream>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif

namespace FormulaSetups {
namespace {
using Json = std::unique_ptr<cJSON, decltype(&cJSON_Delete)>;
Json parse(const std::string& source) {
    return Json(cJSON_ParseWithOpts(source.c_str(), nullptr, 1), cJSON_Delete);
}
const cJSON* at(const cJSON* root, const std::string& path) {
    std::istringstream parts(path);
    std::string part;
    while (root && std::getline(parts, part, '.')) root = cJSON_GetObjectItemCaseSensitive(root, part.c_str());
    return root;
}
std::string stringAt(const cJSON* root, const char* key) {
    const auto* value = cJSON_GetObjectItemCaseSensitive(root, key);
    return cJSON_IsString(value) && value->valuestring ? value->valuestring : "";
}
bool exists(const std::string& path) { struct stat s; return stat(path.c_str(), &s) == 0; }
bool uniqueKeys(const cJSON* object) {
    std::set<std::string> keys;
    for (const cJSON* child = object ? object->child : nullptr; child; child = child->next)
        if (!child->string || !keys.insert(child->string).second) return false;
    return true;
}
}

bool context(const std::string& document, const std::vector<PhysicsSetupField>& fields,
             const std::string& car, const std::string& track, Context& out, std::string& error) {
    auto root = parse(document);
    const auto* base = cJSON_GetObjectItemCaseSensitive(root.get(), "base");
    const auto* current = cJSON_GetObjectItemCaseSensitive(root.get(), "document");
    if (!cJSON_IsObject(base) || !cJSON_IsObject(current) || car.empty() || track.empty()) {
        error = "Perfil base de reglajes no disponible"; return false;
    }
    char* serialized = cJSON_PrintUnformatted(base);
    if (!serialized) { error = "No se pudo identificar el perfil base"; return false; }
    unsigned char hash[32];
    rhash_library_init();
    const int hashed = rhash_msg(RHASH_SHA256, serialized, std::strlen(serialized), hash);
    cJSON_free(serialized);
    if (hashed) { error = "No se pudo identificar el perfil base"; return false; }
    Context candidate;
    candidate.car = car; candidate.track = track; candidate.backend = "rustphysics";
    for (unsigned char byte : hash) { char hex[3]; std::snprintf(hex, sizeof(hex), "%02x", byte); candidate.baseHash += hex; }
    for (const auto& field : fields) {
        if (!field.editable) continue;
        const auto* original = at(base, field.path);
        const auto* resolved = at(current, field.path);
        if (!cJSON_IsNumber(original) || !cJSON_IsNumber(resolved)) {
            error = "Campo de reglaje sin valor base: " + field.path; return false;
        }
        candidate.rules[field.path] = {field.minimum, field.maximum, original->valuedouble,
                                      resolved->valuedouble, field.unit == "count"};
    }
    if (candidate.rules.empty()) { error = "Este coche no ofrece reglajes editables"; return false; }
    out = candidate;
    return true;
}

std::string filename(const std::string& name) {
    std::string safe;
    for (unsigned char c : name) {
        if (safe.size() >= 100) break;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_') safe += c;
        else if (c == ' ' && !safe.empty() && safe.back() != '_') safe += '_';
    }
    return safe.empty() ? "" : safe + ".json";
}

bool validate(const Context& ctx, const Setup& setup, std::string& error) {
    if (setup.name.empty() || setup.name.size()>100 || setup.name.find_first_not_of(" \t\r\n")==std::string::npos
        || std::any_of(setup.name.begin(),setup.name.end(),[](unsigned char c) { return c<32; })) {
        error = "El nombre debe tener entre 1 y 100 caracteres"; return false;
    }
    for (const auto& pair : setup.values) {
        auto found = ctx.rules.find(pair.first);
        if (found == ctx.rules.end()) { error = "Parametro fijo o desconocido: " + pair.first; return false; }
        const Rule& rule = found->second;
        if (!std::isfinite(pair.second) || pair.second < rule.minimum || pair.second > rule.maximum
            || (rule.integral && std::floor(pair.second) != pair.second)) {
            error = "Valor fuera de limites: " + pair.first; return false;
        }
    }
    if (setup.values.size() != ctx.rules.size()) { error = "Reglaje incompleto: faltan parametros editables"; return false; }
    return true;
}

std::vector<std::string> list(const std::string& directory) {
    std::vector<std::string> files;
#ifdef _WIN32
    WIN32_FIND_DATAA entry;
    HANDLE handle = FindFirstFileA((directory + "/*.json").c_str(), &entry);
    if (handle != INVALID_HANDLE_VALUE) {
        do { if (!(entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) files.push_back(entry.cFileName); }
        while (FindNextFileA(handle, &entry));
        FindClose(handle);
    }
#else
    if (DIR* dir = opendir(directory.c_str())) {
        while (dirent* entry = readdir(dir)) {
            std::string name(entry->d_name);
            if (name.size() > 5 && name.substr(name.size()-5) == ".json") files.push_back(name);
        }
        closedir(dir);
    }
#endif
    std::sort(files.begin(), files.end());
    return files;
}

bool save(const std::string& directory, const Context& ctx, const Setup& setup, bool overwrite, std::string& error) {
    if (!validate(ctx, setup, error)) return false;
#ifdef _WIN32
    _mkdir(directory.c_str());
#else
    mkdir(directory.c_str(), 0755);
#endif
    const std::string target = directory + '/' + filename(setup.name);
    if (!overwrite && exists(target)) { error = "El archivo ya existe; confirma sobrescribir"; return false; }
    Json root(cJSON_CreateObject(), cJSON_Delete);
    cJSON_AddNumberToObject(root.get(), "schema_version", 1);
    cJSON_AddStringToObject(root.get(), "name", setup.name.c_str());
    cJSON_AddStringToObject(root.get(), "track_id", ctx.track.c_str());
    cJSON_AddStringToObject(root.get(), "car_id", ctx.car.c_str());
    cJSON_AddStringToObject(root.get(), "physics_backend", ctx.backend.c_str());
    cJSON_AddStringToObject(root.get(), "base_profile_sha256", ctx.baseHash.c_str());
    cJSON* values = cJSON_AddObjectToObject(root.get(), "values");
    for (const auto& pair : setup.values) cJSON_AddNumberToObject(values, pair.first.c_str(), pair.second);
    char* text = cJSON_Print(root.get());
    if (!text) { error = "No se pudo serializar el reglaje"; return false; }
    const std::string temporary = target + ".tmp";
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    output << text << '\n'; cJSON_free(text); output.close();
    if (!output) { std::remove(temporary.c_str()); error = "No se pudo escribir el reglaje"; return false; }
#ifdef _WIN32
    const bool replaced = MoveFileExA(temporary.c_str(), target.c_str(), MOVEFILE_WRITE_THROUGH | (overwrite ? MOVEFILE_REPLACE_EXISTING : 0)) != 0;
#else
    const bool replaced = std::rename(temporary.c_str(), target.c_str()) == 0;
#endif
    if (!replaced) { std::remove(temporary.c_str()); error = "No se pudo guardar el archivo; el anterior se conserva"; return false; }
    return true;
}

bool load(const std::string& directory, const std::string& file, const Context& ctx, Setup& setup, std::string& error) {
    if (file.size() < 6 || filename(file.substr(0, file.size()-5)) != file) { error = "Nombre de archivo no permitido"; return false; }
    std::ifstream input(directory + '/' + file, std::ios::binary);
    if (!input) { error = "No se pudo abrir el reglaje"; return false; }
    input.seekg(0, std::ios::end);
    const auto size = input.tellg();
    if (size <= 0 || size > 65536) { error = "Archivo de reglaje vacio o demasiado grande"; return false; }
    input.seekg(0);
    std::string source(static_cast<size_t>(size), '\0'); input.read(&source[0], size);
    if (!input || source.find('\0') != std::string::npos) { error = "Archivo de reglaje invalido"; return false; }
    auto root = parse(source);
    if (!cJSON_IsObject(root.get()) || !uniqueKeys(root.get())) { error = "JSON invalido o claves duplicadas"; return false; }
    const std::set<std::string> allowed = {"schema_version","name","track_id","car_id","physics_backend","base_profile_sha256","values"};
    for (const cJSON* child = root->child; child; child = child->next)
        if (!allowed.count(child->string)) { error = "Contenido ajeno al reglaje: " + std::string(child->string); return false; }
    const auto* version = cJSON_GetObjectItemCaseSensitive(root.get(), "schema_version");
    if (!cJSON_IsNumber(version) || version->valuedouble != 1) { error = "Version de reglaje incompatible"; return false; }
    if (stringAt(root.get(), "car_id") != ctx.car || stringAt(root.get(), "track_id") != ctx.track
        || stringAt(root.get(), "physics_backend") != ctx.backend || stringAt(root.get(), "base_profile_sha256") != ctx.baseHash) {
        error = "Reglaje incompatible con el coche, circuito o perfil base"; return false;
    }
    Setup candidate; candidate.name = stringAt(root.get(), "name");
    const auto* values = cJSON_GetObjectItemCaseSensitive(root.get(), "values");
    if (!cJSON_IsObject(values) || !uniqueKeys(values)) { error = "Valores invalidos o repetidos"; return false; }
    for (const cJSON* child = values->child; child; child = child->next) {
        if (!cJSON_IsNumber(child)) { error = "Valor de reglaje no numerico"; return false; }
        candidate.values[child->string] = child->valuedouble;
    }
    if (!validate(ctx, candidate, error)) return false;
    setup = candidate;
    return true;
}
}
