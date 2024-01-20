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

#ifndef KANDLE_KANDLE_FILE_HANDLER_H
#define KANDLE_KANDLE_FILE_HANDLER_H

#include <filesystem>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "eschema/legacy.hpp"
#include "eschema/release.hpp"
#include "kandle/kandle_directory.h"
#include "kandle_3d_model_hander.h"
#include "kandle_footprint_handler.h"
#include "kandle_symbol_handler.h"
#include "kandle_utils.hpp"

namespace Kandle {
class FileHandler {
  static bool validate_zip_file(const std::string& path);

  static void build_library_paths(const std::string& library_name);

  static void format_filename(std::string& filename);

 public:
  struct FilePaths {
    std::string symbol;
    std::string footprint;
    std::string dmodel;
  };

  static std::string unzip(const std::string& path);

  static FilePaths get_tmp_file_paths(const std::string& library_name);

  static bool delete_component(const std::string& library_name,
                               const std::string& part_name);
  static bool confirm_deletion(const std::string& library_name,
                               const std::string& part_name);

  static bool store_zip_in_global_directory(const std::string& zip_path,
                                            std::string kandle_path);
};
}  // namespace Kandle

#endif  // KANDLE_KANDLE_FILE_HANDLER_H
