#pragma once
#include "Types.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>


namespace lob {


class Portfolio{
public:

    explicit Portfolio(double initial_cash = 10000000.0 , double fee_bps = 5.0) noexcept 
      : initial_cash_(initial_cash),
        cash_(initial_cash),
        peak_equity_(initial_cash),
        fee_bps_(fee_bps){}
    

    void on_trade(Side side , Price price , Quantity quantity){
        if(quantity == 0) return;

        double trade_val = static_cast<double>(price) * static_cast<double>(quantity);
        double fee = trade_val * (fee_bps_/10000.0);
        total_fees_ += fee;
        total_trades_++;
        total_volume_++;

        int64_t trade_qty = (side == Side::Buy) ? static_cast<int64_t>(quantity) : -static_cast<int64_t>(quantity);

        if(side == Side::Buy){
            cash_ -= (trade_val + fee);
        }
        else{
            cash_ += (trade_val - fee);
        }

        if(position_ == 0){
            position_ = trade_qty;
            avg_entry_price_ = static_cast<double>(price);
        }
        else if((position_ > 0 && trade_qty > 0) || (position_ < 0 && trade_qty < 0)){
            double old_cost = std::abs(position_) * avg_entry_price_;
            double new_cost = old_cost + trade_val;
            position_ += trade_qty;
            avg_entry_price_ = new_cost / std::abs(position_);
        }
        else{
            int64_t closing_qty = std::min(std::abs(position_), std::abs(trade_qty));

            double pnl_per_share = (position_ > 0)
            ? (static_cast<double>(price) - avg_entry_price_) : (avg_entry_price_ - static_cast<double>(price));

            realized_pnl_ += (pnl_per_share * closing_qty);
            int64_t remaining_position = position_ + trade_qty;

            if(remaining_position == 0){
                position_ = 0;
                avg_entry_price_ = 0.0;
            }
            else if((position_ > 0 && remaining_position > 0) || 
                (position_ < 0 && remaining_position < 0)){
                position_ = remaining_position;
            }
            else{
                position_ = remaining_position;
                avg_entry_price_ = static_cast<double>(price);
            }
        }
    }

    void update_market_price(Price current_price) noexcept {
        current_price_= current_price;
        if(position_ == 0){
            unrealized_pnl_ = 0.0;
        }
        else if(position_ > 0){
            unrealized_pnl_ = position_ * (static_cast<double>(current_price) - avg_entry_price_);
        }
        else{
            unrealized_pnl_ = std::abs(position_) * (avg_entry_price_ - static_cast<double>(current_price));
        }

        double eq = equity();

        if(eq > peak_equity_){
            peak_equity_ = eq;
        }
        double current_dd = peak_equity_ - eq;
        if(current_dd > max_drawdown_){
            max_drawdown_ = current_dd;
        }
    }

    [[nodiscard]] double cash() const noexcept { return cash_; }
    [[nodiscard]] int64_t position() const noexcept { return position_; }
    [[nodiscard]] double avg_entry_price() const noexcept { return avg_entry_price_; }
    [[nodiscard]] double realized_pnl() const noexcept { return realized_pnl_; }
    [[nodiscard]] double unrealized_pnl() const noexcept { return unrealized_pnl_; }
    [[nodiscard]] double total_fees() const noexcept { return total_fees_; }
    [[nodiscard]] double peak_equity() const noexcept { return peak_equity_; }
    [[nodiscard]] double max_drawdown() const noexcept { return max_drawdown_; }
    [[nodiscard]] uint64_t total_trades() const noexcept { return total_trades_; }
    [[nodiscard]] uint64_t total_volume() const noexcept { return total_volume_; }
    [[nodiscard]] double equity() const noexcept {
    return cash_ + (position_ * static_cast<double>(current_price_));
    }
    [[nodiscard]] double return_pct() const noexcept {
        return ((equity() - initial_cash_) / initial_cash_) - 100.0;
    }

private : 
    double   initial_cash_{0.0};
    double   cash_{0.0};
    int64_t  position_{0};            // Signed inventory (+Long, -Short, 0 Flat)
    double   avg_entry_price_{0.0};   // Cost basis per share
    Price    current_price_{0};       // Latest mark-to-market price
    double   realized_pnl_{0.0};      // Permanently locked-in profit/loss
    double   unrealized_pnl_{0.0};    // Floating mark-to-market profit/loss
    double   total_fees_{0.0};        // Accumulated transaction fees
    double   peak_equity_{0.0};       // High-water mark for equity
    double   max_drawdown_{0.0};      // Maximum peak-to-trough decline
    double   fee_bps_{5.0};           // Fee in basis points (5 bps = 0.05%)
    uint64_t total_trades_{0};        // Number of executed fills
    uint64_t total_volume_{0};        // Total shares traded
};
}
