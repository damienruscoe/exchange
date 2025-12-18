#pragma once

#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>

#include "ObjectPool.h"
#include "Order.h"
#include "databento/record.hpp"

class OrderBook {
public:
  OrderBook();

  void ProcessMboMsg(const databento::MboMsg &msg);

  void AddOrder(const databento::MboMsg &msg);
  void ModifyOrder(const databento::MboMsg &msg);
  void CancelOrder(const databento::MboMsg &msg);
  void CancelOrderById(OrderId order_id);
  void TradeOrder(const databento::MboMsg &msg);

  Price GetBestBid() const;
  Price GetBestAsk() const;

  void Snapshot(std::ostream &os) const;

private:
  using BidBook = std::map<Price, OrderList *, std::greater<Price>>;
  using AskBook = std::map<Price, OrderList *, std::less<Price>>;
  using OrderMap = std::unordered_map<OrderId, Order *>;

  void AppendOrder(OrderList *list, Order *order);
  void RemoveOrder(Order *order);

  void Match();

  BidBook bids;
  AskBook asks;
  OrderMap orders;

  ObjectPool<Order> order_pool;
  ObjectPool<OrderList> list_pool;
};
