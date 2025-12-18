#pragma once

#include <algorithm> // For std::lower_bound
#include <iostream>
#include <unordered_map>
#include <vector>

#include "ObjectPool.h"
#include "Order.h"
#include "databento/record.hpp"

// Custom FlatMap implementation using a sorted std::vector
template <typename Key, typename Value, typename Compare> class FlatMap {
public:
  using value_type = std::pair<Key, Value>;
  using container_type = std::vector<value_type>;
  using iterator = typename container_type::iterator;
  using const_iterator = typename container_type::const_iterator;

  FlatMap(size_t reserve_size = 1000) { data_.reserve(reserve_size); }

  iterator find(const Key &key) {
    auto it = std::lower_bound(data_.begin(), data_.end(), key,
                               [](const value_type &element, const Key &k) {
                                 return Compare()(element.first, k);
                               });
    if (it != data_.end() && !Compare()(key, it->first)) {
      return it;
    }
    return data_.end();
  }

  const_iterator find(const Key &key) const {
    auto it = std::lower_bound(data_.begin(), data_.end(), key,
                               [](const value_type &element, const Key &k) {
                                 return Compare()(element.first, k);
                               });
    if (it != data_.end() && !Compare()(key, it->first)) {
      return it;
    }
    return data_.end();
  }

  std::pair<iterator, bool> emplace(const Key &key, Value value) {
    auto it = std::lower_bound(data_.begin(), data_.end(), key,
                               [](const value_type &element, const Key &k) {
                                 return Compare()(element.first, k);
                               });
    if (it != data_.end() && !Compare()(key, it->first)) {
      return {it, false}; // Key already exists
    }
    return {data_.insert(it, {key, value}), true};
  }

  void erase(iterator it) { data_.erase(it); }

  bool empty() const { return data_.empty(); }

  size_t size() const { return data_.size(); }

  iterator begin() { return data_.begin(); }
  const_iterator begin() const { return data_.begin(); }
  iterator end() { return data_.end(); }
  const_iterator end() const { return data_.end(); }

  value_type &front() { return data_.front(); }
  const value_type &front() const { return data_.front(); }

private:
  container_type data_;
};

class FlatMapOrderBook {
public:
  FlatMapOrderBook();

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
  using BidBook = FlatMap<Price, OrderList *, std::greater<Price>>;
  using AskBook = FlatMap<Price, OrderList *, std::less<Price>>;
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
