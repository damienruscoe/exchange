#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

#include "databento/dbn_decoder.hpp"
#include "databento/file_stream.hpp"
#include "databento/log.hpp"
#include "databento/record.hpp"

#include "FlatMapOrderBook.h"
#include "OrderBook.h"

std::vector<databento::MboMsg>
load_mbo_msgs(const std::filesystem::path &file_path) {
  std::vector<databento::MboMsg> msgs;
  databento::NullLogReceiver log_receiver;
  databento::InFileStream file_stream{file_path};
  databento::DbnDecoder decoder{&log_receiver, std::move(file_stream)};

  decoder.DecodeMetadata();

  while (const databento::Record *record = decoder.DecodeRecord()) {
    if (record->RType() == databento::RType::Mbo) {
      msgs.push_back(record->Get<databento::MboMsg>());
    }
  }
  return msgs;
}

std::vector<databento::MboMsg> mbo_msgs_;

static void BM_OrderBook_ProcessMsgLatency(benchmark::State &state) {
  OrderBook order_book;
  size_t i = 0;

  for (auto _ : state) {
    order_book.ProcessMboMsg(mbo_msgs_[i]);
    i = (i + 1) % mbo_msgs_.size();
  }
}
BENCHMARK(BM_OrderBook_ProcessMsgLatency);

static void BM_FlatMapOrderBook_ProcessMsgLatency(benchmark::State &state) {
  FlatMapOrderBook order_book;
  size_t i = 0;

  for (auto _ : state) {
    order_book.ProcessMboMsg(mbo_msgs_[i]);
    i = (i + 1) % mbo_msgs_.size();
  }
}
BENCHMARK(BM_FlatMapOrderBook_ProcessMsgLatency);

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <path_to_dbn_file>" << std::endl;
    return 1;
  }

  const std::string dbn_file_path = argv[1];
  mbo_msgs_ = load_mbo_msgs(dbn_file_path);

  if (mbo_msgs_.empty()) {
    std::cerr << "Error: No MBO messages loaded from " << dbn_file_path
              << std::endl;
    return 1;
  }

  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  return 0;
}
