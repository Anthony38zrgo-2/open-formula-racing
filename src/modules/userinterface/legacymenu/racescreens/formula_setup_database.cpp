#include "formula_setup_database.h"
#include <sqlite3.h>
#include <cjson/cJSON.h>
#include <memory>
#include <set>

namespace FormulaSetups {
namespace {
using Statement = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;
Statement prepare(sqlite3* db, const char* sql, std::string& error) {
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(db,sql,-1,&raw,nullptr)!=SQLITE_OK) error=sqlite3_errmsg(db);
    return Statement(raw,sqlite3_finalize);
}
void bind(sqlite3_stmt* s,int i,const std::string& text) { sqlite3_bind_text(s,i,text.c_str(),-1,SQLITE_TRANSIENT); }
std::string text(sqlite3_stmt* s,int i) {
    const auto* value=sqlite3_column_text(s,i);
    return value?reinterpret_cast<const char*>(value):"";
}
bool execute(sqlite3* db,const char* sql,std::string& error) {
    if (sqlite3_exec(db,sql,nullptr,nullptr,nullptr)==SQLITE_OK) return true;
    error=sqlite3_errmsg(db); return false;
}
bool encode(const Context& ctx,const Setup& setup,std::string& values,std::string& error) {
    if (!validate(ctx,setup,error)) return false;
    cJSON* object=cJSON_CreateObject();
    for (const auto& pair:setup.values) cJSON_AddNumberToObject(object,pair.first.c_str(),pair.second);
    char* raw=cJSON_PrintUnformatted(object); cJSON_Delete(object);
    if (!raw) { error="No se pudo preparar el reglaje"; return false; }
    values=raw; cJSON_free(raw); return true;
}
bool written(sqlite3* db,sqlite3_stmt* s,std::string& error) {
    if (sqlite3_step(s)!=SQLITE_DONE) {
        error=sqlite3_errcode(db)==SQLITE_CONSTRAINT ? "El nombre ya existe o el circuito/equipo no esta registrado" : sqlite3_errmsg(db);
        return false;
    }
    if (sqlite3_changes(db)!=1) { error="El reglaje no existe o no es compatible con este coche"; return false; }
    return true;
}
}
Database::~Database() { if (db) sqlite3_close(db); }
bool Database::open(const std::string& path,std::string& error) {
    if (db) { error="La base de reglajes ya esta abierta"; return false; }
    if (sqlite3_open_v2(path.c_str(),&db,SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE,nullptr)!=SQLITE_OK) {
        error=db?sqlite3_errmsg(db):"No se pudo abrir la base de reglajes";
        if (db) sqlite3_close(db);
        db=nullptr;
        return false;
    }
    sqlite3_busy_timeout(db,3000);
    if (!execute(db,"PRAGMA foreign_keys=ON; PRAGMA synchronous=FULL;",error)) return false;
    auto version=prepare(db,"PRAGMA user_version",error);
    if (!version || sqlite3_step(version.get())!=SQLITE_ROW) return false;
    const int current=sqlite3_column_int(version.get(),0); version.reset();
    if (current>1) { error="Version de base de datos mas reciente que este juego"; return false; }
    const char* schema =
        "BEGIN IMMEDIATE;"
        "CREATE TABLE IF NOT EXISTS circuit(id TEXT PRIMARY KEY,name TEXT NOT NULL,country TEXT NOT NULL,length_m REAL NOT NULL CHECK(length_m>0));"
        "CREATE TABLE IF NOT EXISTS team(id TEXT PRIMARY KEY,name TEXT NOT NULL);"
        "CREATE TABLE IF NOT EXISTS car_setup("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,name TEXT NOT NULL CHECK(length(trim(name)) BETWEEN 1 AND 100),"
        "circuit_id TEXT NOT NULL REFERENCES circuit(id),team_id TEXT NOT NULL REFERENCES team(id),"
        "car_id TEXT NOT NULL,physics_backend TEXT NOT NULL,base_profile_sha256 TEXT NOT NULL,"
        "schema_version INTEGER NOT NULL DEFAULT 1 CHECK(schema_version=1),"
        "values_json TEXT NOT NULL CHECK(json_valid(values_json) AND json_type(values_json)='object' AND length(values_json)<=65536),"
        "created_at TEXT NOT NULL DEFAULT(strftime('%Y-%m-%dT%H:%M:%fZ','now')),"
        "modified_at TEXT NOT NULL DEFAULT(strftime('%Y-%m-%dT%H:%M:%fZ','now')),"
        "UNIQUE(circuit_id,car_id,name COLLATE NOCASE));"
        "CREATE INDEX IF NOT EXISTS car_setup_circuit_date ON car_setup(circuit_id,modified_at DESC,id DESC);"
        "INSERT OR IGNORE INTO circuit VALUES('melbourne-nineteen-ninety-seven','Melbourne 1997','Australia',5299.673);"
        "INSERT OR IGNORE INTO team VALUES('williams','Williams');"
        "PRAGMA user_version=1; COMMIT;";
    if (!execute(db,schema,error)) { sqlite3_exec(db,"ROLLBACK",nullptr,nullptr,nullptr); return false; }
    return true;
}
bool Database::list(const std::string& circuit,std::vector<Record>& rows,std::string& error) {
    auto statement=prepare(db,"SELECT s.id,s.name,t.name,s.created_at,s.modified_at FROM car_setup s JOIN team t ON t.id=s.team_id WHERE circuit_id=? ORDER BY modified_at DESC,s.id DESC",error);
    if (!statement) return false;
    bind(statement.get(),1,circuit); std::vector<Record> candidate; int result;
    while ((result=sqlite3_step(statement.get()))==SQLITE_ROW) {
        Record row; row.id=sqlite3_column_int64(statement.get(),0); row.name=text(statement.get(),1);
        row.team=text(statement.get(),2); row.created=text(statement.get(),3); row.modified=text(statement.get(),4);
        candidate.push_back(row);
    }
    if (result!=SQLITE_DONE) { error=sqlite3_errmsg(db); return false; }
    rows=std::move(candidate); return true;
}
bool Database::create(const Context& ctx,const Setup& setup,const std::string& team,std::int64_t& id,std::string& error) {
    std::string values; if (!encode(ctx,setup,values,error)) return false;
    auto s=prepare(db,"INSERT INTO car_setup(name,circuit_id,team_id,car_id,physics_backend,base_profile_sha256,values_json) VALUES(?,?,?,?,?,?,?)",error);
    if (!s) return false;
    bind(s.get(),1,setup.name); bind(s.get(),2,ctx.track); bind(s.get(),3,team); bind(s.get(),4,ctx.car);
    bind(s.get(),5,ctx.backend); bind(s.get(),6,ctx.baseHash); bind(s.get(),7,values);
    if (!written(db,s.get(),error)) return false;
    id=sqlite3_last_insert_rowid(db); return true;
}
bool Database::update(std::int64_t id,const Context& ctx,const Setup& setup,std::string& error) {
    std::string values; if (!encode(ctx,setup,values,error)) return false;
    auto s=prepare(db,"UPDATE car_setup SET name=?,values_json=?,modified_at=strftime('%Y-%m-%dT%H:%M:%fZ','now') WHERE id=? AND circuit_id=? AND car_id=? AND physics_backend=? AND base_profile_sha256=? AND schema_version=1",error);
    if (!s) return false;
    bind(s.get(),1,setup.name); bind(s.get(),2,values); sqlite3_bind_int64(s.get(),3,id);
    bind(s.get(),4,ctx.track); bind(s.get(),5,ctx.car); bind(s.get(),6,ctx.backend); bind(s.get(),7,ctx.baseHash);
    return written(db,s.get(),error);
}
bool Database::load(std::int64_t id,const Context& ctx,Setup& setup,std::string& error) {
    auto s=prepare(db,"SELECT name,car_id,physics_backend,base_profile_sha256,schema_version,values_json FROM car_setup WHERE id=? AND circuit_id=?",error);
    if (!s) return false;
    sqlite3_bind_int64(s.get(),1,id); bind(s.get(),2,ctx.track);
    if (sqlite3_step(s.get())!=SQLITE_ROW) { error="El reglaje no pertenece al circuito actual o ya no existe"; return false; }
    if (text(s.get(),1)!=ctx.car || text(s.get(),2)!=ctx.backend || text(s.get(),3)!=ctx.baseHash || sqlite3_column_int(s.get(),4)!=1) {
        error="Reglaje incompatible con el coche o perfil actual"; return false;
    }
    const std::string raw=text(s.get(),5);
    if (raw.empty() || raw.size()>65536 || raw.find('\0')!=std::string::npos) { error="Reglaje almacenado invalido"; return false; }
    std::unique_ptr<cJSON,decltype(&cJSON_Delete)> object(cJSON_ParseWithOpts(raw.c_str(),nullptr,1),cJSON_Delete);
    if (!cJSON_IsObject(object.get())) { error="Reglaje almacenado invalido"; return false; }
    Setup candidate; candidate.name=text(s.get(),0); std::set<std::string> seen;
    for (const auto* field=object->child; field; field=field->next) {
        if (!field->string || !seen.insert(field->string).second || !cJSON_IsNumber(field)) { error="Reglaje con valores invalidos o duplicados"; return false; }
        candidate.values[field->string]=field->valuedouble;
    }
    if (!validate(ctx,candidate,error)) return false;
    setup=std::move(candidate); return true;
}
bool Database::remove(std::int64_t id,const std::string& circuit,std::string& error) {
    auto s=prepare(db,"DELETE FROM car_setup WHERE id=? AND circuit_id=?",error);
    if (!s) return false;
    sqlite3_bind_int64(s.get(),1,id); bind(s.get(),2,circuit);
    return written(db,s.get(),error);
}
}
