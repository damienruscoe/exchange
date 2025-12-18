#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "databento/dbn_decoder.hpp"
#include "databento/file_stream.hpp"
#include "databento/log.hpp"
#include "databento/record.hpp"

#include "cli.h"
#include "FlatMapOrderBook.h"
#include "OrderBook.h"

std::ostream &nl(std::ostream &os) { return os << '\n'; }

template <typename OrderBook>
void generate_json_output(const std::string& dbn_file_path, const std::string& output_json_path) {
    OrderBook order_book;
    std::cout << "Generating " << output_json_path << std::endl;
    std::ofstream output_file(output_json_path);

    if (!output_file.is_open()) {
        std::cerr << "Error: Could not open output file: " << output_json_path << std::endl;
        return;
    }

    output_file << "[" << nl;

    databento::NullLogReceiver log_receiver;
    databento::InFileStream file_stream{dbn_file_path};
    databento::DbnDecoder decoder{&log_receiver, std::move(file_stream)};

    decoder.DecodeMetadata();

    bool first_record = true;
    while (const databento::Record *record = decoder.DecodeRecord()) {
        if (record->RType() == databento::RType::Mbo) {
            const auto& msg = record->Get<databento::MboMsg>();
            order_book.ProcessMboMsg(msg);

            if (!first_record) {
                output_file << "," << nl;
            }
            output_file << "{" << nl;
            output_file << "  \"action\": \"" << msg.action << "\"," << nl;
            output_file << "  \"hd\": {" << nl;
            output_file << "    \"instrument_id\": " << msg.hd.instrument_id << "," << nl;
            output_file << "    \"length\": " << static_cast<int>(msg.hd.length) << "," << nl;
            output_file << "    \"publisher_id\": " << msg.hd.publisher_id << "," << nl;
            output_file << "    \"rtype\": " << static_cast<int>(msg.hd.rtype) << "," << nl;
            output_file << "    \"ts_event\": " << msg.hd.ts_event.time_since_epoch().count() << nl;
            output_file << "  }," << nl;
            output_file << "  \"levels\": [" << nl;
            order_book.Snapshot(output_file);
            output_file << "  ]," << nl;
            output_file << "  \"price\": " << msg.price << "," << nl;
            output_file << "  \"sequence\": " << msg.sequence << "," << nl;
            output_file << "  \"side\": \"" << msg.side << "\"," << nl;
            output_file << "  \"size\": " << msg.size << "," << nl;
            output_file << "  \"ts_recv\": " << msg.ts_recv.time_since_epoch().count() << nl;
            output_file << "}";
            first_record = false;
        }
    }

    output_file << nl << "]" << nl;
    output_file.close();
}

int main(int argc, char **argv) {
    for (const auto& dbn_file_path : cli::get_dbn_files(argc, argv)) {
        std::filesystem::path p(dbn_file_path);
        std::string filename = p.filename().string();

        std::filesystem::create_directories("artifacts/mbp");
        generate_json_output<OrderBook>(dbn_file_path, "artifacts/mbp/map_" + filename + ".json");

        std::filesystem::create_directories("artifacts/mbp");
        generate_json_output<FlatMapOrderBook>(dbn_file_path, "artifacts/mbp/flatmap_" + filename + ".json");
    }

    return 0;
}
