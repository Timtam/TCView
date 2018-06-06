#pragma once

#include <string>

std::wstring GetModuleDirectory();
std::vector<std::string> string_split(const char *s, char delim);
std::wstring mbs_to_wcs(std::string mbs);
