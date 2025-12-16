#pragma once

#include <compare>

#include "string.hpp"

struct User {
    String userid;
    String username;
    String passwd;
    int privilege;
    User() = default;
    User(const User& user) = default;
    User(String userid_, String username_, String passwd_, int privilege_) {
        userid = userid_;
        username = username_;
        passwd = passwd_;
        privilege = privilege_;
    }
    static User default_user() {
        static User _user("defaultuser0", "defaultuser0", "", 0);
        return _user;
    }
    const bool operator==(const User rhs) const {
        return this->userid == rhs.userid;
    }
    const bool operator<(const User rhs) const {
        return this->userid < rhs.userid;
    }
    const bool operator>=(const User rhs) const {
        return this->userid >= rhs.userid;
    }
};