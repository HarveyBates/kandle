// MIT License
//
// Copyright (c) 2023 Harvey Bates
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "eschema/release.hpp"

bool Symbol::new_from_legacy(Legacy *legacy_component,
                             const std::string &filename) {
    output_filename = filename;

    // Each of these methods write to a file (filename)
    if (!build_header()) {
        std::cout << "Error building symbol header" << std::endl;
        return false;
    }
    if (!build_symbol(legacy_component)) {
        std::cout << "Error building symbol" << std::endl;
        return false;
    }
    if (!build_properties(legacy_component)) {
        std::cout << "Error building properties" << std::endl;
        return false;
    }
    if (!build_graphics(legacy_component)) {
        std::cout << "Error building graphics" << std::endl;
        return false;
    }

    if (!write_to_file("  )\n)")) {
        return false;
    }

    return true;
}

bool Symbol::write_to_file(const char *contents) {

    std::fstream symbolFile(output_filename,
                            std::fstream::out | std::fstream::app);

    if (symbolFile.is_open()) {
        symbolFile << contents << "\n";
        symbolFile.close();
    } else {
        return false;
    }

    return true;
}

/**
 * @note The header brackets enclose the entire symbol definition. This means
 * the last brackets must be added by another function.
 *
 * @example
 * (kicad_symbol_lib
 *   (version VERSION)
 *   (generator GENERATOR)
 *   -- Contents of symbol library --
 * )
 */
bool Symbol::build_header() {
    memset(buffer, 0, sizeof(buffer));

    // Note, closing bracket is added at a later stage
    snprintf(buffer, sizeof(buffer), "(kicad_symbol_lib "
                                     "(version %s) (generator %s)",
             KICAD_VERSION, KICAD_GENERATOR);

    return write_to_file(buffer);
}


/**
 * (symbol
 *   "LIBRARY_ID" | "UNIT_ID"
 *   [(extends "LIBRARY_ID")]
 *   [(pin_numbers hide)]
 *   [(pin_names [(offset OFFSET)] hide)]
 *   (in_bom yes | no)
 *   (on_board yes | no)
 *   SYMBOL_PROPERTIES...
 *   GRAPHIC_ITEMS...
 *   PINS...
 *   UNITS...
 *   [(unit_name "UNIT_NAME")]
 * )
 */
bool Symbol::build_symbol(Legacy *legacy_component) {
    char pin_buf[AUX_BUF_SIZE]{};
    memset(buffer, 0, sizeof(buffer));

    memcpy(pin_buf, build_pins_definition(legacy_component), sizeof(pin_buf));

    snprintf(buffer, sizeof(buffer),
             "  (symbol \"%s\" %s (in_bom yes) (on_board yes)",
             legacy_component->def.name, pin_buf);

    return write_to_file(buffer);
}

/**
 * @example
 *   [(pin_numbers hide)]
 *   [(pin_names [(offset OFFSET)] hide)]
 *
 * @param legacy_component
 * @return
 */
const char *Symbol::build_pins_definition(Legacy *legacy_component) {
    char buf[40]{};

    memset(aux_buffer, 0, sizeof(aux_buffer));

    if (!Utils::assert_true(legacy_component->def.show_pin_number)) {
        strncat(aux_buffer, "(pin_numbers hide) ",
                sizeof(aux_buffer) - strlen(aux_buffer) - 1);
    }

    double offset = Utils::mils_to_millimeters(
            legacy_component->def.pin_name_offset);

    if (Utils::assert_true(legacy_component->def.show_pin_name)) {
        snprintf(buf, sizeof(buf), "(pin_names (offset %.3f))", offset);
        strncat(aux_buffer, buf, sizeof(aux_buffer) - strlen(aux_buffer) - 1);
    } else {
        snprintf(buf, sizeof(buf), "(pin_names (offset %.3f) hide)", offset);
        strncat(aux_buffer, buf, sizeof(aux_buffer) - strlen(aux_buffer) - 1);
    }

    return aux_buffer;
}


/**
 * @example
 * (property
 *   "KEY"
 *   "VALUE"
 *   (id N)
 *   POSITION_IDENTIFIER
 *   TEXT_EFFECTS
 * )
 *
 * @param legacy_component
 * @return
 */
bool Symbol::build_properties(Legacy *legacy_component) {
    double pos_x, pos_y;
    char font_buf[AUX_BUF_SIZE];
    char justify_buf[AUX_BUF_SIZE];
    const char *key;

    // Inbuilt keys (in order)
    const int N_INBUILT_KEYS = 4;
    const char *keys[N_INBUILT_KEYS] = {"Reference", "Value", "Footprint",
                                        "Datasheet"};

    int i = 0;
    for (const auto &info: legacy_component->info) {
        memset(buffer, 0, sizeof(buffer));

        // Assign the key from either the 4 inbuilt keys or a special key
        if (i < N_INBUILT_KEYS) {
            key = keys[i];
        } else {
            key = info.field_name;
        }

        pos_x = Utils::mils_to_millimeters(info.pos_x);
        pos_y = Utils::mils_to_millimeters(info.pos_y);

        // Get font information
        memcpy(font_buf, build_font(info.font_size,
                                    info.bold, info.italic), sizeof(font_buf));
        // Get font justification
        memcpy(justify_buf, build_text_justification(&info),
               sizeof(justify_buf));

        snprintf(buffer, sizeof(buffer),
                 "    (property \"%s\" \"%s\" (id %d) (at %.2f %.2f 0)\n"
                 "      (effects %s %s",
                 key, info.text, i, pos_x, pos_y, font_buf, justify_buf);

        if (info.visibility == 'V') {
            strncat(buffer, ")\n    )", sizeof(buffer) - strlen(buffer) - 1);
        } else {
            strncat(buffer, " hide)\n    )",
                    sizeof(buffer) - strlen(buffer) - 1);
        }

        if (!write_to_file(buffer)) {
            return false;
        }

        i++;
    }

    return true;
}

/**
 * @brief Builds font information for use in the symbol properties.
 *
 * @example
 *  (font
 *    [(face FACE_NAME)]
 *    (size HEIGHT WIDTH)
 *    [(thickness THICKNESS)]
 *    [bold]
 *    [italic]
 *    [(line_spacing LINE_SPACING)]
 *  )
 *
 * @param info
 * @return
 */
const char *Symbol::build_font(const int font_size,
                               const char bold,
                               const char italic) {
    double font_size_f = Utils::mils_to_millimeters(font_size);

    memset(aux_buffer, 0, sizeof(aux_buffer));

    snprintf(aux_buffer, sizeof(aux_buffer), "(font (size %.3f %.3f)",
             font_size_f, font_size_f);

    if (bold == 'B') {
        strncat(aux_buffer, " bold",
                sizeof(aux_buffer) - strlen(aux_buffer) - 1);
    }

    if (italic == 'I') {
        strncat(aux_buffer, " italic",
                sizeof(aux_buffer) - strlen(aux_buffer) - 1);
    }

    strncat(aux_buffer, ")", sizeof(aux_buffer) - strlen(aux_buffer) - 1);

    return aux_buffer;
}

/**
 * @brief Builds justification information for a symbol.
 *
 * @example
 * [(justify [left | right] [top | bottom] [mirror])]
 *
 * @param info
 * @return
 */
const char *Symbol::build_text_justification(
        const Component::Information *info) {
    memset(aux_buffer, 0, sizeof(aux_buffer));

    if (info->horz_justification != 'C') {
        strncat(aux_buffer, "(justify ",
                sizeof(aux_buffer) - strlen(aux_buffer) - 1);
        add_justification(info->horz_justification);
        strncat(aux_buffer, " ", sizeof(aux_buffer) - strlen(aux_buffer) - 1);
        add_justification(info->vert_justification);
        strncat(aux_buffer, ")", sizeof(aux_buffer) - strlen(aux_buffer) - 1);
    }

    return aux_buffer;
}

/**
 * @brief Used to get the horizontal or vertical justification of text.
 * @warning This appends to the aux buffer in the background.
 *
 * @param identifier Character describing horizontal or vertical justification.
 */
void Symbol::add_justification(char identifier) {
    switch (identifier) {
        case 'L':
            strncat(aux_buffer, "left",
                    sizeof(aux_buffer) - strlen(aux_buffer) - 1);
            break;
        case 'R':
            strncat(aux_buffer, "right",
                    sizeof(aux_buffer) - strlen(aux_buffer) - 1);
            break;
        case 'B':
            strncat(aux_buffer, "bottom",
                    sizeof(aux_buffer) - strlen(aux_buffer) - 1);
            break;
        case 'T':
            strncat(aux_buffer, "top",
                    sizeof(aux_buffer) - strlen(aux_buffer) - 1);
            break;
        default:
            return;
    }
}


bool Symbol::build_graphics(Legacy *legacy_component) {
    memset(buffer, 0, sizeof(buffer));

    snprintf(buffer, sizeof(buffer), "    (symbol \"%s_0_0\"",
             legacy_component->def.name);

    // Start of graphics section
    if (!write_to_file(buffer)) {
        return false;
    }

    // Polygons
    if (!legacy_component->polygons.empty()) {
        if (!build_polygons(legacy_component->polygons)) {
            return false;
        }
    }

    // Circles
    if (!legacy_component->circles.empty()) {
        if (!build_circles(legacy_component->circles)) {
            return false;
        }
    }

    // Arcs
    if (!legacy_component->arcs.empty()) {
        if (!build_arcs(legacy_component->arcs)) {
            return false;
        }
    }

    // Rectangles
    if (!legacy_component->rectangles.empty()) {
        if (!build_rectangles(legacy_component->rectangles)) {
            return false;
        }
    }

    // Pins
    if (!build_pins(legacy_component)) {
        std::cout << "Error building pins" << std::endl;
        return false;
    }

    // Closing bracket - end of graphics section
    if (!write_to_file("    )")) {
        return false;
    }

    return true;
}

/**
 * @brief Adds polygons from legacy component into a .kicad_sym file.
 *
 * @example
 * (polyline
 *   COORDINATE_POINT_LIST
 *   STROKE_DEFINITION
 *   FILL_DEFINITION
 * )
 *
 * @param polygons
 * @return
 */
bool Symbol::build_polygons(const std::vector<Component::Polygon> &polygons) {
    double x0, y0, x1, y1;
    double stroke_width;
    std::string fill;

    for (const auto &polygon: polygons) {
        memset(aux_buffer, 0, sizeof(aux_buffer));

        x0 = Utils::mils_to_millimeters(polygon.x0);
        y0 = Utils::mils_to_millimeters(polygon.y0);
        x1 = Utils::mils_to_millimeters(polygon.x1);
        y1 = Utils::mils_to_millimeters(polygon.y1);
        stroke_width = Utils::mils_to_millimeters(polygon.thickness);

        switch (polygon.background) {
            case 'F':
            case 'f':
                fill = "background"; // Filled
                break;
            case 'N':
            default:
                fill = "none"; // Transparent
                break;
        }

        snprintf(aux_buffer,
                 sizeof(aux_buffer),
                 "      (polyline\n"
                 "        (pts\n"
                 "          (xy %.3f %.3f)\n"
                 "          (xy %.3f %.3f)\n"
                 "        )\n"
                 "        (stroke (width %.3f) (type default) (color 0 0 0 0))\n"
                 "        (fill (type %s))\n"
                 "      )",
                 x0, y0, x1, y1, stroke_width, fill.c_str());

        // Write polygon to file
        if (!write_to_file(aux_buffer)) {
            return false;
        }
    }

    return true;
}

bool Symbol::build_pins(const Legacy *legacy_component) {
    double pos_x, pos_y;
    std::string pin_type;
    int orientation;
    double length;
    PinShape pin_shape;
    char font_name_buf[128];
    char font_num_buf[128];

    for (const auto &pin: legacy_component->pins) {
        memset(buffer, 0, sizeof(buffer));
        memset(font_name_buf, 0, sizeof(font_num_buf));
        memset(font_num_buf, 0, sizeof(font_num_buf));

        pin_type = get_pin_type(pin.electric_type);
        pin_shape = get_pin_shape(pin.shape);

        pos_x = Utils::mils_to_millimeters(pin.pos_x);
        pos_y = Utils::mils_to_millimeters(pin.pos_y);
        orientation = get_pin_orientation(pin.orientation);
        length = Utils::mils_to_millimeters(pin.length);

        memcpy(font_name_buf, build_font(pin.text_name_size),
               sizeof(font_name_buf));

        memcpy(font_num_buf, build_font(pin.text_num_size),
               sizeof(font_num_buf));

        snprintf(buffer, sizeof(buffer),
                 "      (pin %s %s (at %.3f %.3f %d) (length %.3f)\n"
                 "        (name \"%s\" (effects %s))\n"
                 "        (number \"%s\" (effects %s))\n"
                 "      )",
                 pin_type.c_str(), pin_shape.shape.c_str(),
                 pos_x, pos_y, orientation, length, pin.name,
                 font_name_buf, pin.number, font_num_buf);

        if (!write_to_file(buffer)) {
            return false;
        }
    }


    return true;
}

std::string Symbol::get_pin_type(const char identifier) {
    std::string pin_type;

    switch (identifier) {
        case Component::PinElectricalType::INPUT:
            pin_type = "input";
            break;
        case Component::PinElectricalType::OUTPUT:
            pin_type = "output";
            break;
        case Component::PinElectricalType::BIDIRECTIONAL:
            pin_type = "bidirectional";
            break;
        case Component::PinElectricalType::TRI_STATE:
            pin_type = "tri_state";
            break;
        case Component::PinElectricalType::PASSIVE:
            pin_type = "passive";
            break;
        case Component::PinElectricalType::POWER_INPUT:
            pin_type = "power_in";
            break;
        case Component::PinElectricalType::POWER_OUTPUT:
            pin_type = "power_out";
            break;
        case Component::PinElectricalType::OPEN_COLLECTOR:
            pin_type = "open_collector";
            break;
        case Component::PinElectricalType::OPEN_EMITTER:
            pin_type = "open_emitter";
            break;
        case Component::PinElectricalType::NOT_CONNECTED:
            pin_type = "free";
            break;
        case Component::PinElectricalType::UNSPECIFIED:
        default:
            pin_type = "unspecified";
            break;
    }

    return pin_type;
}

Symbol::PinShape Symbol::get_pin_shape(const char *shape_buf) {
    PinShape pin_shape;

    // Pin shape not found
    if (strlen(shape_buf) == 0) {
        pin_shape.shape = "line"; // Default is line
        return pin_shape;
    }

    int offset = 0;
    if (shape_buf[0] == 'N') {
        offset = 1;
        pin_shape.visible = false;
        pin_shape.identifier = shape_buf[offset++];
    } else {
        pin_shape.visible = true;
        pin_shape.identifier = shape_buf[offset++];
    }

    switch (pin_shape.identifier) {
        case 'I': // Inverted
            pin_shape.shape = "inverted";
            break;
        case 'C': // Clock
            if (shape_buf[offset] == 'I') { // CI
                pin_shape.shape = "inverted_clock";
            } else if (shape_buf[offset] == 'L') { // CL
                pin_shape.shape = "clock_low";
            } else { // C
                pin_shape.shape = "clock";
            }
            break;
        case 'L':
            pin_shape.shape = "input_low";
            break;
        case 'V':
            pin_shape.shape = "output_low";
            break;
        case 'F':
            pin_shape.shape = "edge_clock_high";
            break;
        case 'X':
            pin_shape.shape = "non_logic";
            break;
        default:
            pin_shape.shape = "line";
            break;
    }

    return pin_shape;
}

int Symbol::get_pin_orientation(const char identifier) {
    int angle;

    switch (identifier) {
        case 'D':
            angle = 270;
            break;
        case 'R':
            angle = 0;
            break;
        case 'L':
            angle = 180;
            break;
        case 'U':
        default:
            angle = 90;
    }

    return angle;
}

bool Symbol::build_circles(const std::vector<Component::Circle> &circles) {
    double pos_x, pos_y;
    double radius;
    double stroke_width;
    std::string fill;

    for (const auto &circle: circles) {
        memset(buffer, 0, sizeof(buffer));

        pos_x = Utils::mils_to_millimeters(circle.posx);
        pos_y = Utils::mils_to_millimeters(circle.posy);
        radius = Utils::mils_to_millimeters(circle.radius);
        stroke_width = Utils::mils_to_millimeters(circle.thickness);

        switch (circle.background) {
            case 'F':
            case 'f':
                fill = "background"; // Filled
                break;
            case 'N':
            default:
                fill = "none"; // Transparent
                break;
        }

        snprintf(buffer,
                 sizeof(buffer),
                 "      (circle\n"
                 "        (center %.3f %.3f)\n"
                 "        (radius %.3f)\n"
                 "        (stroke (width %.3f) (type default) (color 0 0 0 0))"
                 " (fill (type %s))\n"
                 "      )", pos_x, pos_y, radius, stroke_width, fill.c_str());

        if (!write_to_file(buffer)) {
            return false;
        }
    }

    return true;
}

bool Symbol::build_arcs(const std::vector<Component::Arc> &arcs) {
    double start_x, start_y;
    double mid_x, mid_y;
    double end_x, end_y;
    double stroke_width;
    std::string fill;

    for (const auto &arc: arcs) {
        memset(buffer, 0, sizeof(buffer));

        start_x = Utils::mils_to_millimeters(arc.start_point_x);
        start_y = Utils::mils_to_millimeters(arc.start_point_y);
        mid_x = Utils::mils_to_millimeters(arc.posx);
        mid_y = Utils::mils_to_millimeters(arc.posy);
        end_x = Utils::mils_to_millimeters(arc.end_point_x);
        end_y = Utils::mils_to_millimeters(arc.end_point_y);

        stroke_width = Utils::mils_to_millimeters(arc.thickness);

        switch (arc.background) {
            case 'F':
            case 'f':
                fill = "background"; // Filled
                break;
            case 'N':
            default:
                fill = "none"; // Transparent
                break;
        }

        snprintf(buffer,
                 sizeof(buffer),
                 "      (arc (start %.3f %.3f) (mid %.3f %.3f) (end %.3f %.3f)\n"
                 "        (stroke (width %.3f) (type default))\n"
                 "        (fill (type %s))\n"
                 "      )",
                 start_x, start_y, mid_x, mid_y, end_x, end_y,
                 stroke_width, fill.c_str());

        if (!write_to_file(buffer)) {
            return false;
        }
    }

    return true;
}

bool Symbol::build_rectangles(
        const std::vector<Component::Rectangle> &rectangles) {
    double start_x, start_y;
    double end_x, end_y;
    double stroke_width;
    std::string fill;

    for (const auto &rectangle: rectangles) {
        memset(buffer, 0, sizeof(buffer));

        start_x = Utils::mils_to_millimeters(rectangle.startx);
        start_y = Utils::mils_to_millimeters(rectangle.starty);
        end_x = Utils::mils_to_millimeters(rectangle.endx);
        end_y = Utils::mils_to_millimeters(rectangle.endy);

        stroke_width = Utils::mils_to_millimeters(rectangle.thickness);

        switch (rectangle.background) {
            case 'F':
            case 'f':
                fill = "background"; // Filled
                break;
            case 'N':
            default:
                fill = "none"; // Transparent
                break;
        }

        snprintf(buffer,
                 sizeof(buffer),
                 "      (rectangle (start %.3f %.3f) (end %.3f %.3f)\n"
                 "        (stroke (width %.3f) (type default))"
                 " (fill (type %s))\n"
                 "      )",
                 start_x, start_y, end_x, end_y, stroke_width, fill.c_str());

        if (!write_to_file(buffer)) {
            return false;
        }
    }

    return true;
}

bool Symbol::build_text_fields(
        const std::vector<Component::Text> &text_fields) {
    double pos_x, pos_y;
    double rotation;
    char text_effects[128];
    std::string fill;

    for (const auto &text_field: text_fields) {
        memset(buffer, 0, sizeof(buffer));
        memset(text_effects, 0, sizeof(text_effects));

        pos_x = Utils::mils_to_millimeters(text_field.posx);
        pos_y = Utils::mils_to_millimeters(text_field.posy);
        rotation = Utils::mils_to_millimeters(text_field.orientation);

        memcpy(text_effects, build_font(text_field.dimension),
               sizeof(text_effects));

        snprintf(buffer,
                 sizeof(buffer),
                 "      (text\n"
                 "        \"%s\"\n"
                 "        (at %.3f %.3f %.3f)\n"
                 "        (effects %s"
                 "      )",
                 text_field.text, pos_x, pos_y, rotation, text_effects);

        if (!write_to_file(buffer)) {
            return false;
        }
    }

    return true;
}
