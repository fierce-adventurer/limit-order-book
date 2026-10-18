#pragma once

#include "Types.hpp"
#include "Order.hpp"
#include <list>
#include <cassert>

namespace lob{

class PriceLevel {
public:
    explicit PriceLevel(Price price) noexcept
        : price_(price), total_volume_(0) {}

    [[nodiscard]] Price price() const noexcept {return price_; }
    [[nodiscard]] Quantity total_volume() const noexcept {return total_volume_; }
    [[nodiscard]] std::size_t order_count() const noexcept { return orders_.size(); }
    [[nodiscard]] bool empty() const noexcept { return orders_.empty(); } 

    void add_order(const Order& order){
        assert(order.price == price_ && "order price must match price level!");
        orders_.push_back(order);
        total_volume_ += order.open_quantity;
    }

    [[nodiscard]] Order& front() noexcept {
        assert(!orders_.empty() && "Cannot access front of an empty price level!");
        return orders_.front();
    }

    [[nodiscard]] const Order& front() const noexcept {
        assert(!orders_.empty() && "Cannot access front of an empty price level!");
        return orders_.front();
    }

    void pop_front() noexcept {
        assert(!orders_.empty() && "Cannot pop from an empty price level!");
        total_volume_ -= orders_.front().open_quantity;
        orders_.pop_front();
    }

    void reduce_volume(Quantity qty) noexcept {
        assert(qty <= total_volume_ && "Cannot reduce volume below zero!");
        total_volume_ -= qty;
    }

    [[nodiscard]] std::list<Order>& orders() noexcept { return orders_;}
    [[nodiscard]] const std::list<Order>& orders() const noexcept { return orders_; }

private:
    Price price_{0};
    Quantity total_volume_{0};
    std::list<Order> orders_{};
};
}