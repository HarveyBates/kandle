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

#include <cxxopts.hpp>
#include <iostream>

#include "kandle/handlers/kandle_3d_model_hander.h"
#include "kandle/handlers/kandle_file_handler.h"
#include "kandle/kandle_directory.h"

int main(int argc, char** argv) {
  // Default options
  cxxopts::Options options("Kandle",
                           "KiCAD 3rd Party Component Management Tool");

  // Set args and their help information
  options.add_options()("I,init", "Initialise a KiCAD project with Kandle.",
                        cxxopts::value<bool>())

      ("L,list", "List existing component libraries.", cxxopts::value<bool>())

          ("f,filename",
           "Path to zipped (.zip) component file (from "
           "symbol vendors).",
           cxxopts::value<std::string>())

              ("l,library",
               "Name of the library the component belongs to. "
               "E.g. op-amps for an LM358 IC.",
               cxxopts::value<std::string>())

                  ("s,save",
                   "Save the (.zip) file to $KANDLE_DEFAULT_DIR for quick "
                   "adding to future projects.",
                   cxxopts::value<bool>())

                      ("d,delete",
                       "Delete a part (provide part name without extension).",
                       cxxopts::value<std::string>())

                          ("h,help", "Display help information.");

  // Get arguments from user input
  auto result = options.parse(argc, argv);

  // Check if arg contains help (then display help information)
  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    exit(0);
  }

  // Ensure <project_name>.kicad_pro is in current directory, exit otherwise
  Kandle::DirectoryStructure::validate_working_directory();

  // Get environment variable where downloaded libraries are stored
  const char* kandle_path = std::getenv("KANDLE_DEFAULT_PATH");
  if (kandle_path == nullptr) {
    std::cout << "Error locating $KANDLE_DEFAULT_PATH, insure it is defined"
                 " in your ~/.bashrc or ~/.zshrc"
              << std::endl;
  }

  // Setup Kandle directory structure, only needs to be called once on project
  // initialisation
  if (result.count("init")) {
    Kandle::DirectoryStructure::initialise();
    exit(0);
  }

  // Delete a part
  if (result.count("delete")) {
    if (!result.count("library")) {
      std::cerr << "A valid library name is required to delete a part."
                << std::endl;
      exit(1);
    }
    std::string part_name = result["delete"].as<std::string>();
    std::string library_name = result["library"].as<std::string>();
    exit(Kandle::FileHandler::delete_component(library_name, part_name));
  }

  // List symbols in symbols directory
  if (result.count("list")) {
    Kandle::DirectoryStructure::list();
    exit(0);
  }

  // Ensure user has provided the "-l" or "library" target library flag
  if (!result.count("library")) {
    std::cerr << "Library not provided. "
                 "A valid library name must be provided "
                 "(see kandle --help). "
                 "Exiting."
              << std::endl;
    exit(1);
  }

  // Ensure user has provided the "-f" or "filename" flag
  if (!result.count("filename")) {
    std::cerr << "Filename not provided. "
                 "A valid filename name must be provided "
                 "(see kandle --help). "
                 "Exiting."
              << std::endl;
    exit(1);
  }

  // User input is valid and flags are present now preform tasks

  // Get the path of the .zip file
  std::string zip_path = result["filename"].as<std::string>();

  // Unzip the file
  Kandle::FileHandler::unzip(zip_path);

  // Save zip file for future projects (optional)
  // Requires global variable `KANDLE_DEFAULT_PATH` to exist
  if (result.count("save") && kandle_path != nullptr) {
    std::string str(kandle_path);
    Kandle::FileHandler::store_zip_in_global_directory(zip_path, kandle_path);
  }

  // Get the library name as a string
  std::string library_name = result["library"].as<std::string>();

  // If available get the path of the symbol, footprint and 3D Model located in
  // the tmp directory
  Kandle::FileHandler::FilePaths files =
      Kandle::FileHandler::get_tmp_file_paths(library_name);

  // Move files from the tmp dir into their respective place and do any
  // conversions
  Kandle::SymbolHandler::add_to_project(files.symbol, library_name);
  Kandle::FootprintHandler::add_to_project(files.footprint, library_name);
  Kandle::Model3DHander::add_to_project(files.dmodel, library_name);

  return 0;
}
