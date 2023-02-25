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

#include "eschema/release.hpp"

bool Symbol::new_from_legacy(Legacy* legacy_component,
                             const std::string& filename) {
    output_filename = filename;

    // Each of these methods write to the file (filename)
    if (!build_header()) {
        return false;
    }
    if (!build_symbol(legacy_component)) {
        return false;
    }
    if (!build_properties(legacy_component)) {
        return false;
    }

    return true;
}

bool Symbol::write_to_file(const char* contents) {

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
bool Symbol::build_symbol(Legacy* legacy_component) {
    char pin_buf[AUX_BUF_SIZE]{};
    memset(buffer, 0, sizeof(buffer));

    memcpy(pin_buf, build_pins(legacy_component), sizeof(pin_buf));

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
const char* Symbol::build_pins(Legacy* legacy_component) {
    char buf[40]{};

    memset(aux_buffer, 0, sizeof(aux_buffer));

    if (!Utils::assert_true(legacy_component->def.show_pin_number)) {
        strcat(aux_buffer, "(pin_numbers hide) ");
    }

    double offset = Utils::mils_to_millimeters(
            legacy_component->def.pin_name_offset);

    if (Utils::assert_true(legacy_component->def.show_pin_name)) {
        snprintf(buf, sizeof(buf), "(pin_names (offset %.3f) show)", offset);
        strcat(aux_buffer, buf);
    } else {
        snprintf(buf, sizeof(buf), "(pin_names (offset %.3f) hide)", offset);
        strcat(aux_buffer, buf);
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
bool Symbol::build_properties(Legacy* legacy_component) {
    double pos_x, pos_y;
    char font_buf[AUX_BUF_SIZE];
    char justify_buf[AUX_BUF_SIZE];
    const char* key;

    // Inbuilt keys (in order)
    const int N_INBUILT_KEYS = 4;
    const char* keys[N_INBUILT_KEYS] = {"Reference", "Value", "Footprint",
                                        "Datasheet"};

    int i = 0;
    for (const auto& info: legacy_component->info) {
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
        memcpy(font_buf, build_font(&info), sizeof(font_buf));
        // Get font justification
        memcpy(justify_buf, build_justification(&info), sizeof(justify_buf));

        snprintf(buffer, sizeof(buffer),
                 "    (property \"%s\" \"%s\" (id %d) (at %.2f %.2f 0)\n"
                 "      (effects %s %s",
                 key, info.text, i, pos_x, pos_y, font_buf, justify_buf);

        if (info.visibility == 'V') { strcat(buffer, ")\n    )"); }
        else { strcat(buffer, " hide)\n    )"); }

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
const char* Symbol::build_font(const Component::Information* info) {
    double font_size = Utils::mils_to_millimeters(info->font_size);

    memset(aux_buffer, 0, sizeof(aux_buffer));

    snprintf(aux_buffer, sizeof(aux_buffer), "(font (size %.2f %.2f)",
             font_size, font_size);

    if (info->bold == 'B') { strcat(aux_buffer, " bold"); }
    if (info->italic == 'I') { strcat(aux_buffer, " italic"); }
    strcat(aux_buffer, ")");

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
const char* Symbol::build_justification(const Component::Information* info) {
    memset(aux_buffer, 0, sizeof(aux_buffer));

    strcat(aux_buffer, "(justify ");
    add_justification(info->horz_justification);
    strcat(aux_buffer, " ");
    add_justification(info->vert_justification);
    strcat(aux_buffer, ")");

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
            strcat(aux_buffer, "left");
            break;
        case 'R':
            strcat(aux_buffer, "right");
            break;
        case 'C':
            strcat(aux_buffer, "centre");
            break;
        case 'B':
            strcat(aux_buffer, "bottom");
            break;
        case 'T':
            strcat(aux_buffer, "top");
            break;
        default:
            return;
    }
}

