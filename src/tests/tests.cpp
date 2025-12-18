#include "OrderBook.h"
#include "FlatMapOrderBook.h"
#include "gtest/gtest.h"

//using OrderBook_t = OrderBook;
using OrderBook_t = FlatMapOrderBook;

databento::MboMsg CreateMboMsg(OrderId order_id, Price price, Quantity quantity,
                               char side, char action) {
  databento::MboMsg msg{};
  msg.hd.rtype = databento::RType::Mbo;
  msg.order_id = order_id;
  msg.price = price;
  msg.size = quantity;
  msg.side = static_cast<databento::Side>(side);
  msg.action = static_cast<databento::Action>(action);
  return msg;
}

TEST(OrderBookTest, AddSingleBid) {
  OrderBook_t book;
  auto msg = CreateMboMsg(1, 10000, 10, 'B', 'A');
  book.ProcessMboMsg(msg);
  EXPECT_EQ(book.GetBestBid(), 10000);
  EXPECT_EQ(book.GetBestAsk(), 0);
}

TEST(OrderBookTest, AddSingleAsk) {
  OrderBook_t book;
  auto msg = CreateMboMsg(1, 10100, 10, 'A', 'A');
  book.ProcessMboMsg(msg);
  EXPECT_EQ(book.GetBestAsk(), 10100);
  EXPECT_EQ(book.GetBestBid(), 0);
}

TEST(OrderBookTest, AddAndCancelBid) {
  OrderBook_t book;
  auto add_msg = CreateMboMsg(1, 10000, 10, 'B', 'A');
  book.ProcessMboMsg(add_msg);
  EXPECT_EQ(book.GetBestBid(), 10000);

  auto cancel_msg = CreateMboMsg(1, 10000, 0, 'B', 'C');
  book.ProcessMboMsg(cancel_msg);
  EXPECT_EQ(book.GetBestBid(), 0);
}

TEST(OrderBookTest, SimpleCross) {
  OrderBook_t book;
  auto ask_msg = CreateMboMsg(1, 10100, 10, 'A', 'A');
  book.ProcessMboMsg(ask_msg);
  EXPECT_EQ(book.GetBestAsk(), 10100);

  // This bid crosses the spread and should match
  auto bid_msg = CreateMboMsg(2, 10100, 5, 'B', 'A');
  book.ProcessMboMsg(bid_msg);

  // The bid should be partially filled, and the resting ask should be modified
  // Since the bid is fully filled, the best bid should be 0.
  // The ask was partially filled, so it should still be on the book.
  EXPECT_EQ(book.GetBestBid(), 0);
  EXPECT_EQ(book.GetBestAsk(), 10100);
}

TEST(OrderBookTest, FullCross) {
  OrderBook_t book;
  auto ask_msg = CreateMboMsg(1, 10100, 10, 'A', 'A');
  book.ProcessMboMsg(ask_msg);

  // This bid crosses and fills the entire resting ask
  auto bid_msg = CreateMboMsg(2, 10100, 10, 'B', 'A');
  book.ProcessMboMsg(bid_msg);

  // Both orders should be completely filled and removed from the book
  EXPECT_EQ(book.GetBestBid(), 0);
  EXPECT_EQ(book.GetBestAsk(), 0);
}

TEST(OrderBookTest, AddMultipleAndCancel) {
  OrderBook_t book;
  book.ProcessMboMsg(CreateMboMsg(1, 10000, 10, 'B', 'A'));
  book.ProcessMboMsg(CreateMboMsg(2, 10010, 10, 'B', 'A'));
  book.ProcessMboMsg(CreateMboMsg(3, 9990, 10, 'B', 'A'));
  EXPECT_EQ(book.GetBestBid(), 10010);

  book.ProcessMboMsg(CreateMboMsg(2, 10010, 0, 'B', 'C'));
  EXPECT_EQ(book.GetBestBid(), 10000);

  book.ProcessMboMsg(CreateMboMsg(1, 10000, 0, 'B', 'C'));
  EXPECT_EQ(book.GetBestBid(), 9990);
}
