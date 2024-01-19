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

#include "kandle/directory_structure.h"

namespace fs = std::filesystem;

/**
 * @brief Helper method to get a directory path from a Directories enum.
 * @param dir Directory enum.
 * @return Path to directory or empty if an error occurred.
 */
std::string Kandle::DirectoryStructure::get_directory_path(Directories dir) {
  switch (dir) {
    case DIR_COMPONENTS:
      return "/components";
    case DIR_COMPONENTS_EXTERN:
      return "/components/extern";
    case DIR_SYMBOLS:
      return "/components/extern/symbols";
    case DIR_FOOTPRINTS:
      return "/components/extern/footprints";
    case DIR_3D_MODELS:
      return "/components/extern/3dmodels";
    case DIR_TEMPORARY:
      return "/components/extern/tmp";
    default:
      break;
  }
  return "";
}

std::string Kandle::DirectoryStructure::get_absolute_directory_path(
    Directories dir) {
  std::string pwd = fs::current_path();
  std::string abs_path = get_directory_path(dir);
  if (abs_path.empty()) {
    return "";
  }
  return pwd + abs_path;
}

/**
 * @brief Checks to make sure kandle is operating in a projects root directory
 * (next to a <project_name>.kicad_pro file).
 * @return True or exit failure.
 */
bool Kandle::DirectoryStructure::validate_working_directory() {
  std::string path = fs::current_path();

  for (const auto& dir_item : fs::directory_iterator{path}) {
    const auto item = fs::path(dir_item);
    if (item.extension() == ".kicad_pro") {
      return true;
    }
  }

  std::cerr << "KiCAD project not found in current working directory. Exiting."
            << std::endl;

  exit(1);
}

/**
 * @brief Create a directory if it doesnt exist.
 * @param absolute_path Path to directory.
 * @return True if already exists or created successfully.
 */
bool Kandle::DirectoryStructure::create_directory(
    const std::string& absolute_path) {
  if (fs::exists(absolute_path)) {
    std::cout << "Directory exists (skipping): " << absolute_path << std::endl;
    return true;
  }

  return fs::create_directories(absolute_path);
}

/**
 * @brief Initialise the Kandle directory structure by creating all required
 * directories.
 * @return True if successful or exit error.
 */
bool Kandle::DirectoryStructure::initialise() {
  std::cout << "Creating Kandle directory structure..." << std::endl;

  Directories dirs[] = {DIR_COMPONENTS, DIR_COMPONENTS_EXTERN, DIR_SYMBOLS,
                        DIR_FOOTPRINTS, DIR_3D_MODELS,         DIR_TEMPORARY};

  for (const auto& dir : dirs) {
    // Get directory name
    std::string dir_name = get_absolute_directory_path(dir);
    if (dir_name.empty()) {
      std::cerr << "Directory type error." << std::endl;
      exit(1);
    }

    // Create directory if not already exists
    if (!create_directory(dir_name)) {
      std::cerr << "\nUnable to create directory: " << dir_name << " Exiting."
                << std::endl;
      exit(1);
    }
  }

  return true;
}

/**
 * @brief List components and whether or not they have a symbol, footprint and
 * 3D model.
 */
void Kandle::DirectoryStructure::list() {
  std::vector<DirList> dir_list;

  if (list_symbols(dir_list) == 0) {
    std::cout << "No libraries found." << std::endl;
    exit(1);
  }

  list_footprints(dir_list);

  list_3d_models(dir_list);

  // Create CLI table
  auto table = create_table(dir_list);

  // Print table
  std::cout << table << std::endl;
}

/**
 * @brief Append symbol names in alphabetical order to list of directories for
 * displaying to user.
 * @param dir_list DirList structure.
 * @return Count of symbols, zero if error.
 */
int Kandle::DirectoryStructure::list_symbols(std::vector<DirList>& dir_list) {
  std::string symbols_dir = get_absolute_directory_path(DIR_SYMBOLS);
  if (symbols_dir.empty()) {
    std::cerr << "Directory type error." << std::endl;
    exit(1);
  }

  // Get a list of symbol directories (to sort alphabetically)
  std::vector<fs::directory_entry> directories;
  for (const auto& dir : fs::directory_iterator{symbols_dir}) {
    directories.push_back(dir);
  }

  // Sort the directories alphabetically
  std::sort(directories.begin(), directories.end(),
            [](const fs::directory_entry& a, const fs::directory_entry& b) {
              return a.path().lexically_normal() < b.path().lexically_normal();
            });

  // Iterate through symbol directories
  int count = 0;
  for (const auto& dir : directories) {
    auto item = fs::path(dir);
    if (item.extension() != ".kicad_sym") {
      continue;
    }

    DirList dir_item;
    dir_item.library = item.stem().string();

    std::vector<std::string> lines = Utils::readlines(fs::path(dir));

    // Get a symbol names in a library
    for (const auto& line : lines) {
      if (std::empty(line)) {
        continue;
      }

      std::string symbol_name = Utils::get_symbol_name_from_file(line);
      if (symbol_name.empty()) {
        continue;
      }

      dir_item.symbol.push_back(symbol_name);
    }
    count++;

    dir_list.push_back(dir_item);
  }
  return count;
}

/**
 * @brief Append footprint names to list of directories for displaying to user.
 * @param dir_list DirList structure.
 */
void Kandle::DirectoryStructure::list_footprints(
    std::vector<DirList>& dir_list) {
  // Get a list of footprints
  std::string footprints_dir = get_absolute_directory_path(DIR_FOOTPRINTS);
  if (footprints_dir.empty()) {
    std::cerr << "Directory type error." << std::endl;
    exit(1);
  }

  for (const auto& dir : fs::directory_iterator{footprints_dir}) {
    // Get footprint libraries (.pretty directories)
    auto footprint_dir = fs::path(dir);
    if (footprint_dir.extension() != ".pretty") {
      continue;
    }

    // Get footprint names (unlike symbols we don't need to search the file as
    // these are filenames)
    for (const auto& fp_dir : fs::directory_iterator{footprint_dir}) {
      auto lbr_dir = fs::path(fp_dir);
      if (lbr_dir.extension() != ".kicad_mod") {
        continue;
      }

      for (auto& dir_item : dir_list) {
        if (dir_item.library == footprint_dir.stem().string()) {
          // Add footprint to library tracker
          dir_item.footprint.push_back(lbr_dir.stem().string());
        }
      }
    }
  }
}

/**
 * @brief Append 3d-model names to list of directories for displaying to user.
 * @param dir_list DirList structure.
 */
void Kandle::DirectoryStructure::list_3d_models(
    std::vector<DirList>& dir_list) {
  // Get a list of 3D models
  std::string models_dir = get_absolute_directory_path(DIR_3D_MODELS);
  if (models_dir.empty()) {
    std::cerr << "Directory type error." << std::endl;
    exit(1);
  }

  for (const auto& dir : fs::directory_iterator{models_dir}) {
    auto model_dir = fs::path(dir);
    if (!is_directory(model_dir)) {
      continue;
    }

    for (const auto& model : fs::directory_iterator{model_dir}) {
      auto model_path = fs::path(model);
      if (model_path.extension() != ".step") {
        continue;
      }

      for (auto& dir_item : dir_list) {
        if (dir_item.library == model_dir.stem().string()) {
          // Add footprint to library tracker
          dir_item.model_3d.push_back(model_path.stem().string());
        }
      }
    }
  }
}

/**
 * @brief Make a table displaying which parts have symbols, footprints and 3d
 * models.
 *
 * Outputs a table like this:
 * @verbatim
 * +-------------+---------------------+------------+------------+------------+
 * |   Library   |        Part         |   Symbol   |  Footprint |  3D-Model  |
 * +-------------+---------------------+------------+------------+------------+
 * | connectors  | PJ-002A             |     Yes    |     Yes    |     Yes    |
 * +-------------+---------------------+------------+------------+------------+
 * | connectors  | TBP02R2-381-04BE    |     Yes    |     Yes    |     Yes    |
 * +-------------+---------------------+------------+------------+------------+
 * | connectors  | TBP02R2-381-03BE    |     Yes    |     Yes    |     Yes    |
 * +-------------+---------------------+------------+------------+------------+
 * | connectors  | TBP02R2-381-02BE    |     Yes    |     Yes    |     Yes    |
 * +-------------+---------------------+------------+------------+------------+
 * @endverbatim
 * @param dir_list List ot libraries and their symbols, footprints and 3d
 * models.
 */
tabulate::Table Kandle::DirectoryStructure::create_table(
    std::vector<DirList>& dir_list) {
  tabulate::Table table;
  table.add_row({"Library", "Part", "Symbol", "Footprint", "3D-Model"});
  for (size_t i = 0; i < 5; i++) {
    table[0][i]
        .format()
        .font_align(tabulate::FontAlign::center)
        .font_style({tabulate::FontStyle::bold})
        .font_color(tabulate::Color::yellow);
  }

  int row = 0;
  for (const auto& dir_item : dir_list) {
    std::string library;
    std::string part;
    std::string symbol = "-";
    std::string footprint = "-";
    std::string model_3d = "-";

    library = dir_item.library;
    for (const auto& sym : dir_item.symbol) {
      part = sym;
      symbol = "Yes";
      for (const auto& fp : dir_item.footprint) {
        if (fp == sym) {
          footprint = "Yes";
          break;
        }
      }
      for (const auto& mod : dir_item.model_3d) {
        if (mod == sym) {
          model_3d = "Yes";
          break;
        }
      }
      table.add_row({library, part, symbol, footprint, model_3d});
      row++;
      table[row][0].format().font_style({tabulate::FontStyle::bold});
    }
  }

  // Formatting
  table.column(2).format().font_align(tabulate::FontAlign::center).width(12);
  table.column(3).format().font_align(tabulate::FontAlign::center).width(12);
  table.column(4).format().font_align(tabulate::FontAlign::center).width(12);

  return table;
}
