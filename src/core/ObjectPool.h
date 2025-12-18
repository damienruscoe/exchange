#pragma once

#include <vector>

// A simple object pool for arbitrary types
template <typename T> class ObjectPool {
public:
  ObjectPool(size_t size = 100000) {
    pool_.reserve(size);
    for (size_t i = 0; i < size; ++i) {
      pool_.emplace_back();
    }
    free_list_.reserve(size);
    for (size_t i = 0; i < size; ++i) {
      free_list_.push_back(&pool_[i]);
    }
  }

  T *acquire() {
    if (free_list_.empty()) {
      throw std::runtime_error("ObjectPool exhausted");
    }
    T *obj = free_list_.back();
    free_list_.pop_back();
    return obj;
  }

  void release(T *obj) { free_list_.push_back(obj); }

private:
  std::vector<T> pool_;
  std::vector<T *> free_list_;
};
