#include "lob/MatchingEngine.hpp"
#include <iostream>
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

// Test 1: Exact matching where both Maker and Taker are 100% filled
void test_exact_fill() {
    MatchingEngine engine;

    // Maker: Sell 10 shares @ 100.00
    engine.submit_order(Order{1, 10000, 10, Side::Sell});
    TEST_ASSERT(engine.book().total_orders() == 1, "Maker should be in book");

    // Taker: Buy 10 shares @ 100.00 (Crosses spread!)
    engine.submit_order(Order{2, 10000, 10, Side::Buy});

    // Invariant: Both orders should be fully filled and removed from the book
    TEST_ASSERT(engine.book().empty(), "Book should be empty after exact match");
    TEST_ASSERT(engine.trades().size() == 1, "Exactly 1 trade should be recorded");

    const Trade& t = engine.trades()[0];
    TEST_ASSERT(t.maker_order_id == 1, "Maker must be Order #1");
    TEST_ASSERT(t.taker_order_id == 2, "Taker must be Order #2");
    TEST_ASSERT(t.price == 10000, "Trade price must be 10000");
    TEST_ASSERT(t.quantity == 10, "Trade quantity must be 10");
}

// Test 2: Invariant - Trade executes at Maker's price (Price Improvement)
void test_price_improvement() {
    MatchingEngine engine;

    // Maker offers to sell @ 99.00
    engine.submit_order(Order{1, 9900, 10, Side::Sell});

    // Aggressive Taker is willing to pay up to 105.00
    engine.submit_order(Order{2, 10500, 10, Side::Buy});

    TEST_ASSERT(engine.trades().size() == 1, "1 trade should occur");
    // KEY INVARIANT: Trade price must be 9900 (Maker's price), NOT 10500!
    TEST_ASSERT(engine.trades()[0].price == 9900, "Trade must execute at Maker's price (9900)");
}

// Test 3: Partial Fill & FIFO Priority Retention
void test_partial_fill_fifo_priority() {
    MatchingEngine engine;

    // Alice arrives 1st: Sells 50 @ 100.00
    engine.submit_order(Order{1, 10000, 50, Side::Sell});
    // Bob arrives 2nd: Sells 20 @ 100.00
    engine.submit_order(Order{2, 10000, 20, Side::Sell});

    // Charlie buys 20 @ 100.00 (Crosses spread)
    engine.submit_order(Order{3, 10000, 20, Side::Buy});

    // Charlie should match with Alice (FIFO priority)
    TEST_ASSERT(engine.trades().size() == 1, "1 trade should occur");
    TEST_ASSERT(engine.trades()[0].maker_order_id == 1, "Alice should be filled first");
    TEST_ASSERT(engine.trades()[0].quantity == 20, "Trade volume should be 20");

    // Alice should still have 30 shares resting, and Bob has 20 shares
    auto best_ask = engine.book().best_ask();
    TEST_ASSERT(best_ask.has_value(), "Ask level should still exist");
    TEST_ASSERT(best_ask->volume == 50, "Remaining level volume should be 30 + 20 = 50");
    TEST_ASSERT(best_ask->order_count == 2, "There should still be 2 resting orders");

    // FIFO Verification: Next buy should STILL match against Alice's remaining 30 shares!
    engine.submit_order(Order{4, 10000, 10, Side::Buy});
    TEST_ASSERT(engine.trades().size() == 2, "Second trade should occur");
    TEST_ASSERT(engine.trades()[1].maker_order_id == 1, "Alice must retain priority for remaining 30 shares");
}

// Test 4: Sweeping Multiple Price Levels and Resting Unfilled Remainder
void test_sweep_multiple_levels() {
    MatchingEngine engine;

    // Populate asks at 3 distinct price levels
    engine.submit_order(Order{1, 10000, 10, Side::Sell}); // 10 @ 100.00
    engine.submit_order(Order{2, 10100, 15, Side::Sell}); // 15 @ 101.00
    engine.submit_order(Order{3, 10200, 20, Side::Sell}); // 20 @ 102.00

    // Aggressive Buyer wants 30 shares with limit price 101.50
    // Should match: 10 @ 100.00, 15 @ 101.00, and remaining 5 cannot match 102.00!
    engine.submit_order(Order{4, 10150, 30, Side::Buy});

    TEST_ASSERT(engine.trades().size() == 2, "Should sweep exactly 2 levels");
    TEST_ASSERT(engine.trades()[0].price == 10000 && engine.trades()[0].quantity == 10, "Trade 1: 10 @ 10000");
    TEST_ASSERT(engine.trades()[1].price == 10100 && engine.trades()[1].quantity == 15, "Trade 2: 15 @ 10100");

    // The unfilled 5 shares must now rest as the Best Bid @ 101.50
    auto best_bid = engine.book().best_bid();
    auto best_ask = engine.book().best_ask();

    TEST_ASSERT(best_bid.has_value() && best_bid->price == 10150, "Remaining 5 shares must rest at Best Bid 10150");
    TEST_ASSERT(best_bid->volume == 5, "Resting bid volume must be 5");
    TEST_ASSERT(best_ask.has_value() && best_ask->price == 10200, "Best ask should now be 10200");
}

// Test 5: Market Order Execution & Discard of Unfilled Remainder
void test_market_order() {
    MatchingEngine engine;

    engine.submit_order(Order{1, 10000, 10, Side::Sell}); // 10 @ 100.00

    // Market Buy for 25 shares (wants 15 more shares than available in book)
    engine.submit_order(Order{2, 0, 25, Side::Buy, OrderType::Market});

    // Should fill 10 shares and completely empty the ask book
    TEST_ASSERT(engine.trades().size() == 1, "1 trade should occur");
    TEST_ASSERT(engine.trades()[0].quantity == 10, "10 shares traded");

    // Market orders NEVER rest in the book!
    TEST_ASSERT(engine.book().empty(), "Market order remainder must NOT rest in book");
}

// Test 6: Observer Pattern - Trade Callback Notification
void test_trade_listener() {
    MatchingEngine engine;

    int callback_count = 0;
    Quantity total_traded_volume = 0;

    // Change line 140 to:
    engine.set_trade_listener([&](const Trade& t) {
        callback_count++;
        total_traded_volume += t.quantity;
    });

    engine.submit_order(Order{1, 10000, 10, Side::Sell});
    engine.submit_order(Order{2, 10000, 10, Side::Buy});

    TEST_ASSERT(callback_count == 1, "Listener should be called exactly once");
    TEST_ASSERT(total_traded_volume == 10, "Total traded volume recorded in listener should be 10");
}

} // anonymous namespace

int main() {
    std::cout << "========================================\n";
    std::cout << "    RUNNING MATCHING ENGINE TESTS       \n";
    std::cout << "========================================\n";

    RUN_TEST(test_exact_fill);
    RUN_TEST(test_price_improvement);
    RUN_TEST(test_partial_fill_fifo_priority);
    RUN_TEST(test_sweep_multiple_levels);
    RUN_TEST(test_market_order);
    RUN_TEST(test_trade_listener);

    std::cout << "========================================\n";
    std::cout << "  🎉 ALL MATCHING ENGINE TESTS PASSED! 🎉\n";
    std::cout << "========================================\n";
    return 0;
}