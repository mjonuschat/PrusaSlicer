#include "Slic3r/Utils.hpp"
#include "Slic3r/Assert.hpp"

namespace Slic3r {

size_t get_utf8_sequence_length(const std::string& text, size_t pos)
{
	ASSERT(pos < text.size());
	return get_utf8_sequence_length(text.c_str() + pos, text.size() - pos);
}

size_t get_utf8_sequence_length(const char *seq, size_t size)
{
	size_t length = 0;
	unsigned char c = seq[0];
	if (c < 0x80) { // 0x00-0x7F
		// is ASCII letter
		length++;
	}
	// Bytes 0x80 to 0xBD are trailer bytes in a multibyte sequence.
	// pos is in the middle of a utf-8 sequence. Add the utf-8 trailer bytes.
	else if (c < 0xC0) { // 0x80-0xBF
		length++;
		while (length < size) {
			c = seq[length];
			if (c < 0x80 || c >= 0xC0) {
				break; // prevent overrun
			}
			length++; // add a utf-8 trailer byte
		}
	}
	// Bytes 0xC0 to 0xFD are header bytes in a multibyte sequence.
	// The number of one bits above the topmost zero bit indicates the number of bytes (including this one) in the whole sequence.
	else if (c < 0xE0) { // 0xC0-0xDF
	 // add a utf-8 sequence (2 bytes)
		if (2 > size) {
			return size; // prevent overrun
		}
		length += 2;
	}
	else if (c < 0xF0) { // 0xE0-0xEF
	 // add a utf-8 sequence (3 bytes)
		if (3 > size) {
			return size; // prevent overrun
		}
		length += 3;
	}
	else if (c < 0xF8) { // 0xF0-0xF7
	 // add a utf-8 sequence (4 bytes)
		if (4 > size) {
			return size; // prevent overrun
		}
		length += 4;
	}
	else if (c < 0xFC) { // 0xF8-0xFB
	 // add a utf-8 sequence (5 bytes)
		if (5 > size) {
			return size; // prevent overrun
		}
		length += 5;
	}
	else if (c < 0xFE) { // 0xFC-0xFD
	 // add a utf-8 sequence (6 bytes)
		if (6 > size) {
			return size; // prevent overrun
		}
		length += 6;
	}
	else { // 0xFE-0xFF
	 // not a utf-8 sequence
		length++;
	}
	return length;
}

}
