#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include "databento/dbn_decoder.hpp"
#include "databento/file_stream.hpp"
#include "databento/log.hpp"
#include "databento/record.hpp"

#include "cli.h"
#include "FlatMapOrderBook.h"
#include "OrderBook.h"

class Duration
{
	public:
		Duration() :
			start_time{std::chrono::high_resolution_clock::now()}
		{}

		long long operator*() const {
			return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
		}

	private:
		std::chrono::high_resolution_clock::time_point start_time;
};

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

int main(int argc, char **argv) {
		std::ofstream csv_file("artifacts/benchmark_results.csv");

    for (const auto& dbn_file_path : cli::get_dbn_files(argc, argv)) {
				mbo_msgs_ = load_mbo_msgs(dbn_file_path);
				std::filesystem::path file_path{dbn_file_path};

				if (mbo_msgs_.empty()) {
					std::cerr << "Error: No MBO messages loaded from " << dbn_file_path
										<< std::endl;
					return 1;
				}

				// Benchmark OrderBook
				{
					csv_file << file_path.filename().string() << "OrderBook," << mbo_msgs_.size() << ',';

					OrderBook order_book;
					Duration overall_duration;

					for (const auto& msg : mbo_msgs_) {
							Duration trade_duration;
							order_book.ProcessMboMsg(msg);
							csv_file << *trade_duration << ',';
					}
					csv_file << *overall_duration << "\n";
				}

				// Benchmark FlatMapOrderBook
				{
					csv_file << file_path.filename().string() << "FlatOrderBook," << mbo_msgs_.size() << ',';

					FlatMapOrderBook flat_map_order_book;
					Duration overall_duration;

					for (const auto& msg : mbo_msgs_) {
							Duration trade_duration;
							flat_map_order_book.ProcessMboMsg(msg);
							csv_file << *trade_duration << ',';
					}
					csv_file << *overall_duration << "\n";
				}

		}

		csv_file.close();
		std::cout << "Benchmark results written to benchmark_results.csv" << std::endl;

		return 0;
}

