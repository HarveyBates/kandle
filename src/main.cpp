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
#include "kandle/filestructure.h"
#include "kandle/filehandler.h"

int main(int argc, char** argv) {
    cxxopts::Options options("kandle",
                             "KiCAD 3rd Party Component Management Tool");

    options.add_options()
            ("I,init", "Initialise a KiCAD project with Kandle.",
             cxxopts::value<bool>())

            ("L,list", "List existing component libraries.",
             cxxopts::value<bool>())

            ("f,filename", "Path to zipped (.zip) component file (from "
                           "symbol vendors).",
             cxxopts::value<std::string>())

            ("l,library", "Name of the library the component belongs to. "
                          "E.g. op-amps for an LM358 IC.",
             cxxopts::value<std::string>())

            ("h,help", "Help information.");

    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    Kandle::FileStructure::validate_directory();

    if (result.count("list")) {
        Kandle::FileStructure::list();
        exit(0);
    }

    if (result.count("init")) {
        Kandle::FileStructure::initialise();
        exit(0);
    }

    if (!result.count("library")) {
        std::cerr << "Library not provided. "
                     "A valid library name must be provided "
                     "(see kandle --help). "
                     "Exiting."
                  << std::endl;
        exit(1);
    }


    if (!result.count("filename")) {
        std::cerr << "Filename not provided. "
                     "A valid filename name must be provided "
                     "(see kandle --help). "
                     "Exiting."
                  << std::endl;
        exit(1);
    }

    std::string filename;
    std::string library_name = result["library"].as<std::string>();

    filename = result["filename"].as<std::string>();
    Kandle::FileHandler::unzip(filename);

    Kandle::FileHandler::FilePaths files =
            Kandle::FileHandler::recursive_extract_paths(library_name);

    Kandle::FileHandler::import_symbol(files.symbol);
    Kandle::FileHandler::import_footprint(files.footprint);
    Kandle::FileHandler::import_3dmodel(files.dmodel);

    return 0;
}

