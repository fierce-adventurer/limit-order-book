#pragma once

#include "Types.hpp"

namespace lob {

    struct Trade {
        OrderId maker_order_id{0};

        OrderId taker_order_id{0};

        Price price{0};
        
        Quantity quantity{0};

        Timestamp timestamp{0};

        constexpr Trade() noexcept = default;

        constexpr Trade(OrderId maker, OrderId taker, Price p , Quantity q, Timestamp ts) noexcept
            : maker_order_id(maker),
              taker_order_id(taker),
              price(p),
              quantity(q),
              timestamp(ts){}
    };
}