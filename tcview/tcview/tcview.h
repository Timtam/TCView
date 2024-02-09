#pragma once

#include <filesystem>
#include <string>

std::filesystem::path GetModuleDirectory();
std::vector<std::string> string_split(const char *s, char delim);
