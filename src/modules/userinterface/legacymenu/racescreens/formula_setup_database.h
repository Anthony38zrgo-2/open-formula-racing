#ifndef FORMULA_SETUP_DATABASE_H
#define FORMULA_SETUP_DATABASE_H
#include "formula_setup_store.h"
#include <cstdint>
struct sqlite3;
namespace FormulaSetups {
struct Record {
    std::int64_t id = 0;
    std::string name, team, created, modified;
};
class Database {
    sqlite3* db = nullptr;
public:
    Database() = default;
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    bool open(const std::string& path, std::string& error);
    bool list(const std::string& circuit, std::vector<Record>& rows, std::string& error);
    bool create(const Context& context, const Setup& setup, const std::string& team,
                std::int64_t& id, std::string& error);
    bool update(std::int64_t id, const Context& context, const Setup& setup, std::string& error);
    bool load(std::int64_t id, const Context& context, Setup& setup, std::string& error);
    bool remove(std::int64_t id, const std::string& circuit, std::string& error);
};
}
#endif
