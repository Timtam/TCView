#pragma once

#include <filesystem>
#include <string>

std::experimental::filesystem::v1::path GetModuleDirectory();
std::vector<std::string> string_split(const char *s, char delim);
