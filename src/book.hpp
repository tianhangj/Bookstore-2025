#pragma once

#include "string.hpp"

struct Book {
    String ISBN;
    String name;
    String author;
    String keyword;
    double price;
    int quantity;
    Book() {
        ISBN = name = author = keyword = "uninitialized";
        price = quantity = 0;
    }
    const bool operator==(const Book rhs) const {
        return this->ISBN == rhs.ISBN;
    }
    const bool operator>=(const Book rhs) const {
        return this->ISBN >= rhs.ISBN;
    }
    const bool operator<(const Book rhs) const {
        return this->ISBN < rhs.ISBN;
    }
};