/*
 * MIT License
 *
 * Copyright (c) 2023 Harvey Bates
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "kandle/filehandler.h"

namespace fs = std::filesystem;

std::string Kandle::FileHandler::unzip(const std::string& path) {

    validate_zip_file(path);

    std::cout << "Extracting from: " << path << std::endl;

    std::string output_path = "components/extern/tmp/";
    output_path += fs::path(path).stem(); // Gets the filename only

    std::cout << "Extracting to: " << output_path << std::endl;

    if (fs::exists(output_path)) {
        std::cout << "Output directory: " << output_path << " already exists."
                  << std::endl;
        output_directory = output_path;
        return output_path;
    }

    std::ostringstream oss;
    oss << "unzip \"" << path << "\" -d \"" << output_path
        << "\" > /dev/null 2>&1";
    std::string cmd = oss.str();

    int err = std::system(cmd.c_str());
    if (err == 0) {
        std::cout << "Successfully extracted to: " << output_path << std::endl;
        output_directory = output_path;
        return output_path;
    }

    std::cerr << "Files could not be extracted from: " << path
              << std::endl;
    exit(1);
}

// Check that the input file path actually contains a .zip file
bool Kandle::FileHandler::validate_zip_file(const std::string& path) {

    if (!fs::is_regular_file(path)) {
        std::cerr << "Error unknown file: \"" << path << "\". Exiting."
                  << std::endl;
        exit(1);
    }

    auto zip_path = fs::path(path);

    if (zip_path.extension() == ".zip") {
        return true;
    }

    std::cerr << "Error unsupported filetype: \"" << path << "\". Exiting."
              << std::endl;
    exit(1);
}

bool Kandle::FileHandler::recursive_identify_files() {

    for (const auto& dir_item: fs::recursive_directory_iterator{
            output_directory}) {
        auto item = fs::path(dir_item);

        if (!std::empty(component_file_paths.symbol) &&
            !std::empty(component_file_paths.footprint) &&
            !std::empty(component_file_paths.dmodel)) {
            return true;
        }

        if (item.extension() == ".kicad_sym") {
            std::cout << "Found symbol: " << item << std::endl;
            component_file_paths.symbol = item;
        }
        if (item.extension() == ".kicad_mod") {
            std::cout << "Found footprint: " << item << std::endl;
            component_file_paths.footprint = item;
        }
        if (item.extension() == ".stp" || item.extension() == ".step") {
            std::cout << "Found 3D model: " << item << std::endl;
            component_file_paths.dmodel = item;
        }
    }

    return false;
}



