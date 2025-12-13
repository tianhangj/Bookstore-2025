#pragma once

#include <compare>
#include <cstring>

struct String {
    char s[64];
    String() { memset(s, 0, sizeof(s)); }
    String(const char* _s) { strcpy(s, _s); }
    const bool operator==(const String rhs) const { return strcmp(this->s, rhs.s) == 0; }
    const std::partial_ordering operator<=>(const String rhs) const {
        int _cmp = strcmp(this->s, rhs.s);
        if (_cmp > 0) {
            return std::partial_ordering::greater;
        } else if (_cmp < 0) {
            return std::partial_ordering::less;
        } else {
            return std::partial_ordering::equivalent;
        }
    }
};