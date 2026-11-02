#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace shelf {

constexpr std::size_t TITLE_LEN = 64;
constexpr std::size_t AUTHOR_LEN = 48;

struct Book {
    std::int32_t id = 0;        // primary key
    std::string title;
    std::string author;         // secondary index
    std::int32_t year = 0;
    double price = 0.0;
    bool available = true;
};

}
