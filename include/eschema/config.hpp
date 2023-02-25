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

