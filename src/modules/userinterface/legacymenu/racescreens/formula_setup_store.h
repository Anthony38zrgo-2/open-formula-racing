#ifndef FORMULA_SETUP_STORE_H
#define FORMULA_SETUP_STORE_H

#include <iphysicsengine.h>
#include <map>
#include <string>
#include <vector>

namespace FormulaSetups {
struct Rule {
    double minimum, maximum, base, current;
    bool integral;
};
struct Context {
    std::string car, track, backend, baseHash;
    std::map<std::string, Rule> rules;
};
struct Setup {
    std::string name;
    std::map<std::string, double> values;
};
bool context(const std::string& document, const std::vector<PhysicsSetupField>& fields,
             const std::string& car, const std::string& track, Context& out, std::string& error);
bool validate(const Context& context, const Setup& setup, std::string& error);
std::string filename(const std::string& name);
std::vector<std::string> list(const std::string& directory);
bool save(const std::string& directory, const Context& context, const Setup& setup,
          bool overwrite, std::string& error);
bool load(const std::string& directory, const std::string& file, const Context& context,
          Setup& setup, std::string& error);
}
#endif
