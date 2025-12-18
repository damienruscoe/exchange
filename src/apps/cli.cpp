#include "cli.h"

#include <filesystem>
#include <iostream>

std::vector<std::string> cli::get_dbn_files(int argc, char **argv)
{
    std::vector<std::string> dbn_files;
    std::string input_path;

    if (argc < 2) {
        input_path = "../resources/test_data/";
        if (!std::filesystem::exists(input_path) || !std::filesystem::is_directory(input_path)) {
            std::cerr << "Default directory not found: " << input_path << std::endl;
            std::cerr << "Usage: " << argv[0] << " [path_to_dbn_file_or_directory]" << std::endl;
            exit(1);
        }
    } else {
        input_path = argv[1];
    }

    if (std::filesystem::is_directory(input_path)) {
        for (const auto& entry : std::filesystem::directory_iterator(input_path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".dbn") {
                dbn_files.push_back(entry.path().string());
            }
        }
    } else if (std::filesystem::is_regular_file(input_path)) {
        dbn_files.push_back(input_path);
    } else {
        std::cerr << "Error: Path is not a valid file or directory: " << input_path << std::endl;
        exit(1);
    }

    if (dbn_files.empty()) {
        std::cerr << "No .dbn files found in " << input_path << std::endl;
        exit(1);
    }
		return dbn_files;
}

