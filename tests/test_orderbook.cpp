#include "lob/Orderbook.hpp"
#include <cassert>
#include <iostream>


#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "FAILED: " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
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

void test_empty_book() {
    OrderBook book;
    TEST_ASSERT(book.empty(), "New book should be empty");
    TEST_ASSERT(book.total_orders() == 0, "Total orders should be 0");
    TEST_ASSERT(!book.best_bid().has_value(), "Best bid should be nullopt for empty book");
    TEST_ASSERT(!book.best_ask().has_value(), "Best ask should be nullopt for empty book");
    TEST_ASSERT(!book.spread().has_value(), "Spread should be nullopt for empty book");
}

void test_bbo_and_spread() {
    OrderBook book;
    book.add_order(Order{1, 10000, 10, Side::Buy});  // Bid @ 100.00
    book.add_order(Order{2,  9900, 20, Side::Buy});  // Bid @ 99.00
    book.add_order(Order{3, 10200, 15, Side::Sell}); // Ask @ 102.00
    book.add_order(Order{4, 10300,  5, Side::Sell}); // Ask @ 103.00
    auto best_bid = book.best_bid();
    auto best_ask = book.best_ask();
    auto spread   = book.spread();
    TEST_ASSERT(best_bid.has_value() && best_bid->price == 10000, "Best bid must be 10000");
    TEST_ASSERT(best_bid->volume == 10, "Best bid volume must be 10");
    TEST_ASSERT(best_ask.has_value() && best_ask->price == 10200, "Best ask must be 10200");
    TEST_ASSERT(best_ask->volume == 15, "Best ask volume must be 15");
    TEST_ASSERT(spread.has_value() && *spread == 200, "Spread must be 10200 - 10000 = 200");
}

void test_price_level_aggregation() {
    OrderBook book;
    book.add_order(Order{1, 10000, 10, Side::Buy});
    book.add_order(Order{2, 10000, 25, Side::Buy});
    book.add_order(Order{3, 10000, 15, Side::Buy});
    TEST_ASSERT(book.bid_level_count() == 1, "There should be exactly 1 price level");
    TEST_ASSERT(book.total_orders() == 3, "Total orders should be 3");
    auto bb = book.best_bid();
    TEST_ASSERT(bb.has_value(), "Best bid must exist");
    TEST_ASSERT(bb->volume == 50, "Aggregate volume should be 10 + 25 + 15 = 50");
    TEST_ASSERT(bb->order_count == 3, "Order count should be 3");
}

void test_cancellation_and_pruning() {
    OrderBook book;
    book.add_order(Order{1, 10000, 10, Side::Buy});
    book.add_order(Order{2, 10000, 20, Side::Buy});

    bool c1 = book.cancel_order(1);
    TEST_ASSERT(c1 == true, "Cancellation of existing order should succeed");
    TEST_ASSERT(book.total_orders() == 1, "Total orders should be 1");
    TEST_ASSERT(book.best_bid()->volume == 20, "Remaining volume should be 20");

    bool c2 = book.cancel_order(2);
    TEST_ASSERT(c2 == true, "Cancellation of second order should succeed");
    TEST_ASSERT(book.total_orders() == 0, "Total orders should be 0");
    TEST_ASSERT(book.bid_level_count() == 0, "Price level should be pruned from map");
    TEST_ASSERT(!book.best_bid().has_value(), "Best bid should now be nullopt");

    bool c_fake = book.cancel_order(999);
    TEST_ASSERT(c_fake == false, "Canceling non-existent order should return false");
}

void test_duplicate_order_id() {
    OrderBook book;
    bool first = book.add_order(Order{100, 5000, 10, Side::Buy});
    bool second = book.add_order(Order{100, 5000, 20, Side::Buy});
    TEST_ASSERT(first == true, "First order must be accepted");
    TEST_ASSERT(second == false, "Duplicate order ID must be rejected");
    TEST_ASSERT(book.total_orders() == 1, "Order book size must remain 1");
}
}

int main() {
    std::cout << "========================================\n";
    std::cout << "     RUNNING ORDER BOOK UNIT TESTS      \n";
    std::cout << "========================================\n";
    RUN_TEST(test_empty_book);
    RUN_TEST(test_bbo_and_spread);
    RUN_TEST(test_price_level_aggregation);
    RUN_TEST(test_cancellation_and_pruning);
    RUN_TEST(test_duplicate_order_id);
    std::cout << "========================================\n";
    std::cout << "  🎉 ALL ORDER BOOK TESTS PASSED! 🎉   \n";
    std::cout << "========================================\n";
    return 0;
}