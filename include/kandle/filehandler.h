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

// TODO handle other OS

namespace Kandle {
    class FileHandler {
        static bool validate_zip_file(const std::string& path);

        static std::string output_directory;

        static struct ComponentFilePaths {
            std::string symbol;
            std::string footprint;
            std::string dmodel;
        } component_file_paths;

    public:
        static std::string unzip(const std::string& path);

        static bool recursive_identify_files();
    };
} // namespace Kandle

#endif //KANDLE_FILEHANDLER_H
