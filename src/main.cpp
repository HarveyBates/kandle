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

#include <iostream>
#include <cxxopts.hpp>
#include "utils.hpp"
#include "eschema/legacy.hpp"
#include "eschema/release.hpp"
#include "kandle/filestructure.h"
#include "kandle/filehandler.h"

int main(int argc, char** argv) {
    cxxopts::Options options("kandle",
                             "KiCAD 3rd Party Component Management Tool");

    options.add_options()
            ("I,init", "Initialise a KiCAD project with Kandle",
             cxxopts::value<bool>())

            ("f,filename", "Path to zipped (.zip) component file",
             cxxopts::value<std::string>())

            ("o,output-file", "Path for converted file",
             cxxopts::value<std::string>())

            ("h,help", "Print usage");

    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    Kandle::FileStructure::validate_directory();

    if (result.count("init")) {
        Kandle::FileStructure::initialise();
    }

    std::string filename;
    Legacy legacy;
    if (result.count("filename")) {
        filename = result["filename"].as<std::string>();
        Kandle::FileHandler::unzip(filename);
        Kandle::FileHandler::recursive_identify_files();

        //std::vector<std::string> lines =
        //        Utils::readlines(filename);
        //legacy.convert(lines);
    }

    //Symbol symbol;
    //if (result.count("output-file")) {
    //    filename = result["output-file"].as<std::string>();
    //    symbol.new_from_legacy(&legacy, filename);
    //}

    return 0;
}

