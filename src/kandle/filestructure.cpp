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

#include "kandle/filestructure.h"

namespace fs = std::filesystem;

static const std::vector<std::string> dirs = {
        "components",
        "components/extern",
        "components/extern/symbols",
        "components/extern/footprints",
        "components/extern/3dmodels",
        "components/extern/tmp",
        "components/extern/resources"
};

bool Kandle::FileStructure::validate_directory() {

    std::string path = fs::current_path();

    for (const auto& dir_item: fs::directory_iterator{path}) {
        const auto item = fs::path(dir_item);
        if (item.extension() == ".kicad_pro") {
            return true;
        }
    }

    std::cerr
            << "KiCAD project not found in current working directory. Exiting."
            << std::endl;

    exit(1);
}

bool Kandle::FileStructure::create_directory(const std::string& relative_path,
                                             std::size_t& n_existing) {
    if (fs::exists(relative_path)) {
        n_existing++;
        return true;
    }
    return fs::create_directories(relative_path);
}

bool Kandle::FileStructure::initialise() {

    std::cout << "Initialising Kandle directory structure...";

    std::size_t n_existing = 0;
    for (const auto& dir: dirs) {
        if (!create_directory(dir, n_existing)) {
            std::cerr << "\nUnable to create directory: " << dir << " Exiting."
                      << std::endl;
            exit(1);
        }
    }

    if (n_existing == dirs.size()) {
        std::cout << " directories already exist." << std::endl;
    } else {
        std::cout << " complete." << std::endl;
    }

    return true;
}

void Kandle::FileStructure::list() {
    int count = 0;

    std::string path = fs::current_path();
    path += "/components/extern/symbols";

    for (const auto& dir_item: fs::directory_iterator{path}) {
        auto item = fs::path(dir_item);
        if (item.extension() == ".kicad_sym") {
            std::cout << item.stem().string() << std::endl;
            count++;
        }
    }

    if (count == 0) {
        std::cout << "No libraries found." << std::endl;
        exit(1);
    }
}
