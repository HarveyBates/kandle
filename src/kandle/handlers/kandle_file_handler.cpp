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

#include "kandle/handlers/kandle_file_handler.h"

namespace fs = std::filesystem;

std::string output_directory;
static Kandle::FileHandler::FilePaths library_file_paths;

void Kandle::FileHandler::format_filename(std::string& filename) {
  // Remove ultra-librarian prefix
  if (filename.substr(0, 3) == "ul_") {
    filename = filename.substr(3);
  }

  // Remove CSE prefix
  if (filename.substr(0, 4) == "LIB_") {
    filename = filename.substr(4);
  }

  // Replace spaces and - with _
  std::replace(filename.begin(), filename.end(), ' ', '_');
  std::replace(filename.begin(), filename.end(), '-', '_');
}

bool Kandle::FileHandler::store_zip_in_global_directory(
    const std::string& zip_path, std::string kandle_path) {
  std::ifstream src(zip_path, std::ios::binary);

  validate_zip_file(zip_path);

  std::string filename = fs::path(zip_path).stem();
  format_filename(filename);
  if (kandle_path.back() != '/') {
    kandle_path += '/';
  }
  kandle_path += filename + ".zip";

  // Already exists skip
  if (fs::exists(kandle_path)) {
    std::cout << "Output directory: " << kandle_path << " already exists."
              << std::endl;
    return true;
  }

  std::ofstream dest(kandle_path, std::ios::binary);

  // Files couldn't be loaded
  if (!src || !dest) {
    std::cerr << "Invalid directory: " << kandle_path << std::endl;
    return false;
  }

  // Copy file
  dest << src.rdbuf();

  return true;
}

std::string Kandle::FileHandler::unzip(const std::string& path) {
  validate_zip_file(path);

  std::cout << "Extracting from: " << path << std::endl;

  std::string output_path = "components/extern/tmp/";
  std::string filename = fs::path(path).stem();

  format_filename(filename);

  output_path += filename;  // Add new filename to path

  std::cout << "Extracting to: " << output_path << std::endl;

  if (fs::exists(output_path)) {
    output_directory = output_path;
    return output_path;
  }

  std::ostringstream oss;
  oss << "unzip \"" << path << "\" -d \"" << output_path
      << "\" > /dev/null 2>&1";
  std::string cmd = oss.str();

  int err = std::system(cmd.c_str());
  if (err == 0) {
    output_directory = output_path;
    return output_path;
  }

  std::cerr << "Files could not be extracted from: " << path << std::endl;
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

void Kandle::FileHandler::build_library_paths(const std::string& library_name) {
  // File
  library_file_paths.symbol = "components/extern/symbols/";
  library_file_paths.symbol += library_name;
  library_file_paths.symbol += ".kicad_sym";

  // Directory
  library_file_paths.footprint = "components/extern/footprints/";
  library_file_paths.footprint += library_name;
  library_file_paths.footprint += ".pretty";

  // File
  library_file_paths.dmodel = "components/extern/3dmodels/";
  library_file_paths.dmodel += library_name;
}

Kandle::FileHandler::FilePaths Kandle::FileHandler::get_tmp_file_paths(
    const std::string& library_name) {
  bool symbol_found = false;
  bool footprint_found = false;
  bool dmodel_found = false;

  FilePaths component_file_paths;
  build_library_paths(library_name);

  for (const auto& dir_item :
       fs::recursive_directory_iterator{output_directory}) {
    auto item = fs::path(dir_item);

    if (item.stem() == "KiCad") {
      std::cout << "Component Search Engine component detected." << std::endl;
      symbol_found = false;
      footprint_found = false;
      dmodel_found = false;
    }

    if (!symbol_found && item.extension() == ".kicad_sym") {
      std::cout << "Found symbol: " << item << std::endl;
      component_file_paths.symbol = item;
      symbol_found = true;
    }

    if (!symbol_found && item.extension() == ".lib") {
      std::string symbol_path = item;
      component_file_paths.symbol = SymbolHandler::convert_symbol(symbol_path);
      symbol_found = true;
    }

    if (!footprint_found && item.extension() == ".kicad_mod") {
      std::cout << "Found footprint: " << item << std::endl;
      component_file_paths.footprint = item;
      footprint_found = true;
    }
    if (!dmodel_found &&
        (item.extension() == ".stp" || item.extension() == ".step")) {
      std::cout << "Found 3D model: " << item << std::endl;
      component_file_paths.dmodel = item;
      dmodel_found = true;
    }
  }

  return component_file_paths;
}

bool Kandle::FileHandler::confirm_deletion(const std::string& library_name,
                                           const std::string& part_name) {
  // Confirm from user
  std::cout << "Do you wish to delete \"" << library_name << "/" << part_name
            << "\"? (y/n)" << std::endl;
  char response;
  std::cin >> response;

  // Validate response and try to delete file
  return (response == 'y');
}

bool Kandle::FileHandler::delete_component(const std::string& library_name,
                                           const std::string& part_name) {
  if (!confirm_deletion(library_name, part_name)) {
    std::cout << "Cancelled the deletion of: " << library_name << "/"
              << part_name << std::endl;
    return false;
  }

  if (Kandle::SymbolHandler::remove_from_project(library_name, part_name)) {
    std::cout << "Successfully deleted symbol: " << library_name << "/"
              << part_name << std::endl;
  } else {
    std::cerr << "Error deleting symbol for: " << library_name << "/"
              << part_name << std::endl;
  }

  if (Kandle::FootprintHandler::remove_from_project(library_name, part_name)) {
    std::cout << "Successfully deleted footprint: " << library_name << "/"
              << part_name << std::endl;
  } else {
    std::cerr << "Error deleting footprint for: " << library_name << "/"
              << part_name << std::endl;
  }

  if (Kandle::Model3DHander::remove_from_project(library_name, part_name)) {
    std::cout << "Successfully deleted 3d model: " << library_name << "/"
              << part_name << std::endl;
  } else {
    std::cerr << "Error deleting 3d model for: " << library_name << "/"
              << part_name << std::endl;
  }

  return true;
}
