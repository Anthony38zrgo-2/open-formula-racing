#include "formula_setup_store.h"
#include <cjson/cJSON.h>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>

static std::string read(const std::string& path) {
    std::ifstream input(path,std::ios::binary); return std::string(std::istreambuf_iterator<char>(input),{});
}
static void write(const std::string& path, const std::string& data) { std::ofstream(path,std::ios::binary)<<data; }
int main(int argc,char** argv) {
    if (argc!=4) return 2;
    int checks=0,failures=0;
    auto check=[&](bool okay,const char* name) { ++checks; if (!okay) { ++failures; std::cerr<<"FAIL "<<name<<'\n'; } };
    std::vector<PhysicsSetupField> fields;
    std::ifstream descriptors(argv[2]); std::string line;
    while (std::getline(descriptors,line)) {
        std::istringstream row(line); PhysicsSetupField f; int editable;
        std::getline(row,f.path,'\t'); std::getline(row,f.label,'\t'); std::getline(row,f.unit,'\t');
        if (!(row>>f.value>>f.minimum>>f.maximum>>f.step>>editable)) return 3;
        f.editable=editable!=0; fields.push_back(f);
    }
    FormulaSetups::Context context; std::string error;
    check(FormulaSetups::context(read(argv[1]),fields,"williams-fw19","melbourne-1997",context,error),"actual Rust profile context");
    if (context.rules.empty()) return 4;
    FormulaSetups::Setup original; original.name="Melbourne_1997_Williams_FW19";
    for (auto& pair:context.rules) original.values[pair.first]=pair.second.current;
    const std::string directory=argv[3], file=FormulaSetups::filename(original.name), path=directory+'/'+file;
    check(FormulaSetups::save(directory,context,original,true,error),"save actual editable snapshot");
    const std::string serialized=read(path);
    FormulaSetups::Setup restored;
    check(FormulaSetups::load(directory,file,context,restored,error),"load actual editable snapshot");
    check(restored.values==original.values,"exact numeric roundtrip including angular fields");
    check(restored.values.size()==context.rules.size(),"complete editable whitelist only");
    check(serialized.find("\"base\"")==std::string::npos && serialized.find("\"document\"")==std::string::npos,"full vehicle documents excluded");
    check(!FormulaSetups::save(directory,context,original,false,error),"overwrite requires explicit confirmation");
    check(read(path)==serialized,"overwrite refusal preserves file");
    auto expectRejected=[&](const std::string& text,const char* reason) {
        write(path,text); auto before=restored.values;
        check(!FormulaSetups::load(directory,file,context,restored,error),reason);
        check(restored.values==before,"rejection leaves current setup intact");
    };
    auto mutate=[&](const std::function<void(cJSON*)>& change) {
        cJSON* root=cJSON_Parse(serialized.c_str()); change(root); char* text=cJSON_Print(root);
        std::string result(text); cJSON_free(text); cJSON_Delete(root); return result;
    };
    expectRejected(mutate([](cJSON* r){ cJSON_AddNumberToObject(cJSON_GetObjectItem(r,"values"),"chassis.mass",9999); }),"fixed field injection rejected");
    expectRejected(mutate([](cJSON* r){ cJSON_AddNumberToObject(cJSON_GetObjectItem(r,"values"),"tires.front.radius",5); }),"known read-only field rejected");
    expectRejected(mutate([](cJSON* r){ cJSON_AddStringToObject(r,"document","vehicle"); }),"full document injection rejected");
    expectRejected(mutate([](cJSON* r){ cJSON_ReplaceItemInObject(r,"car_id",cJSON_CreateString("other-car")); }),"wrong car rejected");
    expectRejected(mutate([](cJSON* r){ cJSON_ReplaceItemInObject(r,"track_id",cJSON_CreateString("fuji")); }),"wrong circuit rejected");
    expectRejected(mutate([](cJSON* r){ cJSON_ReplaceItemInObject(r,"physics_backend",cJSON_CreateString("simuv5")); }),"wrong backend rejected");
    expectRejected(mutate([](cJSON* r){ cJSON_ReplaceItemInObject(r,"base_profile_sha256",cJSON_CreateString("old-profile")); }),"changed base profile rejected");
    expectRejected(mutate([](cJSON* r){ cJSON_ReplaceItemInObject(r,"schema_version",cJSON_CreateNumber(2)); }),"unknown format rejected");
    expectRejected(mutate([&](cJSON* r){ cJSON_DeleteItemFromObject(cJSON_GetObjectItem(r,"values"),context.rules.begin()->first.c_str()); }),"incomplete snapshot rejected");
    expectRejected(mutate([&](cJSON* r){ cJSON_ReplaceItemInObject(cJSON_GetObjectItem(r,"values"),context.rules.begin()->first.c_str(),cJSON_CreateNumber(context.rules.begin()->second.maximum+1)); }),"out of range rejected");
    expectRejected(mutate([](cJSON* r){ cJSON_ReplaceItemInObject(cJSON_GetObjectItem(r,"values"),"powertrain.differential.clutches",cJSON_CreateNumber(2.5)); }),"fractional clutch count rejected");
    expectRejected(mutate([&](cJSON* r){ cJSON_AddNumberToObject(cJSON_GetObjectItem(r,"values"),context.rules.begin()->first.c_str(),context.rules.begin()->second.current); }),"duplicate values rejected");
    expectRejected(mutate([](cJSON* r){ cJSON_AddNumberToObject(r,"schema_version",1); }),"duplicate metadata rejected");
    expectRejected(serialized+" garbage","trailing data rejected");
    expectRejected("{bad json}","malformed JSON rejected");
    write(path,serialized);
    check(!FormulaSetups::load(directory,"../escape.json",context,restored,error),"load path traversal rejected");
    check(!FormulaSetups::save(directory+"/missing/child",context,original,true,error),"write failure is reported");
    check(read(path)==serialized,"unrelated write failure preserves valid file");
    check(FormulaSetups::list(directory).size()>=1,"saved files discoverable");
    check(FormulaSetups::filename("Melbourne 1997_Williams FW19")==file,"default circuit plus car filename");
    std::cout<<"SETUP_CHECKS="<<checks<<" FAILURES="<<failures<<" EDITABLE_FIELDS="<<context.rules.size()<<'\n';
    return failures?1:0;
}
