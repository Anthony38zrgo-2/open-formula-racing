#include "formula_setup_database.h"
#include <sqlite3.h>
#include <fstream>
#include <iostream>
#include <sstream>

int main(int argc,char** argv) {
    std::string error;
    if (argc==3 && std::string(argv[1])=="--initialize") {
        FormulaSetups::Database database;
        if (!database.open(argv[2],error)) { std::cerr<<error; return 1; }
        std::cout<<"DATABASE_INITIALIZED\n"; return 0;
    }
    if (argc!=4) return 2;
    int checks=0,failures=0;
    auto check=[&](bool value,const char* name) { ++checks; if (!value) { ++failures; std::cerr<<"FAIL "<<name<<" "<<error<<'\n'; } };
    std::ifstream doc(argv[1]); const std::string document(std::istreambuf_iterator<char>(doc),{});
    std::ifstream input(argv[2]); std::string line; std::vector<PhysicsSetupField> fields;
    while (std::getline(input,line)) {
        std::istringstream row(line); PhysicsSetupField f; int editable;
        std::getline(row,f.path,'\t'); std::getline(row,f.label,'\t'); std::getline(row,f.unit,'\t');
        row>>f.value>>f.minimum>>f.maximum>>f.step>>editable; f.editable=editable!=0; fields.push_back(f);
    }
    FormulaSetups::Context ctx;
    check(FormulaSetups::context(document,fields,"williams-fw19","melbourne-nineteen-ninety-seven",ctx,error),"actual editable catalog");
    FormulaSetups::Setup original; original.name="Carrera - equilibrado";
    for (const auto& rule:ctx.rules) original.values[rule.first]=rule.second.current;
    const std::string path=argv[3]; std::int64_t id=0, second=0;
    {
        FormulaSetups::Database database;
        check(database.open(path,error),"initialize SQLite schema and seed");
        std::vector<FormulaSetups::Record> rows;
        check(database.list(ctx.track,rows,error) && rows.empty(),"new circuit list empty");
        check(database.create(ctx,original,"williams",id,error) && id>0,"create setup with all editable values");
        check(database.list(ctx.track,rows,error) && rows.size()==1 && rows[0].team=="Williams","list names and joined team");
        const auto created=rows[0].created;
        check(!created.empty() && rows[0].modified==created,"creation and modification timestamps initialized");
        FormulaSetups::Setup loaded;
        check(database.load(id,ctx,loaded,error) && loaded.values==original.values,"exact database roundtrip");
        check(!database.create(ctx,original,"williams",second,error),"duplicate name refused without replacing record");
        original.name="Clasificacion"; original.values["brakes.front_brake_bias"]=.615;
        check(database.update(id,ctx,original,error),"update existing selected setup");
        check(database.list(ctx.track,rows,error) && rows[0].created==created && rows[0].modified>=created,"update preserves creation date");
        check(database.load(id,ctx,loaded,error) && loaded.name==original.name && loaded.values==original.values,"load updated values");
        const auto safe=loaded.values;
        auto invalid=original; invalid.values["tires.front.radius"]=3;
        check(!database.update(id,ctx,invalid,error),"fixed field rejected on update");
        invalid=original; invalid.values.erase(invalid.values.begin());
        check(!database.create(ctx,invalid,"williams",second,error),"incomplete setup rejected on create");
        invalid=original; invalid.values["brakes.front_brake_bias"]=100;
        check(!database.update(id,ctx,invalid,error),"out of range rejected");
        auto other=ctx; other.car="another-car";
        check(!database.load(id,other,loaded,error) && loaded.values==safe,"wrong car cannot load or mutate editor");
        check(!database.update(id,other,original,error),"wrong car cannot overwrite");
        other=ctx; other.track="fuji";
        check(database.list(other.track,rows,error) && rows.empty(),"list limited to circuit");
        check(!database.load(id,other,loaded,error),"other circuit cannot load");
        check(!database.remove(id,other.track,error),"other circuit cannot delete");
        other=ctx; other.baseHash="changed";
        check(!database.load(id,other,loaded,error),"changed base profile rejected");
        other=ctx; other.backend="simuv5";
        check(!database.load(id,other,loaded,error),"different backend rejected");
        auto third=original; third.name="Otro equipo";
        check(!database.create(ctx,third,"not-indexed",second,error),"team foreign key enforced");
        other=ctx; other.track="not-indexed";
        check(!database.create(other,third,"williams",second,error),"circuit foreign key enforced");
        third.name="Robert'); DROP TABLE car_setup;--";
        check(database.create(ctx,third,"williams",second,error),"name bound safely as SQL parameter");
        check(database.load(second,ctx,loaded,error) && loaded.name==third.name,"punctuation in names preserved");
        check(database.remove(second,ctx.track,error),"delete selected setup");
        check(!database.load(second,ctx,loaded,error),"deleted setup no longer loads");
    }
    {
        FormulaSetups::Database reopened;
        check(reopened.open(path,error),"reopen database after shutdown");
        FormulaSetups::Setup loaded;
        check(reopened.load(id,ctx,loaded,error) && loaded.values==original.values,"saved setup survives restart");
    }
    sqlite3* raw=nullptr; sqlite3_open(path.c_str(),&raw);
    auto scalar=[&](const char* sql) {
        sqlite3_stmt* s=nullptr; sqlite3_prepare_v2(raw,sql,-1,&s,nullptr); int count=-1;
        if (sqlite3_step(s)==SQLITE_ROW) count=sqlite3_column_int(s,0);
        sqlite3_finalize(s); return count;
    };
    check(scalar("SELECT count(*) FROM circuit")==1,"only Melbourne indexed");
    check(scalar("SELECT count(*) FROM team")==1,"only Williams indexed");
    check(scalar("SELECT count(*) FROM json_each((SELECT values_json FROM car_setup LIMIT 1))")==33,"complete whitelist stored, no vehicle document");
    sqlite3_exec(raw,"UPDATE car_setup SET values_json=json_set(values_json,'$.tires.front.radius',5)",nullptr,nullptr,nullptr);
    {
        FormulaSetups::Database database; database.open(path,error); FormulaSetups::Setup loaded;
        check(!database.load(id,ctx,loaded,error),"database tampering cannot inject fixed parameters");
    }
    sqlite3_exec(raw,"PRAGMA user_version=99",nullptr,nullptr,nullptr);
    {
        FormulaSetups::Database future;
        check(!future.open(path,error),"newer database schema refused without migration");
    }
    sqlite3_exec(raw,"PRAGMA user_version=1",nullptr,nullptr,nullptr); sqlite3_close(raw);
    {
        FormulaSetups::Database database; database.open(path,error);
        check(database.update(id,ctx,original,error),"restore test record after tampering checks");
    }
    std::cout<<"SQLITE_CHECKS="<<checks<<" FAILURES="<<failures<<" EDITABLE_FIELDS="<<ctx.rules.size()<<'\n';
    return failures?1:0;
}
