#pragma once
#include "Types.hpp"
#include "Order.hpp"
#include "PriceLevel.hpp"
#include <map>
#include <unordered_map>
#include <optional>
#include <functional>

namespace lob {

struct LevelInfo{
    Price price{0};
    Quantity volume{0};
    std::size_t order_count{0};
};

class OrderBook {
public:
    struct OrderLocation {
        Side side;
        Price price;
        std::list<Order>::iterator it;
    };

    OrderBook() = default;

    bool add_order(const Order& order){
        if(orders_map_.find(order.id) != orders_map_.end()){
            return false;
        }

        if(order.side == Side::Buy){
            auto [level_it, _] = bids_.try_emplace(order.price, order.price);
            level_it->second.add_order(order);

            auto order_it = std::prev(level_it->second.orders().end());
            orders_map_[order.id] = OrderLocation{Side::Buy, order.price, order_it};

        }
        else {
            auto [level_it, _] = asks_.try_emplace(order.price, order.price);
            level_it->second.add_order(order);

            auto order_it = std::prev(level_it->second.orders().end());
            orders_map_[order.id] = OrderLocation{Side::Sell, order.price, order_it};
        }

        return true;
    }

    bool cancel_order(OrderId id){
        auto it = orders_map_.find(id);
        if(it == orders_map_.end()){
            return false;
        }

        const OrderLocation& loc = it->second;

        if(loc.side == Side::Buy){
            auto level_it = bids_.find(loc.price);
            if(level_it != bids_.end()){
                level_it -> second.reduce_volume(loc.it->open_quantity);
                level_it->second.orders().erase(loc.it);

                if(level_it ->second.empty()){
                    bids_.erase(level_it);
                }
            }
        }
        else{
            auto level_it = asks_.find(loc.price);
            if(level_it != asks_.end()){
                level_it->second.reduce_volume(loc.it->open_quantity);
                level_it->second.orders().erase(loc.it);

                if(level_it->second.empty()){
                    asks_.erase(level_it);
                }
            }
        }

        orders_map_.erase(it);
        return true;
    }

    [[nodiscard]] std::optional<LevelInfo> best_bid() const noexcept {
        if(bids_.empty()) return std::nullopt;
        const auto& [price, level] = *bids_.begin();
        return LevelInfo{price, level.total_volume(), level.order_count()};
    }

    [[nodiscard]] std:: optional<LevelInfo> best_ask() const noexcept {
        if(asks_.empty()) return std::nullopt;
        const auto& [price, level] = *asks_.begin();
        return LevelInfo{price, level.total_volume(), level.order_count()};
    }

    [[nodiscard]] std::optional<Price> spread() const noexcept {
        auto bb = best_bid();
        auto ba = best_ask();
        if(bb && ba) {
            return ba->price - bb->price;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::size_t total_orders() const noexcept { return orders_map_.size(); }
    [[nodiscard]] std::size_t bid_level_count() const noexcept { return bids_.size(); }
    [[nodiscard]] std::size_t ask_levels_count() const noexcept { return asks_.size(); }
    [[nodiscard]] bool empty() const noexcept { return orders_map_.empty(); }

    [[nodiscard]] auto& bids() noexcept { return bids_;}
    [[nodiscard]] const auto& bids() const noexcept { return bids_; }

    [[nodiscard]] auto& asks() noexcept { return asks_;}
    [[nodiscard]] const auto& asks() const noexcept { return asks_; }

    [[nodiscard]] auto& orders_map() noexcept { return orders_map_; }
    [[nodiscard]] const auto& orders_map() const noexcept { return orders_map_; }

private:
    std::map<Price, PriceLevel, std::greater<Price>> bids_;
    std::map<Price, PriceLevel, std::less<Price>> asks_;
    std::unordered_map<OrderId, OrderLocation> orders_map_;

};
}

