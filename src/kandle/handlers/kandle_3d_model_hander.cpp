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

#include "kandle/handlers/kandle_3d_model_hander.h"

namespace fs = std::filesystem;

std::string Kandle::Model3DHander::get_path(const std::string& library_name) {
  return DirectoryStructure::get_directory_path(
             DirectoryStructure::DIR_3D_MODELS) +
         "/" + library_name;
}

bool Kandle::Model3DHander::add_to_project(const std::string& tmp_file_path,
                                           const std::string& library_name) {
  std::string component_library_path;
  std::string component_path;
  std::string component_name;

  // No 3dmodel library found
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

  // Formatted component name
  component_name = fs::path(tmp_file_path).parent_path().filename();

  // components/extern/3dmodels/<library_name>/<part_name>.ext
  // Handle both .stp and .step
  component_path += component_library_path + "/";
  component_path += component_name;
  component_path += fs::path(tmp_file_path).extension();

  // Copy directly from tmp folder to `component_path`
  Utils::copy_files_binary(tmp_file_path, component_path);

  return true;
}

bool Kandle::Model3DHander::remove_from_project(const std::string& library_name,
                                                const std::string& part_name) {
  // Delete 3D model
  std::string model_path =
      Kandle::DirectoryStructure::get_absolute_directory_path(
          DirectoryStructure::DIR_3D_MODELS);

  if (model_path.empty()) {
    return false;
  }

  std::string model_file_path =
      model_path + "/" + library_name + "/" + part_name + ".step";

  if (!fs::exists(model_file_path)) {
    return false;
  }

  return (std::remove(model_file_path.c_str()) == 0);
}