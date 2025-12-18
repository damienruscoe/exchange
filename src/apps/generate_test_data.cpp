#include <chrono>
#include <filesystem> // For std::filesystem::path
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <stdexcept> // For std::invalid_argument
#include <string>    // For std::string
#include <vector>

#include "databento/dbn_encoder.hpp"
#include "databento/enums.hpp"
#include "databento/file_stream.hpp" // For databento::OutFileStream
#include "databento/record.hpp"

// Enum for different market conditions
enum class MarketCondition {
  HighVolatility,
  FlashCrash,
  BookChurn,
  QuoteStuffing,
  LargeOrderImbalance,
  LiquidityDrain,
  PriceJump
};

// Helper to convert string to MarketCondition enum
MarketCondition parse_market_condition(const std::string &s) {
  if (s == "HighVolatility")
    return MarketCondition::HighVolatility;
  if (s == "FlashCrash")
    return MarketCondition::FlashCrash;
  if (s == "BookChurn")
    return MarketCondition::BookChurn;
  if (s == "QuoteStuffing")
    return MarketCondition::QuoteStuffing;
  if (s == "LargeOrderImbalance")
    return MarketCondition::LargeOrderImbalance;
  if (s == "LiquidityDrain")
    return MarketCondition::LiquidityDrain;
  if (s == "PriceJump")
    return MarketCondition::PriceJump;
  throw std::invalid_argument("Invalid market condition: " + s);
}

std::array AllMarketConditions = {
    "HighVolatility",
    "FlashCrash",
    "BookChurn",
    //"QuoteStuffing",
    "LargeOrderImbalance",
    "LiquidityDrain",
    "PriceJump",
};

// Helper to generate a random price within a range
int64_t random_price(std::mt19937 &gen, int64_t min_p, int64_t max_p) {
  std::uniform_int_distribution<int64_t> distrib(min_p, max_p);
  return distrib(gen);
}

// Helper to generate a random quantity
uint32_t random_quantity(std::mt19937 &gen, uint32_t min_q, uint32_t max_q) {
  std::uniform_int_distribution<uint32_t> distrib(min_q, max_q);
  return distrib(gen);
}

void generate_data(const std::string &output_filename,
                   MarketCondition condition) {
  const size_t num_messages = 500000; // Half a million messages
  const int64_t initial_mid_price =
      100000000000;                   // e.g., $100.00 with 7 decimal places
  const int64_t price_tick = 1000000; // 0.01
  uint32_t min_qty = 1;
  uint32_t max_qty = 100;

  std::random_device rd;
  std::mt19937 gen(rd());

  // Declare distributions outside the loop
  std::uniform_int_distribution<> action_dist(
      0, 100); // 0-100 for action probability
  std::uniform_int_distribution<> price_move_dist_gen(
      -10, 10); // Generator for mid price moves
  std::uniform_int_distribution<> side_dist(0, 1); // 0 for bid, 1 for ask

  // 1. Create Metadata
  databento::Metadata metadata;
  metadata.schema = databento::Schema::Mbo;
  metadata.start = std::chrono::system_clock::now();
  metadata.end = metadata.start; // Initialize 'end'
  metadata.symbols.emplace_back("TEST");
  metadata.stype_in = databento::SType::RawSymbol;
  metadata.stype_out = databento::SType::InstrumentId;
  metadata.limit = num_messages;
  metadata.dataset = "TEST_DATASET";
  metadata.version = 1;
  metadata.ts_out = false;
  metadata.symbol_cstr_len = 128; // Fixed length for symbols, as specified
  metadata.partial = {};          // Initialize 'partial'
  metadata.not_found = {};        // Initialize 'not_found'
  metadata.mappings = {};         // Initialize 'mappings'

  // 2. Construct DbnEncoder with Metadata and IWritable
  databento::OutFileStream file_stream{std::filesystem::path(output_filename)};
  databento::DbnEncoder encoder{metadata, &file_stream};

  int64_t current_mid_price = initial_mid_price;
  uint64_t next_order_id = 1;
  std::map<uint64_t, databento::MboMsg>
      active_orders; // To track orders for cancellations/modifications

  // Simulate initial book depth
  for (int i = 0; i < 100; ++i) {
    // Bids
    databento::MboMsg bid_msg{};
    bid_msg.hd.length = 14; // sizeof(MboMsg) / 4
    bid_msg.hd.rtype = databento::RType::Mbo;
    bid_msg.hd.ts_event =
        std::chrono::system_clock::now(); // Assign time_point directly
    bid_msg.order_id = next_order_id++;
    bid_msg.price = current_mid_price - (i + 1) * price_tick;
    bid_msg.size = random_quantity(gen, min_qty, max_qty);
    bid_msg.side = static_cast<databento::side::Side>(
        'B'); // Corrected: static_cast to enum
    bid_msg.action = static_cast<databento::action::Action>(
        'A');                      // Corrected: static_cast to enum
    encoder.EncodeRecord(bid_msg); // Pass only the record
    active_orders[bid_msg.order_id] = bid_msg;

    // Asks
    databento::MboMsg ask_msg{};
    ask_msg.hd.length = 14; // sizeof(MboMsg) / 4
    ask_msg.hd.rtype = databento::RType::Mbo;
    ask_msg.hd.ts_event =
        std::chrono::system_clock::now(); // Assign time_point directly
    ask_msg.order_id = next_order_id++;
    ask_msg.price = current_mid_price + (i + 1) * price_tick;
    ask_msg.size = random_quantity(gen, min_qty, max_qty);
    ask_msg.side = static_cast<databento::side::Side>(
        'A'); // Corrected: static_cast to enum
    ask_msg.action = static_cast<databento::action::Action>(
        'A');                      // Corrected: static_cast to enum
    encoder.EncodeRecord(ask_msg); // Pass only the record
    active_orders[ask_msg.order_id] = ask_msg;
  }

  // Parameters for FlashCrash
  const size_t flash_crash_start_message_idx =
      num_messages / 4; // Start 1/4 way through
  const size_t flash_crash_duration_messages =
      num_messages / 8; // Last for 1/8 of messages
  const int64_t flash_crash_price_drop_per_message =
      5 * price_tick; // Drop 5 ticks per message

  // Parameters for BookChurn
  const int book_churn_cancel_prob = 50;
  const int book_churn_modify_prob = 40;
  const uint32_t book_churn_min_qty = 1;
  const uint32_t book_churn_max_qty = 10;

  // Parameters for QuoteStuffing
  const int quote_stuffing_add_prob = 70;
  const int quote_stuffing_cancel_prob = 20;
  const int64_t quote_stuffing_price_offset = 50 * price_tick; // Far from mid
  const uint32_t quote_stuffing_min_qty = 1;
  const uint32_t quote_stuffing_max_qty = 5;

  // Parameters for LargeOrderImbalance
  const size_t imbalance_start_message_idx = num_messages / 3;
  const size_t imbalance_duration_messages = num_messages / 6;
  const int imbalance_side_bias = 90; // 90% chance for one side

  // Parameters for LiquidityDrain
  const size_t drain_start_message_idx = num_messages / 2;
  const size_t drain_duration_messages = num_messages / 8;
  const int drain_cancel_prob = 90;
  const int drain_add_prob = 5;

  // Parameters for PriceJump
  const size_t price_jump_interval = num_messages / 10;
  const int64_t price_jump_magnitude = 100 * price_tick;

  for (size_t i = 0; i < num_messages; ++i) {
    databento::MboMsg msg{};
    msg.hd.length = 14; // sizeof(MboMsg) / 4
    msg.hd.rtype = databento::RType::Mbo;
    msg.hd.ts_event =
        std::chrono::system_clock::now(); // Assign time_point directly

    int current_add_prob = 25;
    int current_cancel_prob = 30;
    int current_modify_prob = 20;
    int current_aggressive_prob = 25;
    int current_price_move_min = -10;
    int current_price_move_max = 10;
    uint32_t current_min_qty = min_qty;
    uint32_t current_max_qty = max_qty;
    int current_side_bias = 50; // 50% for bid, 50% for ask

    switch (condition) {
    case MarketCondition::HighVolatility: {
      // Default values are already set for HighVolatility
      break;
    }
    case MarketCondition::FlashCrash: {
      if (i >= flash_crash_start_message_idx &&
          i < flash_crash_start_message_idx + flash_crash_duration_messages) {
        current_cancel_prob = 70;
        current_add_prob = 10;
        current_aggressive_prob = 10;
        current_modify_prob = 10;
        current_mid_price -= flash_crash_price_drop_per_message;
      }
      break;
    }
    case MarketCondition::BookChurn: {
      current_cancel_prob = book_churn_cancel_prob;
      current_modify_prob = book_churn_modify_prob;
      current_add_prob = 100 - (book_churn_cancel_prob +
                                book_churn_modify_prob); // Remaining for adds
      current_aggressive_prob = 0;                       // No aggressive trades
      current_min_qty = book_churn_min_qty;
      current_max_qty = book_churn_max_qty;
      break;
    }
    case MarketCondition::QuoteStuffing: {
      current_add_prob = quote_stuffing_add_prob;
      current_cancel_prob = quote_stuffing_cancel_prob;
      current_modify_prob = 0;
      current_aggressive_prob =
          100 - (quote_stuffing_add_prob + quote_stuffing_cancel_prob);
      current_min_qty = quote_stuffing_min_qty;
      current_max_qty = quote_stuffing_max_qty;
      break;
    }
    case MarketCondition::LargeOrderImbalance: {
      if (i >= imbalance_start_message_idx &&
          i < imbalance_start_message_idx + imbalance_duration_messages) {
        current_side_bias =
            imbalance_side_bias;       // Bias towards one side (e.g., buy)
        current_max_qty = max_qty * 5; // Larger quantities
      }
      break;
    }
    case MarketCondition::LiquidityDrain: {
      if (i >= drain_start_message_idx &&
          i < drain_start_message_idx + drain_duration_messages) {
        current_cancel_prob = drain_cancel_prob;
        current_add_prob = drain_add_prob;
        current_modify_prob = 100 - (drain_cancel_prob + drain_add_prob);
        current_aggressive_prob = 0;
      }
      break;
    }
    case MarketCondition::PriceJump: {
      if (i > 0 && i % price_jump_interval == 0) {
        current_mid_price += price_jump_magnitude; // Sudden jump
      }
      break;
    }
    }

    int action_type = action_dist(gen);

    if (action_type < current_add_prob) { // Add a new order
      msg.action = static_cast<databento::action::Action>(
          'A'); // Corrected: static_cast to enum
      msg.order_id = next_order_id++;
      msg.size = random_quantity(gen, current_min_qty, current_max_qty);
      msg.side = (side_dist(gen) == 0)
                     ? static_cast<databento::side::Side>('B')
                     : static_cast<databento::side::Side>(
                           'A'); // Corrected: static_cast to enum

      if (condition == MarketCondition::QuoteStuffing) {
        if (msg.side == static_cast<databento::side::Side>('B')) {
          msg.price = current_mid_price - quote_stuffing_price_offset -
                      random_price(gen, 1, 10) * price_tick;
        } else {
          msg.price = current_mid_price + quote_stuffing_price_offset +
                      random_price(gen, 1, 10) * price_tick;
        }
      } else if (msg.side == static_cast<databento::side::Side>(
                                 'B')) { // Corrected: static_cast to enum
        msg.price =
            current_mid_price +
            random_price(gen, 1, 5) * price_tick; // Aggressive buy: price above
                                                  // current mid (to hit asks)
      } else {
        // Aggressive sell: price below current mid (to hit bids)
        msg.price = current_mid_price - random_price(gen, 1, 5) * price_tick;
      }
      active_orders[msg.order_id] = msg;
    } else if (action_type < current_add_prob + current_cancel_prob &&
               !active_orders.empty()) { // Cancel an existing order
      msg.action = static_cast<databento::action::Action>(
          'C'); // Corrected: static_cast to enum
      auto it = active_orders.begin();
      std::advance(it, std::uniform_int_distribution<size_t>(
                           0, active_orders.size() - 1)(gen));
      msg.order_id = it->first;
      msg.price = it->second.price;
      msg.side = it->second.side;
      msg.size = it->second.size; // Size is not strictly needed for cancel, but
                                  // good to populate
      active_orders.erase(it);
    } else if (action_type < current_add_prob + current_cancel_prob +
                                 current_modify_prob &&
               !active_orders.empty()) { // Modify an existing order
      msg.action = static_cast<databento::action::Action>(
          'M'); // Corrected: static_cast to enum
      auto it = active_orders.begin();
      std::advance(it, std::uniform_int_distribution<size_t>(
                           0, active_orders.size() - 1)(gen));
      msg.order_id = it->first;
      msg.side = it->second.side;
      msg.price =
          it->second
              .price; // Keep same price for simplicity, or randomly change
      msg.size = random_quantity(gen, current_min_qty,
                                 current_max_qty); // New random size
      active_orders[msg.order_id] = msg;           // Update active order map
    } else if (action_type <
               current_add_prob + current_cancel_prob + current_modify_prob +
                   current_aggressive_prob) { // Aggressive Trading (Add that
                                              // crosses the spread)
      msg.action = static_cast<databento::action::Action>(
          'A'); // Aggressive orders are still 'Add' messages
      msg.order_id = next_order_id++;
      msg.size =
          random_quantity(gen, current_max_qty * 2,
                          current_max_qty * 5); // Larger aggressive quantity

      // Bias side for LargeOrderImbalance
      if (condition == MarketCondition::LargeOrderImbalance &&
          i >= imbalance_start_message_idx &&
          i < imbalance_start_message_idx + imbalance_duration_messages) {
        msg.side =
            (std::uniform_int_distribution<>(0, 99)(gen) < current_side_bias)
                ? static_cast<databento::side::Side>('B')
                : static_cast<databento::side::Side>('A');
      } else {
        msg.side = (side_dist(gen) == 0)
                       ? static_cast<databento::side::Side>('B')
                       : static_cast<databento::side::Side>(
                             'A'); // Corrected: static_cast to enum
      }

      if (msg.side == static_cast<databento::side::Side>(
                          'B')) { // Corrected: static_cast to enum
        // Aggressive buy: price above current mid (to hit asks)
        msg.price = current_mid_price + random_price(gen, 5, 15) * price_tick;
      } else {
        // Aggressive sell: price below current mid (to hit bids)
        msg.price = current_mid_price - random_price(gen, 5, 15) * price_tick;
      }
      // Aggressive orders might not stay on the book, so don't add to
      // active_orders map
    }

    encoder.EncodeRecord(msg); // Pass only the record

    // Simulate mid-price movement for volatility
    if (condition != MarketCondition::FlashCrash) { // FlashCrash handles its
                                                    // own price movement
      price_move_dist_gen.param(std::uniform_int_distribution<int>::param_type(
          current_price_move_min, current_price_move_max));
      current_mid_price += price_move_dist_gen(gen) * price_tick;
    }
    if (current_mid_price < initial_mid_price / 2)
      current_mid_price = initial_mid_price / 2;
    if (current_mid_price > initial_mid_price * 2)
      current_mid_price = initial_mid_price * 2;
  }

  std::cout << "Generated " << num_messages << " messages to "
            << output_filename << std::endl;
}

int main(int argc, char *argv[]) {
  std::string output_dir{"resources/test_data/"};
  std::filesystem::create_directories(output_dir);

  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <MarketCondition>" << std::endl;

    for (auto &condition_name : AllMarketConditions) {
      MarketCondition condition = parse_market_condition(condition_name);
      std::string output_filename = output_dir + condition_name + ".dbn";
      generate_data(output_filename, condition);
    }
  } else {
    std::string condition_name = argv[1];
    MarketCondition condition = parse_market_condition(condition_name);
    std::string output_filename = output_dir + condition_name + ".dbn";
    generate_data(output_filename, condition);
  }
}
