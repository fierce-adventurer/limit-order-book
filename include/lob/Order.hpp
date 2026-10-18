#pragma once

#include "Types.hpp"
#include <cassert>

namespace lob {

struct Order {
    OrderId id{0};
    Price price{0};
    Timestamp timestamp{0};
    Quantity initial_quantity{0};
    Quantity open_quantity{0};
    Side side{Side::Buy};
    OrderType type{OrderType::Limit};

    constexpr Order() noexcept = default;

    constexpr Order(OrderId id, Price price, Quantity qty, Side side,
                    OrderType type = OrderType::Limit, Timestamp ts = 0) noexcept
    : id(id),
      price(price),
      timestamp(ts),
      initial_quantity(qty),
      open_quantity(qty),
      side(side),
      type(type){}

    [[nodiscard]] constexpr bool is_filled() const noexcept {
        return open_quantity == 0;
    }

    constexpr void fill(Quantity qty) noexcept {
        assert(qty <= open_quantity && "Fill quantity exceeds open quantity!");
        open_quantity -= qty;
    }
};

static_assert(sizeof(Order) == 40, "Unexpected padding in Order struct!");
static_assert(alignof(Order) == 8, "Order struct alignment should be 9 bytes!");
}