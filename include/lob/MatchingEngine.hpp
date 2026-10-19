#pragma once
#include "Types.hpp"
#include "Order.hpp"
#include "Trade.hpp"
#include "OrderBook.hpp"
#include <vector>
#include <functional>
#include <algorithm>

namespace lob {
    
class MatchingEngine {
public:
    using TradeListener = std::function<void(const Trade&)>;

    MatchingEngine() = default;

    void set_trade_listener(TradeListener listener) noexcept {
        trade_listener_ = std::move(listener);
    }

    void submit_order(Order order) {
        if(order.type == OrderType::Limit){
            match_limit_order(order);
        }
        else if(order.type == OrderType::Market){
            match_market_order(order);
        }
        else if(order.type == OrderType::Cancel){
            cancel_order(order.id);
        }
    }
    

    bool cancel_order(OrderId id){
        return book_.cancel_order(id);
    }

    [[nodiscard]] const OrderBook& book() const noexcept { return book_; }
    [[nodiscard]] OrderBook& book() noexcept { return book_; }
    [[nodiscard]] const std::vector<Trade>& trades() const noexcept { return trades_; }

private:
    void emit_trade(OrderId maker, OrderId taker, Price price, Quantity qty, Timestamp ts){
        Trade trade{maker, taker, price, qty, ts};
        trades_.push_back(trade);
        if(trade_listener_){
            trade_listener_(trade);
        }
    }

    void match_limit_order(Order& taker) {
        if (taker.side == Side::Buy) {
            match_buy_against_asks(taker, false);
            if (taker.open_quantity > 0) {
                book_.add_order(taker);
            }
        } else {
            match_sell_against_bids(taker, false);
            if (taker.open_quantity > 0) {
                book_.add_order(taker);
            }
        }
    }

    void match_market_order(Order& taker) {
        if (taker.side == Side::Buy) {
            match_buy_against_asks(taker, true);
        } else {
            match_sell_against_bids(taker, true);
        }
    }
    

    void match_buy_against_asks(Order& taker, bool is_market){
        auto& asks = book_.asks();

        while(taker.open_quantity > 0 && !asks.empty()){
            auto ask_it = asks.begin();
            Price best_ask_price = ask_it->first;
            if(!is_market && taker.price < best_ask_price){
                break;
            }

            PriceLevel& level = ask_it->second;

            while(taker.open_quantity > 0 && !level.empty()){
                Order& maker = level.front();
                Quantity fill_qty = std::min(taker.open_quantity, maker.open_quantity);

                taker.fill(fill_qty);
                maker.fill(fill_qty);
                level.reduce_volume(fill_qty);

                emit_trade(maker.id, taker.id, maker.price, fill_qty, taker.timestamp);

                if(maker.is_filled()){
                    book_.orders_map().erase(maker.id);
                    level.pop_front();
                }
            }

            if(level.empty()){
                asks.erase(ask_it);
            }
        }
    }

    void match_sell_against_bids(Order& taker, bool is_market){
        auto& bids = book_.bids();

        while(taker.open_quantity > 0 && !bids.empty()){
            auto bid_it = bids.begin();
            Price best_bid_price = bid_it->first;

            if(!is_market && taker.price > best_bid_price){
                break; 
            }

            PriceLevel& level = bid_it->second;

            while(taker.open_quantity > 0 && !level.empty()){
                Order& maker = level.front();
                Quantity fill_qty = std::min(taker.open_quantity, maker.open_quantity);

                taker.fill(fill_qty);
                maker.fill(fill_qty);
                level.reduce_volume(fill_qty);

                emit_trade(maker.id, taker.id, maker.price, fill_qty, taker.timestamp);

                if(maker.is_filled()){
                    book_.orders_map().erase(maker.id);
                    level.pop_front();
                }
            }

            if(level.empty()){
                bids.erase(bid_it);
            }
        }
    }

    OrderBook book_{};
    std::vector<Trade> trades_{};
    TradeListener trade_listener_{nullptr};

};
}