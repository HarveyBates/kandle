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

#include "kandle/handlers/kandle_footprint_handler.h"

namespace fs = std::filesystem;

std::string Kandle::FootprintHandler::get_path(
    const std::string& library_name) {
  return DirectoryStructure::get_directory_path(
             DirectoryStructure::DIR_FOOTPRINTS) +
         "/" + library_name + ".pretty";
}

/**
 * @brief Constrain footprint silkscreen text size to the default of 1 x 1 with
 * a thickness of 0.15.
 *
 * Lines such as this:
 * @verbatim
 * (effects (font (size 0.64 0.64) (thickness 0.15)))
 * @endverbatim
 * Will be modified to this:
 * @verbatim
 * (effects (font (size 1 1) (thickness 0.15)))
 * @endverbatim
 *
 * @param line Line to modify.
 */
void Kandle::FootprintHandler::constrain_text_size(std::string& line) {
  int idx = 0;
  int offset = 0;
  std::string font_text[3] = {"1", "1", "0.15"};
  std::regex re(R"(\d+(\.\d+)?)");
  std::smatch match;

  std::cout << "Constraining footprint silkscreen text size..." << std::endl;

  while (std::regex_search(line.cbegin() + offset, line.cend(), match, re)) {
    if (idx >= 3) {
      return;
    }
    int match_pos = (int)match.position() + offset;

    line.replace(match_pos, match.length(), font_text[idx]);

    offset = match_pos + (int)font_text[idx].length();
    idx++;
  }
}

bool Kandle::FootprintHandler::modify(const std::string& component_path,
                                      const std::string& library_name) {
  std::cout << "Modifying footprint..." << std::endl;

  // Create a temporary file to hold modified footprint
  std::string tmp_footprint_path = get_path(library_name) + "/.tmp.txt";

  // Get input (actual) and output (tmp) files
  std::fstream footprint_file(component_path);
  std::ofstream tmp_footprint_file(tmp_footprint_path);

  if (!footprint_file.is_open() || !tmp_footprint_file.is_open()) {
    return false;
  }

  // Find font and replace font size
  std::string line;
  while (getline(footprint_file, line)) {
    // Replace font size with default font size
    if (line.find("(effects (font (size ") != std::string::npos) {
      constrain_text_size(line);
    }

    tmp_footprint_file << line << "\n";
  }

  footprint_file.close();
  tmp_footprint_file.close();

  // Remove library file
  if (std::remove(component_path.c_str()) != 0) {
    return false;
  }

  // Rename tmp file to old library file
  if (std::rename(tmp_footprint_path.c_str(), component_path.c_str()) != 0) {
    return false;
  }

  return true;
}

bool Kandle::FootprintHandler::add_to_project(const std::string& tmp_file_path,
                                              const std::string& library_name) {
  std::string component_library_path;
  std::string component_path;
  std::string component_name;

  // No footprint library found (check that it exists in the tmp directory)
  if (std::empty(tmp_file_path)) {
    return false;
  }

  // Get the footprint path
  // e.g. /component/extern/footprints/<library_name>.pretty
  component_library_path = get_path(library_name);

  // Create library directory if it doesn't exist
  if (!exists(fs::path(component_library_path))) {
    fs::create_directories(component_library_path);
  }

  // Get the new component name
  component_name = fs::path(tmp_file_path).parent_path().filename();

  // This renames the file from whatever it was when unzipped to the correctly
  // formatted new component name
  component_path += component_library_path + "/";
  component_path += component_name;
  component_path += ".kicad_mod";

  // Copy file from tmp dir to new component path (with new filename)
  Utils::copy_files_binary(tmp_file_path, component_path);

  // Modify newly created file
  return modify(component_path, library_name);
}

bool Kandle::FootprintHandler::remove_from_project(
    const std::string& library_name, const std::string& part_name) {
  // Delete footprint
  std::string footprint_path =
      Kandle::DirectoryStructure::get_absolute_directory_path(
          DirectoryStructure::DIR_FOOTPRINTS);

  if (footprint_path.empty()) {
    return false;
  }

  // Location of footprint file (inside <library>.pretty)
  // Keeps <library>.pretty even if this is the last footprint otherwise link
  // error with KiCAD
  std::string footprint_file_path = footprint_path + "/" + library_name +
                                    ".pretty/" + part_name + ".kicad_mod";

  if (!fs::exists(footprint_file_path)) {
    return false;
  }

  // Validate successful removal
  return (std::remove(footprint_file_path.c_str()) == 0);
}
