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
#ifndef KANDLE_KANDLE_DIRECTORY_H
#define KANDLE_KANDLE_DIRECTORY_H

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <vector>

#include "kandle_utils.hpp"
#include "tabulate.hpp"

namespace Kandle {
class DirectoryStructure {
 public:
  enum Directories {
    DIR_COMPONENTS,
    DIR_COMPONENTS_EXTERN,
    DIR_SYMBOLS,
    DIR_FOOTPRINTS,
    DIR_3D_MODELS,
    DIR_TEMPORARY
  };

 private:
  struct DirList {
    std::string library;
    std::vector<std::string> symbol;
    std::vector<std::string> footprint;
    std::vector<std::string> model_3d;
  };

  static tabulate::Table create_table(std::vector<DirList>& dir_list);
  static bool create_directory(const std::string& absolute_path);
  static int list_symbols(std::vector<DirList>& dir_list);
  static void list_footprints(std::vector<DirList>& dir_list);
  static void list_3d_models(std::vector<DirList>& dir_list);

 public:
  static std::string get_directory_path(Directories dir);
  static std::string get_absolute_directory_path(Directories dir);
  static bool validate_working_directory();
  static bool initialise();
  static void list();
};
}  // namespace Kandle

#endif  // KANDLE_KANDLE_DIRECTORY_H
