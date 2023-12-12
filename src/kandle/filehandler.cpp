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

#include "kandle/filehandler.h"

namespace fs = std::filesystem;

std::string output_directory;
static Kandle::FileHandler::FilePaths library_file_paths;

std::string Kandle::FileHandler::unzip(const std::string& path) {

    validate_zip_file(path);

    std::cout << "Extracting from: " << path << std::endl;

    std::string output_path = "components/extern/tmp/";
    std::string filename = fs::path(path).stem();

    // Remove ultra-librarian prefix
    if (filename.substr(0, 3) == "ul_") {
      filename = filename.substr(3);
    }

    // Remove CSE prefix
    if (filename.substr(0, 4) == "LIB_") {
      filename = filename.substr(4);
    }

    // Replace spaces and - with _
    std::replace(filename.begin(), filename.end(), ' ', '_');
    std::replace(filename.begin(), filename.end(), '-', '_');

    output_path += filename; // Add new filename to path

    std::cout << "Extracting to: " << output_path << std::endl;

    if (fs::exists(output_path)) {
        std::cout << "Output directory: " << output_path << " already exists."
                  << std::endl;
        output_directory = output_path;
        return output_path;
    }

    std::ostringstream oss;
    oss << "unzip \"" << path << "\" -d \"" << output_path
        << "\" > /dev/null 2>&1";
    std::string cmd = oss.str();

    int err = std::system(cmd.c_str());
    if (err == 0) {
        std::cout << "Successfully extracted to: " << output_path << std::endl;
        output_directory = output_path;
        return output_path;
    }

    std::cerr << "Files could not be extracted from: " << path
              << std::endl;
    exit(1);
}

// Check that the input file path actually contains a .zip file
bool Kandle::FileHandler::validate_zip_file(const std::string& path) {

    if (!fs::is_regular_file(path)) {
        std::cerr << "Error unknown file: \"" << path << "\". Exiting."
                  << std::endl;
        exit(1);
    }

    auto zip_path = fs::path(path);

    if (zip_path.extension() == ".zip") {
        return true;
    }

    std::cerr << "Error unsupported filetype: \"" << path << "\". Exiting."
              << std::endl;
    exit(1);
}

void Kandle::FileHandler::build_library_paths(
        const std::string& library_name) {
    // File
    library_file_paths.symbol = "components/extern/symbols/";
    library_file_paths.symbol += library_name;
    library_file_paths.symbol += ".kicad_sym";

    // Directory
    library_file_paths.footprint = "components/extern/footprints/";
    library_file_paths.footprint += library_name;
    library_file_paths.footprint += ".pretty";

    // File
    library_file_paths.dmodel = "components/extern/3dmodels/";
    library_file_paths.dmodel += library_name;
}

Kandle::FileHandler::FilePaths Kandle::FileHandler::recursive_extract_paths(
        const std::string& library_name) {
    bool symbol_found = false;
    bool footprint_found = false;
    bool dmodel_found = false;

    FilePaths component_file_paths;
    build_library_paths(library_name);

    for (const auto& dir_item: fs::recursive_directory_iterator{
            output_directory}) {
        auto item = fs::path(dir_item);

        if (item.stem() == "KiCad") {
            std::cout << "Component Search Engine component detected."
                      << std::endl;
            symbol_found = false;
            footprint_found = false;
            dmodel_found = false;
        }

        if (!symbol_found && item.extension() == ".kicad_sym") {
            std::cout << "Found symbol: " << item << std::endl;
            component_file_paths.symbol = item;
            symbol_found = true;
        }

        if (!symbol_found && item.extension() == ".lib") {
            std::string symbol_path = item;
            component_file_paths.symbol = convert_symbol(symbol_path);
            symbol_found = true;
        }

        if (!footprint_found && item.extension() == ".kicad_mod") {
            std::cout << "Found footprint: " << item << std::endl;
            component_file_paths.footprint = item;
            footprint_found = true;
        }
        if (!dmodel_found && (item.extension() == ".stp" ||
                              item.extension() == ".step")) {
            std::cout << "Found 3D model: " << item << std::endl;
            component_file_paths.dmodel = item;
            dmodel_found = true;
        }
    }

    return component_file_paths;
}

std::string Kandle::FileHandler::convert_symbol(
        const std::string& legacy_symbol_path) {
    Legacy legacy;
    Symbol symbol;
    std::string filename;
    std::string converted_path;
    std::string new_symbol_path;

    converted_path = fs::path(legacy_symbol_path).parent_path();
    converted_path += "/";

    new_symbol_path = converted_path;
    new_symbol_path += fs::path(legacy_symbol_path).stem();
    new_symbol_path += ".kicad_sym";

    std::vector<std::string> lines = Utils::readlines(legacy_symbol_path);

    // Parse legacy file
    if (!legacy.convert(lines)) {
        std::cerr << "Error converting file. Submit an issue. Exiting."
                  << std::endl;
        exit(1);
    }

    // Covert legacy library to .kicad_sym in the same directory
    if (!symbol.new_from_legacy(&legacy, new_symbol_path)) {
        std::cerr << "Error converting file. Submit an issue. Exiting."
                  << std::endl;
        exit(1);
    }

    // Get and return the new converted file path
    for (const auto& dir_item: fs::directory_iterator{converted_path}) {
        auto item = fs::path(dir_item);
        if (item.extension() == ".kicad_sym") {
            return item;
        }
    }

    std::cerr << "Error converting file. Submit an issue. Exiting."
              << std::endl;

    exit(1);
}

/**
 * @brief Replaces the footprint entry for the symbol file with Kandle footprint
 * identifier.
 *
 * @example OP_AMP:LM358 (library:component) where OP_AMP exists as a directory
 * inside footprints called "OP_AMP.pretty" and LM358 exists as a file called
 * "LM358.kicad_mod".
 *
 * @param line Line in the .kicad_sym file to modify.
 */
void Kandle::FileHandler::substitute_footprint(std::string& line) {
    std::string footprint_path;
    std::regex re(R"("Footprint" ".*")");

    footprint_path += R"("Footprint" ")";
    footprint_path += fs::path(library_file_paths.symbol).stem();
    footprint_path += ":";
    footprint_path += fs::path(output_directory).filename();
    footprint_path += R"(")";

    line = std::regex_replace(line, re, footprint_path);
}

bool Kandle::FileHandler::new_symbol_library(const std::string& path) {
    std::vector<std::string> lines = Utils::readlines(path);

    // Create new file
    std::cout << "Creating new symbol library: "
              << library_file_paths.symbol << std::endl;

    std::fstream symbol_file(library_file_paths.symbol,
                             std::fstream::out | std::fstream::app);


    if (symbol_file.is_open()) {
        for (auto& line: lines) {
            if (std::empty(line)) {
                continue;
            }

            // Replace footprint with footprint path
            if (line.find("Footprint") != std::string::npos) {
                substitute_footprint(line);
            }

            symbol_file << line << "\n";
        }
        symbol_file.close();
    } else {
        return false;
    }

    return true;
}

bool Kandle::FileHandler::append_to_symbol_library(const std::string& path) {

    std::vector<std::string> lines = Utils::readlines(path);

    // Append to existing file
    std::vector<std::string> existing_lines = Utils::readlines(
            library_file_paths.symbol);

    long line_number = 0;
    bool valid_library = false;
    for (const auto& line: existing_lines) {
        if (line.find("(kicad_symbol_lib") != std::string::npos) {
            valid_library = true;
        }

        if (line.find(fs::path(path).stem()) != std::string::npos) {
            std::cout << "Component already exists in symbol library."
                      << std::endl;
            return true;
        }

        if (!valid_library) {
            line_number++;
        }
    }

    // Check if the file contains kicad_symbol_lib, if not exit error
    if (!valid_library) {
        std::cerr << "Invalid KiCad symbol library. Exiting." << std::endl;
        exit(1);
    }

    // Ignore symbol header when appending a symbol to library also assign
    // footprint to symbol
    int symbol_header = 0;
    int index = 0;
    for (auto& line: lines) {
        if (line.find("(kicad_symbol_lib") != std::string::npos) {
            symbol_header = index;
        }

        if (line.find("Footprint") != std::string::npos) {
            substitute_footprint(line);
        }

        index++;
    }

    // Get the index of the last non-empty line
    index = 0;
    int symbol_footer = 0;
    for (auto line = lines.rbegin(); line != lines.rend(); ++line) {
        if (!lines.empty()) {
            symbol_footer = index;
        }
        index++;
    }

    // Append the contents of the symbol to the existing symbol library under
    // the line containing "kicad_symbol_lib"
    existing_lines.insert((existing_lines.begin() + line_number + 1),
                          lines.begin() + symbol_header + 1,
                          lines.end() - (long) (lines.size() - symbol_footer));

    // Write the appended contents back to the file (overwrites)
    std::fstream symbol_file(library_file_paths.symbol, std::fstream::out);
    if (symbol_file.is_open()) {
        for (const auto& line: existing_lines) {
            // Ignore empty lines
            if (!std::empty(line)) {
                symbol_file << line << "\n";
            }
        }
        symbol_file.close();
    } else {
        return false;
    }

    return true;
}

bool Kandle::FileHandler::import_symbol(const std::string& path) {

    // Symbol path not found (probably error unpacking .zip file)
    if (std::empty(path)) {
        return false;
    }

    // Library doesn't exist so create one
    if (!fs::exists(library_file_paths.symbol)) {
        return new_symbol_library(path);
    }

    // Append to existing symbol library
    return append_to_symbol_library(path);
}

void Kandle::FileHandler::straight_copy(const std::string& source,
                                        const std::string& dest) {
    std::ifstream source_file(source, std::ios::binary);
    std::ofstream dest_file(dest, std::ios::binary);

    if (!source_file) {
        std::cerr << "Unable to open file: " << source << std::endl;
        exit(1);
    }

    if (!dest_file) {
        std::cerr << "Unable to write to file: " << dest << std::endl;
        exit(1);
    }

    dest_file << source_file.rdbuf();
}

void Kandle::FileHandler::constrain_footprint_text(std::string& line) {
    int idx = 0;
    int offset = 0;
    std::string font_text[3] = {"1", "1", "0.15"};
    std::regex re(R"(\d+(\.\d+)?)");
    std::smatch match;

    while(std::regex_search(line.cbegin() + offset, line.cend(), match, re)) {
        if (idx >= 3) { return; }
        int match_pos = (int)match.position() + offset;

        line.replace(match_pos, match.length(), font_text[idx]);

        offset = match_pos + (int)font_text[idx].length();
        idx++;
    }
}


bool Kandle::FileHandler::import_footprint(const std::string& path) {

    std::string component_path;

    // No footprint library found
    if (std::empty(path)) {
        return false;
    }

    // Create library directory if it doesn't exist
    if (!exists(fs::path(library_file_paths.footprint))) {
        fs::create_directories(library_file_paths.footprint);
    }

    std::vector<std::string> lines = Utils::readlines(path);

    component_path += library_file_paths.footprint;
    component_path += "/";
    component_path += fs::path(output_directory).filename();
    component_path += ".kicad_mod";

    straight_copy(path, component_path);

    // Open newly copied files
    std::fstream footprint_file(component_path, std::fstream::out);

    if (!footprint_file.is_open()) {
        std::cerr << "Cannot open footprint file." << std::endl;
        return false;
    }

    // Find font and replace font size
    for (auto& line : lines) {
      if (std::empty(line)) {
        continue;
      }

      // Replace font size with default font size
      if (line.find("(effects (font (size ") != std::string::npos) {
        constrain_footprint_text(line);
      }

      footprint_file << line << "\n";
    }

    footprint_file.close();

    return true;
}

bool Kandle::FileHandler::import_3dmodel(const std::string& path) {

    std::string component_path;

    // No 3dmodel library found
    if (std::empty(path)) {
        return false;
    }

    // Create library directory if it doesn't exist
    if (!exists(fs::path(library_file_paths.dmodel))) {
        fs::create_directories(library_file_paths.dmodel);
    }

    component_path += library_file_paths.dmodel;
    component_path += "/";
    component_path += fs::path(output_directory).filename();
    component_path += fs::path(path).extension();

    straight_copy(path, component_path);

    return true;
}



