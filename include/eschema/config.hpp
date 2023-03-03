
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

#pragma once

#include <iostream>
#include <string>

// Divide these by 100 to get their float representation
// i.e. Pin spacing should be 254 / 100 = 2.54
enum class V6Dimensions {
    PIN_LENGTH = 254,
    PIN_SPACING = 254,
    PIN_NUM_SIZE = 127,
    PIN_NAME_SIZE = 127,
    DEFAULT_BOX_LINE_WIDTH = 0,
    PROPERTY_FONT_SIZE = 127,
    FIELD_OFFSET_START = 508,
    FIELD_OFFSET_INCREMENT = 254
};

enum class V5Dimensions {
    PIN_LENGTH = 100,
    PIN_SPACING = 100,
    PIN_NUM_SIZE = 50,
    PIN_NAME_SIZE = 50,
    PIN_NAME_OFFSET = 40,
    DEFAULT_BOX_LINE_WIDTH = 0,
    FIELD_FONT_SIZE = 60,
    FIELD_OFFSET_START = 200,
    FIELD_OFFSET_INCREMENT = 100
};

enum class V5PinType : char {
    input = 'I',
    output = 'O',
    bidirectional = 'B',
    tri_state = 'T',
    passive = 'P',
    _free = 'U',
    unspecified = 'U',
    power_in = 'W',
    power_out = 'W',
    open_collector = 'C',
    open_emitter = 'E',
    no_connect = 'N'
};

enum class V5PinStyle : char {
    //line = '',
    inverted = 'I',
    _clock = 'C',
    inverted_clock = 'F',
    input_low = 'L',
    //clock_low = 'CL' ERROR
    output_low = 'V',
    edge_clock_high = 'C',
    non_logic = 'X'
};


enum class V5BoxFill : char {
    _none = 'N',
    outline = 'F',
    background = 'f'
};

enum class V5Identifiers : char {
    information = 'F',
    pin = 'X',
    rectangle = 'S',
    polygon = 'P',
    circle = 'C',
    arc = 'A',
    text = 'T'
};

