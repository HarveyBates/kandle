//  MIT License
//
//  Copyright (c) 2023 Harvey Bates
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

//
// Created by HBates on 20/1/2024.
//

#include "kandle/handlers/kandle_symbol_handler.h"

namespace fs = std::filesystem;

std::string Kandle::SymbolHandler::get_path(const std::string& library_name) {
  return DirectoryStructure::get_directory_path(
             DirectoryStructure::DIR_SYMBOLS) +
         "/" + library_name + ".kicad_sym";
}

bool Kandle::SymbolHandler::add_to_project(const std::string& tmp_file_path,
                                           const std::string& library_name) {
  std::string component_library_path;
  std::string component_path;
  std::string component_name;

  // Symbol path not found (probably error unpacking .zip file)
  if (std::empty(tmp_file_path)) {
    return false;
  }

  // Get the symbol path
  // e.g. /component/extern/symbols/<library_name>.kicad_sym
  component_library_path = get_path(library_name);

  // Get component name for linking symbol to footprint
  component_name = fs::path(tmp_file_path).parent_path().filename();

  // Library doesn't exist so create one
  if (!fs::exists(component_library_path)) {
    return new_symbol_library(tmp_file_path, component_library_path,
                              component_name);
  }

  // Append to existing symbol library
  return append_to_existing_symbol_library(
      tmp_file_path, component_library_path, component_name);
}

bool Kandle::SymbolHandler::new_symbol_library(
    const std::string& tmp_file_path, const std::string& component_library_path,
    const std::string& component_name) {
  std::vector<std::string> lines = Utils::read_lines_from_file(tmp_file_path);

  // Create new file
  std::cout << "Creating new symbol library: " << component_library_path
            << std::endl;

  std::fstream symbol_file(component_library_path,
                           std::fstream::out | std::fstream::app);

  if (symbol_file.is_open()) {
    for (auto& line : lines) {
      if (std::empty(line)) {
        continue;
      }

      // Replace footprint with footprint path
      if (line.find(" (property \"Footprint\"") != std::string::npos) {
        link_footprint_to_symbol(line, component_library_path, component_name);
      }

      // Replace un-formatted component name with formatted component name
      if (line.find(" (symbol \"") != std::string::npos) {
        std::regex replace_regex("\"([^\"]*)\"");
        std::string fmt_name_line = std::regex_replace(
            line, replace_regex, "\"" + component_name + "\"",
            std::regex_constants::format_first_only);
        symbol_file << fmt_name_line << "\n";
        continue;
      }

      symbol_file << line << "\n";
    }
    symbol_file.close();
  } else {
    return false;
  }

  return true;
}

bool Kandle::SymbolHandler::append_to_existing_symbol_library(
    const std::string& tmp_file_path, const std::string& component_library_path,
    const std::string& component_name) {
  std::vector<std::string> lines = Utils::read_lines_from_file(tmp_file_path);

  // Append to existing file
  std::vector<std::string> existing_lines =
      Utils::read_lines_from_file(component_library_path);

  long line_number = 0;
  bool valid_library = false;
  for (const auto& line : existing_lines) {
    if (line.find("(kicad_symbol_lib") != std::string::npos) {
      valid_library = true;
    }

    if (line.find(fs::path(tmp_file_path).stem()) != std::string::npos) {
      std::cout << "Component already exists in symbol library." << std::endl;
      return true;
    }

    if (!valid_library) {
      line_number++;
    }
  }

  // Check if the file contains kicad_symbol_lib, if not exit error
  if (!valid_library) {
    std::cerr << "Invalid KiCad symbol library. Exiting." << std::endl;
    exit(1);
  }

  // Ignore symbol header when appending a symbol to library also assign
  // footprint to symbol
  int symbol_header = 0;
  int index = 0;
  for (auto& line : lines) {
    if (line.find("(kicad_symbol_lib") != std::string::npos) {
      symbol_header = index;
    }

    if (line.find("Footprint") != std::string::npos) {
      link_footprint_to_symbol(line, component_library_path, component_name);
    }

    // Replace un-formatted component name with formatted component name
    if (line.find(" (symbol \"") != std::string::npos) {
      std::regex replace_regex("\"([^\"]*)\"");
      line =
          std::regex_replace(line, replace_regex, "\"" + component_name + "\"",
                             std::regex_constants::format_first_only);
    }

    index++;
  }

  // Get the index of the last non-empty line
  index = 0;
  int symbol_footer = 0;
  for (auto line = lines.rbegin(); line != lines.rend(); ++line) {
    if (!lines.empty()) {
      symbol_footer = index;
    }
    index++;
  }

  // Append the contents of the symbol to the existing symbol library under
  // the line containing "kicad_symbol_lib"
  existing_lines.insert((existing_lines.begin() + line_number + 1),
                        lines.begin() + symbol_header + 1,
                        lines.end() - (long)(lines.size() - symbol_footer));

  // Write the appended contents back to the file (overwrites)
  std::fstream symbol_file(component_library_path, std::fstream::out);
  if (symbol_file.is_open()) {
    for (const auto& line : existing_lines) {
      // Ignore empty lines
      if (!std::empty(line)) {
        symbol_file << line << "\n";
      }
    }
    symbol_file.close();
  } else {
    return false;
  }

  return true;
}

/**
 * @brief Replaces the footprint entry for the symbol file with Kandle footprint
 * identifier.
 *
 * @example OP_AMP:LM358 (library:component) where OP_AMP exists as a directory
 * inside footprints called "OP_AMP.pretty" and LM358 exists as a file called
 * "LM358.kicad_mod".
 *
 * @param line Line in the .kicad_sym file to modify.
 */
void Kandle::SymbolHandler::link_footprint_to_symbol(
    std::string& line, const std::string& component_library_path,
    const std::string& component_name) {
  std::string footprint_path;

  std::regex re(R"("Footprint" ".*")");

  footprint_path += R"("Footprint" ")";
  footprint_path += fs::path(component_library_path).stem();
  footprint_path += ":";
  footprint_path += component_name + ".kicad_mod";
  footprint_path += R"(")";

  line = std::regex_replace(line, re, footprint_path);
}

bool Kandle::SymbolHandler::remove_from_project(const std::string& library_name,
                                                const std::string& part_name) {
  std::string symbol_path =
      Kandle::DirectoryStructure::get_absolute_directory_path(
          DirectoryStructure::DIR_SYMBOLS);

  if (symbol_path.empty()) {
    return false;
  }
  std::string symbol_file_path =
      symbol_path + "/" + library_name + ".kicad_sym";

  // Asset file exists
  if (!fs::exists(symbol_file_path)) {
    return false;
  }

  // Going to create this temporary file
  std::string tmp_file_path = symbol_path + "/.tmp_" + library_name + ".txt";

  // Get input (actual) and output (tmp) files
  std::ifstream symbol_file(symbol_file_path);
  std::ofstream tmp_file(tmp_file_path);

  if (!symbol_file.is_open() || !tmp_file.is_open()) {
    return false;
  }

  std::string search_symbol = "(symbol \"" + part_name + "\"";
  bool delete_line = false, symbol_found = false;
  std::string line;
  int opening_count = 0, closing_count = 0;
  while (getline(symbol_file, line)) {
    // Check if symbol line
    if (!delete_line && (line.find(search_symbol) == std::string::npos ||
                         line.find("(in_bom") == std::string::npos)) {
      // Write line to tmp file
      tmp_file << line << "\n";
      continue;
    }

    symbol_found = true;

    delete_line = true;

    // Look for closing bracket
    for (const char c : line) {
      if (c == '(') {
        opening_count++;
      }
      if (c == ')') {
        closing_count++;
      }

      if (opening_count > 0 && closing_count > 0) {
        if (opening_count == closing_count) {
          // Closing bracket found
          delete_line = false;
        }
      }
    }
  }

  symbol_file.close();
  tmp_file.close();

  if (!symbol_found) {
    return false;
  }

  // Remove library file
  if (std::remove(symbol_file_path.c_str()) != 0) {
    return false;
  }

  // Rename tmp file to old library file
  if (std::rename(tmp_file_path.c_str(), symbol_file_path.c_str()) != 0) {
    return false;
  }

  return true;
}

std::string Kandle::SymbolHandler::convert_symbol(
    const std::string& legacy_symbol_path) {
  Legacy legacy;
  Symbol symbol;
  std::string filename;
  std::string converted_path;
  std::string new_symbol_path;

  converted_path = fs::path(legacy_symbol_path).parent_path();
  converted_path += "/";

  new_symbol_path = converted_path;
  new_symbol_path += fs::path(legacy_symbol_path).stem();
  new_symbol_path += ".kicad_sym";

  std::vector<std::string> lines =
      Utils::read_lines_from_file(legacy_symbol_path);

  // Parse legacy file
  if (!legacy.convert(lines)) {
    std::cerr << "Error converting file. Submit an issue. Exiting."
              << std::endl;
    exit(1);
  }

  // Covert legacy library to .kicad_sym in the same directory
  if (!symbol.new_from_legacy(&legacy, new_symbol_path)) {
    std::cerr << "Error converting file. Submit an issue. Exiting."
              << std::endl;
    exit(1);
  }

  // Get and return the new converted file path
  for (const auto& dir_item : fs::directory_iterator{converted_path}) {
    auto item = fs::path(dir_item);
    if (item.extension() == ".kicad_sym") {
      return item;
    }
  }

  std::cerr << "Error converting file. Submit an issue. Exiting." << std::endl;

  exit(1);
}
