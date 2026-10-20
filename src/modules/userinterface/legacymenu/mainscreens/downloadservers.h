#ifndef DOWNLOADSERVERS_H
#define DOWNLOADSERVERS_H

#include <string>
#include <vector>

int downloadservers_get(std::vector<std::string> &urls);
int downloadservers_set(const std::vector<std::string> &urls);

#endif
