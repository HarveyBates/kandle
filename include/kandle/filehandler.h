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

#ifndef KANDLE_FILEHANDLER_H
#define KANDLE_FILEHANDLER_H

#include <iostream>
#include <filesystem>
#include <string>
#include <sstream>
#include <vector>
#include <regex>
#include "eschema/release.hpp"
#include "eschema/legacy.hpp"
#include "utils.hpp"

// TODO handle other OS

namespace Kandle {
    class FileHandler {

        static bool validate_zip_file(const std::string& path);

        static std::string convert_symbol(
                const std::string& legacy_symbol_path);

        static bool new_symbol_library(const std::string& path);

        static bool append_to_symbol_library(const std::string& path);

        static void build_library_paths(const std::string& library_name);

        static void straight_copy(const std::string& source,
                                  const std::string& dest);

    public:
        struct FilePaths {
            std::string symbol;
            std::string footprint;
            std::string dmodel;
        };

        static std::string unzip(const std::string& path);

        static FilePaths
        recursive_extract_paths(const std::string& library_name);

        static bool import_symbol(const std::string& path);

        static void substitute_footprint(std::string& line);

        static bool import_footprint(const std::string& path);

        static bool import_3dmodel(const std::string& path);
    };
} // namespace Kandle

#endif //KANDLE_FILEHANDLER_H
