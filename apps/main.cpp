#include "lob/OrderBook.hpp"
#include <iostream>
#include <iomanip>

namespace {

void print_formatted_price(lob::Price price){
    double display_price = static_cast<double>(price) / 100.0;
    std::cout << std::fixed << std::setprecision(2) << "₹" << std::setw(7) << display_price;
}

void print_order_book(const lob::OrderBook& book) {
    std::cout << "\n==================================================\n";
    std::cout << "           LIMIT ORDER BOOK DEPTH LADDER          \n";
    std::cout << "==================================================\n";
    std::cout << " Side | Orders |  Volume  |   Price    \n";
    std::cout << "------+--------+----------+------------\n";

    const auto& asks = book.asks();
    if (asks.empty()) {
        std::cout << "           [ No Asks in Book ]            \n";
    }
    else{
        for(auto it = asks.rbegin(); it != asks.rend(); ++it){
            const auto& [price, level] = *it;
            std::cout << " ASK | "
                << std::setw(6) << level.order_count() << " | "
                << std::setw(8) << level.total_volume() << " |  ";

            print_formatted_price(price);
            if (it == std::prev(asks.rend())) {
                std::cout << "  <-- BEST ASK";
            }
            std::cout << "\n";
        }
    }

    std::cout << "--------------------------------------------------\n";
    auto spread = book.spread();
    if (spread.has_value()) {
        double spread_val = static_cast<double>(*spread) / 100.0;
        std::cout << " SPREAD: ₹" << std::fixed << std::setprecision(2) << spread_val
                  << " (" << *spread << " ticks)\n";
    } else {
        std::cout << " SPREAD: N/A (One or both sides empty)\n";
    }
    std::cout << "--------------------------------------------------\n";

    const auto& bids = book.bids();
    if (bids.empty()) {
        std::cout << "           [ No Bids in Book ]            \n";
    } else {
        for (auto it = bids.begin(); it != bids.end(); ++it) {
            const auto& [price, level] = *it;
            std::cout << "  BID | " 
                      << std::setw(6) << level.order_count() << " | "
                      << std::setw(8) << level.total_volume() << " |  ";
            print_formatted_price(price);
            if (it == bids.begin()) {
                std::cout << "  <-- BEST BID";
            }
            std::cout << "\n";
        }
    }
    std::cout << "==================================================\n";
    std::cout << " Total Orders: " << book.total_orders() << "\n\n";

}
    
}

int main(){
    using namespace lob;

    std::cout << ">>> Initializing Limit Order Book Engine <<<\n";
    OrderBook book;

    std::cout << "\n[1] Adding resting BID orders...\n";
    book.add_order(Order{1, 10000, 20, Side::Buy}); // Alice: 20 @ 100.00
    book.add_order(Order{2, 10000, 15, Side::Buy}); // Bob:   15 @ 100.00 (Same price level!)
    book.add_order(Order{3,  9950, 20, Side::Buy}); // Charlie: 20 @ 99.50
    book.add_order(Order{4,  9900, 50, Side::Buy}); // Dave:   50 @ 99.00

    std::cout << "[2] Adding resting ASK orders...\n";
    book.add_order(Order{5, 10100, 10, Side::Sell}); // Emma:  10 @ 101.00
    book.add_order(Order{6, 10150, 25, Side::Sell}); // Frank: 25 @ 101.50
    book.add_order(Order{7, 10150, 20, Side::Sell}); // Grace: 20 @ 101.50 (Same price level!)
    book.add_order(Order{8, 10200, 30, Side::Sell}); // Henry: 30 @ 102.00

    print_order_book(book);

    auto best_bid = book.best_bid();
    auto best_ask = book.best_ask();
    if (best_bid && best_ask) {
        std::cout << ">>> BBO Check <<<\n";
        std::cout << "Best Bid: ₹" << (best_bid->price / 100.0) 
                  << " (Aggregate Volume: " << best_bid->volume 
                  << " across " << best_bid->order_count << " orders)\n";
        std::cout << "Best Ask: ₹" << (best_ask->price / 100.0) 
                  << " (Aggregate Volume: " << best_ask->volume 
                  << " across " << best_ask->order_count << " orders)\n";
    }

    std::cout << "\n[3] Canceling Order #1 (Alice: 20 @ ₹100.00)...\n";
    bool canceled = book.cancel_order(1);
    std::cout << "Cancellation status: " << (canceled ? "SUCCESS" : "FAILED") << "\n";
    print_order_book(book);

    std::cout << "[4] Canceling Order #5 (Emma: 10 @ ₹101.00 - empties price level)...\n";
    book.cancel_order(5);
    print_order_book(book);

    std::cout << "[5] Attempting to add duplicate Order #2...\n";
    bool duplicate_added = book.add_order(Order{2, 10000, 50, Side::Buy});
    std::cout << "Duplicate add result: " << (duplicate_added ? "UNEXPECTED SUCCESS" : "REJECTED (Correct)") << "\n";
    return 0;
}