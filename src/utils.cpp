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

#include "utils.hpp"

/**
 * @brief Reads a file into a vector where each value 
 * is a single line.
 *
 * @note Comments are ommitted (lines beginning with '#')
 *
 * @param filename The path to a lib or footprint file.
 * @return Vector of strings representing individual lines.
 */
std::vector<std::string> Utils::readlines(
        const std::string& filename) {

    std::vector<std::string> lines;
    std::ifstream infile(filename, std::ios::in);

    if (infile.is_open()) {
        std::string tmp;
        while (getline(infile, tmp)) {
            if (!tmp.empty()) {
                if (tmp.at(0) != '#') {
                    lines.push_back(tmp);
                }
            }
        }
        infile.close();
    } else {
        std::cout << "File: "
                  << filename << " not found." << std::endl;
        exit(1);
    }

    return lines;
}

bool Utils::assert_true(const char c) {
    return c == 'Y';
}

double Utils::mils_to_millimeters(int mils) {
    return (double) mils * 0.0254;
}

std::string Utils::replace_empty_quotes(const std::string& input) {
    std::string buf;
    std::size_t pos = 0;
    std::size_t prev_pos;
    std::string empty_quotes = "\"\"";
    std::string replacement = "\"None\"";

    buf.reserve(input.size());

    while (true) {
        prev_pos = pos;
        pos = input.find(empty_quotes, pos);
        if (pos == std::string::npos) { break; }
        buf.append(input, prev_pos, pos - prev_pos);
        buf.append(replacement);
        pos += empty_quotes.size();
    }

    buf.append(input, prev_pos, input.size() - prev_pos);

    return buf;
}

