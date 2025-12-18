#include "FlatMapOrderBook.h"

FlatMapOrderBook::FlatMapOrderBook() = default;

int64_t FlatMapOrderBook::GetPosition() const { return position_; }
double FlatMapOrderBook::GetBookPnl() const { return book_pnl_; }

Price FlatMapOrderBook::GetBestBid() const {
  if (bids_.empty()) {
    return 0;
  }
  return bids_.front().first;
}

Price FlatMapOrderBook::GetBestAsk() const {
  if (asks_.empty()) {
    return 0;
  }
  return asks_.front().first;
}

void FlatMapOrderBook::ProcessMboMsg(const databento::MboMsg &msg) {
  switch (msg.action) {
  case 'A':
    AddOrder(msg);
    break;
  case 'C':
    CancelOrder(msg);
    break;
  case 'M':
    ModifyOrder(msg);
    break;
  case 'T':
    TradeOrder(msg);
    if (msg.side == 'B') {
        position_ += msg.size;
    } else {
        position_ -= msg.size;
    }
    break;
  case 'F':
    CancelOrder(msg);
    if (msg.side == 'B') {
        position_ += msg.size;
    } else {
        position_ -= msg.size;
    }
    break;
  default:
    break;
  }
  Match();
}

void FlatMapOrderBook::AddOrder(const databento::MboMsg &msg) {
  Order *order = order_pool_.acquire();
  order->order_id = msg.order_id;
  order->price = msg.price;
  order->quantity = msg.size;
  order->side = msg.side;
  order->next = nullptr;
  order->prev = nullptr;

  if (msg.side == 'B') {
    auto it = bids_.find(msg.price);
    if (it != bids_.end()) {
      AppendOrder(it->second, order);
    } else {
      OrderList *new_list = list_pool_.acquire();
      new_list->head = nullptr;
      new_list->tail = nullptr;
      bids_.emplace(msg.price, new_list);
      AppendOrder(new_list, order);
    }
  } else {
    auto it = asks_.find(msg.price);
    if (it != asks_.end()) {
      AppendOrder(it->second, order);
    } else {
      OrderList *new_list = list_pool_.acquire();
      new_list->head = nullptr;
      new_list->tail = nullptr;
      asks_.emplace(msg.price, new_list);
      AppendOrder(new_list, order);
    }
  }
  order_map_[order->order_id] = order;
}

void FlatMapOrderBook::CancelOrder(const databento::MboMsg &msg) {
  CancelOrderById(msg.order_id);
}

void FlatMapOrderBook::CancelOrderById(OrderId order_id) {
  auto map_it = order_map_.find(order_id);
  if (map_it == order_map_.end()) {
    return;
  }

  Order *order = map_it->second;
  RemoveOrder(order);
  order_map_.erase(map_it);

  // If the list is now empty, remove the price level
  if (order->list->head == nullptr) {
    if (order->side == 'B') {
      auto it = bids_.find(order->price);
      if (it != bids_.end()) {
        bids_.erase(it);
      }
    } else {
      auto it = asks_.find(order->price);
      if (it != asks_.end()) {
        asks_.erase(it);
      }
    }
    list_pool_.release(order->list);
  }

  order_pool_.release(order);
}

void FlatMapOrderBook::ModifyOrder(const databento::MboMsg &msg) {
  CancelOrderById(msg.order_id);
  AddOrder(msg);
}

void FlatMapOrderBook::TradeOrder(const databento::MboMsg &msg) {
  auto map_it = order_map_.find(msg.order_id);
  if (map_it == order_map_.end()) {
    return;
  }

  Order *order = map_it->second;
  if (msg.size >= order->quantity) {
    CancelOrderById(msg.order_id);
  } else {
    order->quantity -= msg.size;
  }
}

namespace {

size_t count(const OrderList *ol) {
  if (ol->head == nullptr)
    return 0;

  size_t count{1};
  const Order *cur = ol->head;

  while (ol->tail != cur) {
    ++count;
    cur = cur->next;
  }
  return count;
}

size_t count_size(const OrderList *ol) {
  if (ol->head == nullptr)
    return 0;

  const Order *cur = ol->head;
  size_t count = cur->quantity;

  while (ol->tail != cur) {
    cur = cur->next;
    count += cur->quantity;
  }
  return count;
}

} // namespace

void FlatMapOrderBook::Snapshot(std::ostream &os) const {
	unsigned top_count = 0;
	std::string comma = "";

	for (auto bi = bids_.begin(), ai = asks_.begin();
			bi != bids_.end() || ai != asks_.end();
			++top_count)
	{
		os << comma << "    {" << '\n';
		if (ai != asks_.end()) {
		  OrderList* al = ai->second;
			os << "      \"ask_ct\": " << count(al) << "," << '\n';
			os << "      \"ask_px\": " << ai->first << "," << '\n';
			os << "      \"ask_sz\": " << count_size(al) << "," << '\n';
			ai = std::next(ai);
		} else {
			os << "      \"ask_ct\": " << 0 << "," << '\n';
			os << "      \"ask_px\": " << 0 << "," << '\n';
			os << "      \"ask_sz\": " << 0 << "," << '\n';
		}
		if (bi != bids_.end()) {
		  OrderList* bl = bi->second;
			os << "      \"bid_ct\": " << count(bl) << "," << '\n';
			os << "      \"bid_px\": " << bi->first << "," << '\n';
			os << "      \"bid_sz\": " << count_size(bl) << '\n';
			bi = std::next(bi);
		} else {
			os << "      \"bid_ct\": " << 0 << "," << '\n';
			os << "      \"bid_px\": " << 0 << "," << '\n';
			os << "      \"bid_sz\": " << 0 << '\n';
		}
		os << "    }";
		comma = ",\n";
	}
}

void FlatMapOrderBook::AppendOrder(OrderList *list, Order *order) {
  order->list = list;
  if (list->tail == nullptr) {
    list->head = order;
    list->tail = order;
  } else {
    list->tail->next = order;
    order->prev = list->tail;
    list->tail = order;
  }
}

void FlatMapOrderBook::RemoveOrder(Order *order) {
  if (order->prev) {
    order->prev->next = order->next;
  } else {
    order->list->head = order->next;
  }

  if (order->next) {
    order->next->prev = order->prev;
  } else {
    order->list->tail = order->prev;
  }
}

void FlatMapOrderBook::Match() {
  while (!bids_.empty() && !asks_.empty()) {
    auto best_bid_it = bids_.begin();
    auto best_ask_it = asks_.begin();

    if (best_bid_it->first < best_ask_it->first) {
      break;
    }

    OrderList *bid_list = best_bid_it->second;
    OrderList *ask_list = best_ask_it->second;

    while (bid_list->head && ask_list->head) {
      Order *bid_order = bid_list->head;
      Order *ask_order = ask_list->head;

      Quantity trade_qty = std::min(bid_order->quantity, ask_order->quantity);

      bid_order->quantity -= trade_qty;
      ask_order->quantity -= trade_qty;

      bool bid_filled = (bid_order->quantity == 0);
      bool ask_filled = (ask_order->quantity == 0);

      if (bid_filled) {
        CancelOrderById(bid_order->order_id);
      }
      if (ask_filled) {
        CancelOrderById(ask_order->order_id);
      }

      if (!bid_filled && !ask_filled) {
        break;
      }
    }
  }
}
