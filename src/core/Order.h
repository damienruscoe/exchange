#pragma once

using Price = int64_t;
using Quantity = uint32_t;
using OrderId = uint64_t;

// Forward declaration
struct OrderList;

// Represents a single order on the book, also a node in an intrusive list
struct Order {
  OrderId order_id;
  Price price;
  Quantity quantity;
  char side;
  Order *prev = nullptr;
  Order *next = nullptr;
  OrderList *list = nullptr; // Pointer back to the list it's in
};

// Represents the head and tail of an intrusive list of orders
struct OrderList {
  Order *head = nullptr;
  Order *tail = nullptr;
};
