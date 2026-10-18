#pragma once

#include <cstdint>
#include <string_view>

namespace lob{

using OrderId = uint64_t;
using Price = uint64_t;
using Quantity = uint32_t;
using Timestamp = uint64_t;

enum class Side : uint8_t {
    Buy = 0,
    Sell = 1
};

enum class OrderType : uint8_t{
    Limit = 0,
    Market = 1,
    Cancel = 2
};
constexpr std::string_view side_to_string(Side side)
noexcept{
    switch (side)
    {
        case Side::Buy: return "BUY";
        case Side::Sell: return "SELL";
    }
    return "UNKNOWN";
}

constexpr std::string_view order_type_to_string(OrderType type)
noexcept {
    switch(type){
        case OrderType::Limit: return "LIMIT";
        case OrderType::Market: return "MARKET";
        case OrderType::Cancel: return "CANCEL";
    }
    return "UNKNOWN";
}

}

