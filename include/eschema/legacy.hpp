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
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdio>

#include "eschema/component.hpp"
#include "eschema/config.hpp"
#include "utils.hpp"

class Legacy {
public:
    Component::Definition def;
    std::vector<Component::Information> info;
    std::vector<Component::Pin> pins;
    std::vector<Component::Rectangle> rectangles;
    std::vector<Component::Polygon> polygons;
    std::vector<Component::Circle> circles;
    std::vector<Component::Arc> arcs;
    std::vector<Component::Text> texts;

    bool convert(const std::vector<std::string>& lines);

    bool identify(const std::string& token, const std::string& line);

    bool parse_definition(const std::string& line);

    bool parse_information(const std::string& line);

    bool parse_pin(const std::string& line);

    bool parse_rectangle(const std::string& line);

    bool parse_polygon(const std::string& line);

    bool parse_circle(const std::string& line);

    bool parse_arc(const std::string& line);

    bool parse_text(const std::string& line);

private:

};


