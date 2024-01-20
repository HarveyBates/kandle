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

#pragma once

#include <cstring>
#include <fstream>
#include <iostream>

#include "eschema/component.hpp"
#include "eschema/legacy.hpp"
#include "kandle_utils.hpp"

class Symbol {
  char buffer[512];
  static const int AUX_BUF_SIZE = 512;
  char aux_buffer[AUX_BUF_SIZE];
  std::string output_filename;

  struct PinShape {
    bool visible;
    std::string shape;
    char identifier;
  };

 public:
  bool new_from_legacy(Legacy* legacy_component, const std::string& filename);

 private:
  bool write_to_file(const char* contents);

  bool build_header();

  bool build_symbol(Legacy* legacy_component);

  const char* build_pins_definition(Legacy* legacy_component);

  bool build_properties(Legacy* legacy_component);

  const char* build_font(int font_size, char bold = 'N', char italic = 'N');

  const char* build_text_justification(const Component::Information* info);

  void add_justification(char identifier);

  bool build_graphics(Legacy* legacy_component);

  bool build_polygons(const std::vector<Component::Polygon>& polygons);

  static std::string build_polygon_points(
      const std::vector<std::tuple<int, int>>& coords);

  bool build_pins(const Legacy* legacy_component);

  static PinShape get_pin_shape(const char* shape_buf);

  static std::string get_pin_type(char identifier);

  static int get_pin_orientation(char identifier);

  bool build_circles(const std::vector<Component::Circle>& circles);

  bool build_arcs(const std::vector<Component::Arc>& arcs);

  bool build_rectangles(const std::vector<Component::Rectangle>& rectangles);

  bool build_text_fields(const std::vector<Component::Text>& text_fields);
};
