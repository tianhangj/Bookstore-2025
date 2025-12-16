#pragma once

#include <cstring>
#include <ostream>
#include <string>

struct String {
    char s[64];
    String() { memset(s, 0, sizeof(s)); }
    String(const char* _s) { strcpy(s, _s); }
    String(const std::string _s) { strcpy(s, _s.c_str()); }
    const bool operator==(const String rhs) const { return strcmp(this->s, rhs.s) == 0; }
    const bool operator<(const String rhs) const { return strcmp(this->s, rhs.s) < 0; }
    const bool operator>=(const String rhs) const { return strcmp(this->s, rhs.s) >= 0; }
};
std::ostream& operator<<(std::ostream& output, const String& rhs) {
    output << rhs.s;
    return output;
}