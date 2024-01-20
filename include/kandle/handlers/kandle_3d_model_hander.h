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

#ifndef KANDLE_MODEL_3D_H
#define KANDLE_MODEL_3D_H

#include <filesystem>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

#include "kandle/kandle_directory.h"
#include "kandle_utils.hpp"

namespace Kandle {
class Model3DHander {
  static std::string get_path(const std::string& library_name);

 public:
  static bool add_to_project(const std::string& tmp_file_path,
                             const std::string& library_name);
  static bool remove_from_project(const std::string& library_name,
                                  const std::string& part_name);
};
}  // namespace Kandle

#endif  // KANDLE_MODEL_3D_H
