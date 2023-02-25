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

#include "eschema/legacy.hpp"

bool Legacy::convert(const std::vector<std::string> &lines) {

    bool found_definition = false;

    std::cout << "Converting legacy file." << std::endl;

    // Iterate through lines
    for (const auto &l: lines) {
        std::stringstream strstr(l);
        std::istream_iterator<std::string> it(strstr);
        std::istream_iterator<std::string> end;
        std::vector<std::string> tokens(it, end);

        // Tokenize the line into its space delimited components
        if (!found_definition) {
            for (const auto &t: tokens) {
                if (t == "DEF") {
                    parse_definition(l);
                    found_definition = true;
                    break;
                }
            }
        } else {
            if (!identify(tokens.front(), l)) {
                std::cout << "Parse error" << std::endl;
            }
        }
    }

    return false;
}

bool Legacy::identify(
        const std::string &token,
        const std::string &line) {

    // Trying not to parse any tokens that are user input
    if (token.length() > 2) {
        return true;
    }

    bool res;

    // First token indicates what the line information
    // entails, just need the first letter as they are
    // all unique.
    switch (token[0]) {
        case (int) V5Identifiers::information:
            res = parse_information(line);
            break;
        case (int) V5Identifiers::pin:
            res = parse_pin(line);
            break;
        case (int) V5Identifiers::rectangle:
            res = parse_rectangle(line);
            break;
        case (int) V5Identifiers::polygon:
            res = parse_polygon(line);
            break;
        case (int) V5Identifiers::circle:
            res = parse_circle(line);
            break;
        case (int) V5Identifiers::arc:
            res = parse_arc(line);
            break;
        case (int) V5Identifiers::text:
            res = parse_text(line);
            break;
        default:
            std::cout <<
                      "Unknown .lib identifier: " <<
                      token << std::endl;
            return false;
    }

    return res;
}

bool Legacy::parse_definition(const std::string &line) {

    int res = std::sscanf(line.c_str(), "DEF %50s %25s 0 %d %c %c %d %*c %c",
                          def.name, def.reference, &def.pin_name_offset,
                          &def.show_pin_number, &def.show_pin_name,
                          &def.num_units, &def.pwr_cmp);

    if (res != 7) {
        return false;
    }

    return true;
}

bool Legacy::parse_information(const std::string &line) {
    Component::Information ci;

    int res = std::sscanf(line.c_str(),
                          R"(F%*c "%50[^"]" %d %d %d %c %c %c %c%c%c "%50[^"]")",
                          ci.text, &ci.pos_x, &ci.pos_y, &ci.font_size,
                          &ci.orientation,
                          &ci.visibility, &ci.horz_justification,
                          &ci.vert_justification,
                          &ci.italic, &ci.bold, ci.field_name);

    // Expect at least 10 matches (11 if field name is present)
    if (res < 10) {
        return false;
    }

    info.push_back(ci);

    return true;
}

// Examples:
//
// X TO 1 - 200 0.150 R 40 40 1 1 P
// X 0 1 0 0 0 R 40 40 1 1 W NC
bool Legacy::parse_pin(const std::string &line) {
    Component::Pin pin;

    int res = std::sscanf(line.c_str(),
                          "X %50s %4s %d %d %d %c %d %d %d %d %c %3s",
                          pin.name, pin.number, &pin.pos_x, &pin.pos_y,
                          &pin.length,
                          &pin.orientation, &pin.text_num_size,
                          &pin.text_name_size,
                          &pin.unit, &pin.convert, &pin.electric_type,
                          pin.shape);

    if (res < 10) {
        if (strlen(pin.name) != 0) {
            return false;
        }
    }

    pins.push_back(pin);

    return true;
}

// Example:
// S 0 50.900.900 0 1 0 f
bool Legacy::parse_rectangle(const std::string &line) {

    Component::Rectangle rect{};

    int res = std::sscanf(line.c_str(), "S %d %d %d %d %d %d %d %c",
                          &rect.startx, &rect.starty, &rect.endx, &rect.endy,
                          &rect.unit, &rect.convert,
                          &rect.thickness, &rect.background);

    if (res != 8) {
        return false;
    }

    rectangles.push_back(rect);

    return true;
}

//Example:
//
// P 3 0 1 0 -50 50 50 0 -50 -50 F
// P 2 0 1 0 50 50 50 –50 N
//
bool Legacy::parse_polygon(const std::string &line) {
    Component::Polygon polygon{};

    int res = std::sscanf(line.c_str(), "P %d %d %d %d %d %d %d %d %d %d %c",
                          &polygon.n_points, &polygon.parts, &polygon.convert,
                          &polygon.thickness, &polygon.x0, &polygon.y0,
                          &polygon.x1,
                          &polygon.y1, &polygon.xi, &polygon.yi,
                          &polygon.background);

    if (res != 11) {
        // Retry without xi and yi
        res = std::sscanf(line.c_str(), "P %d %d %d %d %d %d %d %d %c",
                          &polygon.n_points, &polygon.parts, &polygon.convert,
                          &polygon.thickness,
                          &polygon.x0, &polygon.y0, &polygon.x1, &polygon.y1,
                          &polygon.background);

        polygon.xi = 0;
        polygon.yi = 0;

        if (res != 9) {
            return false;
        }
    }

    polygons.push_back(polygon);

    return true;
}

bool Legacy::parse_circle(const std::string &line) {
    Component::Circle circle{};

    int res = sscanf(line.c_str(), "C %d %d %d %d %d %d %c",
                     &circle.posx, &circle.posy, &circle.radius, &circle.unit,
                     &circle.convert,
                     &circle.thickness, &circle.background);

    if (res != 7) {
        return false;
    }

    circles.push_back(circle);

    return true;
}


// Example:
//
// A -1 -200 49 900 -11 0 1 0 N -50 -200 0 -150 
bool Legacy::parse_arc(const std::string &line) {
    Component::Arc arc{};

    int res = sscanf(line.c_str(),
                     "A %d %d %d %d %d %d %d %d %c %d %d %d %d",
                     &arc.posx, &arc.posy, &arc.radius, &arc.start_angle,
                     &arc.end_angle, &arc.part, &arc.convert, &arc.thickness,
                     &arc.background, &arc.start_point_x, &arc.start_point_y,
                     &arc.end_point_x, &arc.end_point_y);

    if (res != 13) {
        return false;
    }

    arcs.push_back(arc);

    return true;
}

bool Legacy::parse_text(const std::string &line) {
    Component::Text t{};

    int res = sscanf(line.c_str(),
                     "T %d %d %d %d %d %d %50s",
                     &t.orientation, &t.posx, &t.posy, &t.dimension,
                     &t.unit, &t.convert, t.text);

    if (res != 7) {
        return false;
    }

    texts.push_back(t);

    return true;
}

