#include "lob/Portfolio.hpp"
#include <iostream>
#include <cmath>
#include <cassert>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "❌ FAILED: " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
            std::exit(1); \
        } \
    } while (0)

#define RUN_TEST(test_func) \
    do { \
        std::cout << "[ RUN      ] " << #test_func << "\n"; \
        test_func(); \
        std::cout << "[       OK ] " << #test_func << "\n"; \
    } while (0)

namespace {

using namespace lob;

// Helper to compare floating-point values with epsilon tolerance
bool approx_equal(double a, double b, double eps = 0.001) {
    return std::abs(a - b) < eps;
}

// Test 1: Opening a new long position and verifying cash and fee deductions
void test_open_long_position() {
    // Initial cash: ₹100,000.00 (10,000,000 paise), 5 bps fee
    Portfolio portfolio{100000.0, 5.0};

    // Buy 10 shares @ ₹100.00 (Trade value = ₹1,000.00)
    // Fee = 1,000 * (5 / 10000) = ₹0.50
    portfolio.on_trade(Side::Buy, 100, 10);

    TEST_ASSERT(portfolio.position() == 10, "Position should be +10");
    TEST_ASSERT(approx_equal(portfolio.avg_entry_price(), 100.0), "Avg entry price should be 100.0");
    TEST_ASSERT(approx_equal(portfolio.total_fees(), 0.50), "Fee should be 0.50");
    TEST_ASSERT(approx_equal(portfolio.cash(), 100000.0 - 1000.0 - 0.50), "Cash deducted correctly");
    TEST_ASSERT(portfolio.realized_pnl() == 0.0, "No realized PnL yet");
}

// Test 2: Scaling into a position (Averaging in)
void test_scaling_and_avg_price() {
    Portfolio portfolio{100000.0, 0.0}; // 0 bps fee for clean arithmetic

    // Buy 10 @ 100.00 (Cost = 1000)
    portfolio.on_trade(Side::Buy, 100, 10);
    // Buy 10 @ 110.00 (Cost = 1100, Total Cost = 2100)
    portfolio.on_trade(Side::Buy, 110, 10);

    TEST_ASSERT(portfolio.position() == 20, "Position should be +20");
    // New avg entry price = 2100 / 20 = 105.00
    TEST_ASSERT(approx_equal(portfolio.avg_entry_price(), 105.0), "Avg entry price should be 105.0");
}

// Test 3: Partial Exit, Realized PnL, and Preserving Cost Basis
void test_partial_exit_realized_pnl() {
    Portfolio portfolio{100000.0, 0.0};

    // Position: 20 shares @ avg 105.00
    portfolio.on_trade(Side::Buy, 100, 10);
    portfolio.on_trade(Side::Buy, 110, 10);

    // Sell 10 shares @ 120.00 (Profit per share = 120 - 105 = 15.00)
    // Expected Realized PnL = 10 * 15.00 = +150.00
    portfolio.on_trade(Side::Sell, 120, 10);

    TEST_ASSERT(portfolio.position() == 10, "Remaining position should be +10");
    TEST_ASSERT(approx_equal(portfolio.realized_pnl(), 150.0), "Realized PnL should be +150.0");
    // KEY INVARIANT: Cost basis of the remaining 10 shares remains 105.00!
    TEST_ASSERT(approx_equal(portfolio.avg_entry_price(), 105.0), "Entry price for remaining shares stays 105.0");
}

// Test 4: Position Flip (Long -> Short)
void test_position_flip() {
    Portfolio portfolio{100000.0, 0.0};

    // Long 5 shares @ 100.00
    portfolio.on_trade(Side::Buy, 100, 5);

    // Sell 15 shares @ 110.00:
    // 5 shares close the long: Realized PnL = 5 * (110 - 100) = +50.00
    // Remaining 10 shares open a NEW SHORT position @ 110.00
    portfolio.on_trade(Side::Sell, 110, 15);

    TEST_ASSERT(portfolio.position() == -10, "Position must be -10 (Short)");
    TEST_ASSERT(approx_equal(portfolio.realized_pnl(), 50.0), "Realized PnL from closed long must be +50.0");
    TEST_ASSERT(approx_equal(portfolio.avg_entry_price(), 110.0), "New Short entry price must be 110.0");
}

// Test 5: Mark-to-Market Valuation, Unrealized PnL, and Max Drawdown
void test_mark_to_market_and_drawdown() {
    Portfolio portfolio{100000.0, 0.0};

    // Buy 10 shares @ 100.00 (Spent 1000. Cash = 99,000)
    portfolio.on_trade(Side::Buy, 100, 10);

    // Market moves up to 120.00:
    // Unrealized PnL = 10 * (120 - 100) = +200.00
    // Equity = 99,000 + (10 * 120) = 100,200.00 (New Peak Equity!)
    portfolio.update_market_price(120);
    TEST_ASSERT(approx_equal(portfolio.unrealized_pnl(), 200.0), "Unrealized PnL should be +200.0");
    TEST_ASSERT(approx_equal(portfolio.equity(), 100200.0), "Equity should be 100200.0");
    TEST_ASSERT(approx_equal(portfolio.peak_equity(), 100200.0), "Peak equity should be 100200.0");

    // Market crashes down to 90.00:
    // Equity = 99,000 + (10 * 90) = 99,900.00
    // Drawdown = Peak (100,200) - Current (99,900) = 300.00
    portfolio.update_market_price(90);
    TEST_ASSERT(approx_equal(portfolio.unrealized_pnl(), -100.0), "Unrealized PnL should be -100.0");
    TEST_ASSERT(approx_equal(portfolio.equity(), 99900.0), "Equity should be 99900.0");
    TEST_ASSERT(approx_equal(portfolio.max_drawdown(), 300.0), "Max Drawdown should be 300.0");
}

} // anonymous namespace

int main() {
    std::cout << "========================================\n";
    std::cout << "       RUNNING PORTFOLIO TESTS          \n";
    std::cout << "========================================\n";

    RUN_TEST(test_open_long_position);
    RUN_TEST(test_scaling_and_avg_price);
    RUN_TEST(test_partial_exit_realized_pnl);
    RUN_TEST(test_position_flip);
    RUN_TEST(test_mark_to_market_and_drawdown);

    std::cout << "========================================\n";
    std::cout << "    🎉 ALL PORTFOLIO TESTS PASSED! 🎉   \n";
    std::cout << "========================================\n";
    return 0;
}