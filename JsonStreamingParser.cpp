/**The MIT License (MIT)

 Copyright (c) 2015 by Daniel Eichhorn

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

 See more at http://blog.squix.ch and https://github.com/squix78/json-streaming-parser
 */

#include "JsonStreamingParser.h"

#ifdef USE_LONG_ERRORS
const char PROGMEM_ERR0[] PROGMEM = "Unescaped control character encountered: %c at position: %d";
const char PROGMEM_ERR1[] PROGMEM = "Start of string expected for object key. Instead got: %c at position: %d";
const char PROGMEM_ERR2[] PROGMEM = "Expected ':' after key. Instead got %c at position %d";
const char PROGMEM_ERR3[] PROGMEM = "Expected ',' or '}' while parsing object. Got: %c at position %d";
const char PROGMEM_ERR4[] PROGMEM = "Expected ',' or ']' while parsing array. Got: %c at position: %d";
const char PROGMEM_ERR5[] PROGMEM = "Finished a literal, but unclear what state to move to. Last state: %d";
const char PROGMEM_ERR6[] PROGMEM = "Cannot have multiple decimal points in a number at: %d";
const char PROGMEM_ERR7[] PROGMEM = "Cannot have a decimal point in an exponent at: %d";
const char PROGMEM_ERR8[] PROGMEM = "Cannot have multiple exponents in a number at: %d";
const char PROGMEM_ERR9[] PROGMEM = "Can only have '+' or '-' after the 'e' or 'E' in a number at: %d";
const char PROGMEM_ERR10[] PROGMEM = "Document must start with object or array";
const char PROGMEM_ERR11[] PROGMEM = "Expected end of document";
const char PROGMEM_ERR12[] PROGMEM = "Internal error. Reached an unknown state at: %d";
const char PROGMEM_ERR13[] PROGMEM = "Unexpected end of string";
const char PROGMEM_ERR14[] PROGMEM = "Unexpected character for value";
const char PROGMEM_ERR15[] PROGMEM = "Unexpected end of array encountered";
const char PROGMEM_ERR16[] PROGMEM = "Unexpected end of object encountered";
const char PROGMEM_ERR17[] PROGMEM = "Expected escaped character after backslash";
const char PROGMEM_ERR18[] PROGMEM = "Expected hex character for escaped Unicode character";
const char PROGMEM_ERR19[] PROGMEM = "Expected '\\u' following a Unicode high surrogate";
const char PROGMEM_ERR20[] PROGMEM = "Expected 'true'";
const char PROGMEM_ERR21[] PROGMEM = "Expected 'false'";
const char PROGMEM_ERR22[] PROGMEM = "Expected 'null'";
#else
const char PROGMEM_ERR0[] PROGMEM = "err0: %c at: %d";
const char PROGMEM_ERR1[] PROGMEM = "err1: %c at: %d";
const char PROGMEM_ERR2[] PROGMEM = "err2: %c at: %d";
const char PROGMEM_ERR3[] PROGMEM = "err3: %c at: %d";
const char PROGMEM_ERR4[] PROGMEM = "err4: %c at: %d";
const char PROGMEM_ERR5[] PROGMEM = "err5: %d";
const char PROGMEM_ERR6[] PROGMEM = "err6: at: %d";
const char PROGMEM_ERR7[] PROGMEM = "err7: at: %d";
const char PROGMEM_ERR8[] PROGMEM = "err8: at: %d";
const char PROGMEM_ERR9[] PROGMEM = "err9: at: %d";
const char PROGMEM_ERR10[] PROGMEM = "err10";
const char PROGMEM_ERR11[] PROGMEM = "err11";
const char PROGMEM_ERR12[] PROGMEM = "err12: %d";
const char PROGMEM_ERR13[] PROGMEM = "err13";
const char PROGMEM_ERR14[] PROGMEM = "err14";
const char PROGMEM_ERR15[] PROGMEM = "err15";
const char PROGMEM_ERR16[] PROGMEM = "err16";
const char PROGMEM_ERR17[] PROGMEM = "err17";
const char PROGMEM_ERR18[] PROGMEM = "err18";
const char PROGMEM_ERR19[] PROGMEM = "err19";
const char PROGMEM_ERR20[] PROGMEM = "err20";
const char PROGMEM_ERR21[] PROGMEM = "err21";
const char PROGMEM_ERR22[] PROGMEM = "err22";
#endif

JsonStreamingParser::JsonStreamingParser() {
	reset();
}

void JsonStreamingParser::reset() {
	state = STATE_START_DOCUMENT;
	bufferPos = 0;
	unicodeEscapeBufferPos = 0;
	unicodeBufferPos = 0;
	characterCounter = 0;
}

void JsonStreamingParser::setListener(JsonListener* listener) {
	myListener = listener;
}

bool JsonStreamingParser::parse(char c) {
//    Serial.println(c);
	// valid whitespace characters in JSON (from RFC4627 for JSON) include:
	// space, horizontal tab, line feed or new line, and carriage return.
	// thanks:
	// http://stackoverflow.com/questions/16042274/definition-of-whitespace-in-json

	if ((c == ' ' || c == '\t' || c == '\n' || c == '\r')
			&& !(state == STATE_IN_STRING || state == STATE_UNICODE
					|| state == STATE_START_ESCAPE || state == STATE_IN_NUMBER
					|| state == STATE_START_DOCUMENT)) {
		return true;
	}

	switch (state) {
	case STATE_IN_STRING:
		if (c == '"') {
			endString();
		} else if (c == '\\') {
			state = STATE_START_ESCAPE;
		} else if ((c < 0x1f) || (c == 0x7f)) {
			sprintf_P(errorMessage, PROGMEM_ERR0, c, characterCounter);
			myListener->error(errorMessage);
			return false;
		} else {
			buffer[bufferPos] = c;
			increaseBufferPointer();
		}
		break;
	case STATE_IN_ARRAY:
		if (c == ']') {
			endArray();
		} else {
			startValue(c);
		}
		break;
	case STATE_IN_OBJECT:
		if (c == '}') {
			endObject();
		} else if (c == '"') {
			startKey();
		} else {
			sprintf_P(errorMessage, PROGMEM_ERR1, c, characterCounter);
			myListener->error(errorMessage);
			return false;
		}
		break;
	case STATE_END_KEY:
		if (c != ':') {
			sprintf_P(errorMessage, PROGMEM_ERR2, c, characterCounter);
			myListener->error(errorMessage);
			return false;
		}
		state = STATE_AFTER_KEY;
		break;
	case STATE_AFTER_KEY:
		startValue(c);
		break;
	case STATE_START_ESCAPE:
		processEscapeCharacters(c);
		break;
	case STATE_UNICODE:
		processUnicodeCharacter(c);
		break;
	case STATE_UNICODE_SURROGATE:
		unicodeEscapeBuffer[unicodeEscapeBufferPos] = c;
		unicodeEscapeBufferPos++;
		if (unicodeEscapeBufferPos == 2) {
			endUnicodeSurrogateInterstitial();
		}
		break;
	case STATE_AFTER_VALUE: {
		// not safe for size == 0!!!
		int within = peek();
		if (within == STACK_OBJECT) {
			if (c == '}') {
				endObject();
			} else if (c == ',') {
				state = STATE_IN_OBJECT;
			} else {
				sprintf_P(errorMessage, PROGMEM_ERR3, c, characterCounter);
				myListener->error(errorMessage);
				return false;
			}
		} else if (within == STACK_ARRAY) {
			if (c == ']') {
				endArray();
			} else if (c == ',') {
				state = STATE_IN_ARRAY;
			} else {
				sprintf_P(errorMessage, PROGMEM_ERR4, c, characterCounter);
				myListener->error(errorMessage);
				return false;
			}
		} else {
			sprintf_P(errorMessage, PROGMEM_ERR5, characterCounter);
			myListener->error(errorMessage);
			return false;
		}
	}
		break;
	case STATE_IN_NUMBER:
		if (c >= '0' && c <= '9') {
			buffer[bufferPos] = c;
			increaseBufferPointer();
		} else if (c == '.') {
			if (doesCharArrayContain(buffer, bufferPos, '.')) {
				sprintf_P(errorMessage, PROGMEM_ERR6, characterCounter);
				myListener->error(errorMessage);
				return false;
			} else if (doesCharArrayContain(buffer, bufferPos, 'e')) {
				sprintf_P(errorMessage, PROGMEM_ERR7, characterCounter);
				myListener->error(errorMessage);
				return false;
			}
			buffer[bufferPos] = c;
			increaseBufferPointer();
		} else if (c == 'e' || c == 'E') {
			if (doesCharArrayContain(buffer, bufferPos, 'e')) {
				sprintf_P(errorMessage, PROGMEM_ERR8, characterCounter);
				myListener->error(errorMessage);
				return false;
			}
			buffer[bufferPos] = c;
			increaseBufferPointer();
		} else if (c == '+' || c == '-') {
			char last = buffer[bufferPos - 1];
			if (!(last == 'e' || last == 'E')) {
				sprintf_P(errorMessage, PROGMEM_ERR9, characterCounter);
				myListener->error(errorMessage);
				return false;
			}
			buffer[bufferPos] = c;
			increaseBufferPointer();
		} else {
			endNumber();
			// we have consumed one beyond the end of the number
			parse(c);
		}
		break;
	case STATE_IN_TRUE:
		buffer[bufferPos] = c;
		increaseBufferPointer();
		if (bufferPos == 4) {
			endTrue();
		}
		break;
	case STATE_IN_FALSE:
		buffer[bufferPos] = c;
		increaseBufferPointer();
		if (bufferPos == 5) {
			endFalse();
		}
		break;
	case STATE_IN_NULL:
		buffer[bufferPos] = c;
		increaseBufferPointer();
		if (bufferPos == 4) {
			endNull();
		}
		break;
	case STATE_START_DOCUMENT: {
		myListener->startDocument();
		if (c == '[') {
			startArray();
		} else if (c == '{') {
			startObject();
		} else {
			sprintf_P(errorMessage, PROGMEM_ERR10);
			myListener->error(errorMessage);
			return false;
		}
		break;
	}
	case STATE_DONE: {
		sprintf_P(errorMessage, PROGMEM_ERR11);
		myListener->error(errorMessage);
		return false;
	}
	default: {
		sprintf_P(errorMessage, PROGMEM_ERR12, characterCounter);
		myListener->error(errorMessage);
		return false;
	}
	}

	characterCounter++;

	return true;
}

void JsonStreamingParser::increaseBufferPointer() {
	bufferPos = min(bufferPos + 1, BUFFER_MAX_LENGTH - 1);
}

void JsonStreamingParser::push(int value) {
	stack[stackPos] = value;
	stackPos++;
}

int JsonStreamingParser::pop() {
	stackPos--;
	return stack[stackPos];
}

int JsonStreamingParser::peek() {
	return stack[stackPos - 1];
}


void JsonStreamingParser::startKey() {
	push(STACK_KEY);
	state = STATE_IN_STRING;
}

void JsonStreamingParser::startValue(char c) {
	if (c == '[') {
		startArray();
	} else if (c == '{') {
		startObject();
	} else if (c == '"') {
		startString();
	} else if (isDigit(c)) {
		startNumber(c);
	} else if (c == 't') {
		state = STATE_IN_TRUE;
		buffer[bufferPos] = c;
		increaseBufferPointer();
	} else if (c == 'f') {
		state = STATE_IN_FALSE;
		buffer[bufferPos] = c;
		increaseBufferPointer();
	} else if (c == 'n') {
		state = STATE_IN_NULL;
		buffer[bufferPos] = c;
		increaseBufferPointer();
	} else {
		sprintf_P(errorMessage, PROGMEM_ERR14);
		myListener->error(errorMessage);
		return;
	}
}

void JsonStreamingParser::startObject() {
	myListener->startObject();
	state = STATE_IN_OBJECT;
	push(STACK_OBJECT);
}

void JsonStreamingParser::endObject() {
	if (pop() != STACK_OBJECT) {
		sprintf_P(errorMessage, PROGMEM_ERR16);
		myListener->error(errorMessage);
		return;
	}
	myListener->endObject();
	state = STATE_AFTER_VALUE;
	if (stackPos == 0) {
		endDocument();
	}
}

void JsonStreamingParser::startArray() {
	myListener->startArray();
	state = STATE_IN_ARRAY;
	push(STACK_ARRAY);
}

void JsonStreamingParser::endArray() {
	if (pop() != STACK_ARRAY) {
		sprintf_P(errorMessage, PROGMEM_ERR15);
		myListener->error(errorMessage);
		return;
	}
	myListener->endArray();
	state = STATE_AFTER_VALUE;
	if (stackPos == 0) {
		endDocument();
	}
}

void JsonStreamingParser::startString() {
	push(STACK_STRING);
	state = STATE_IN_STRING;
}

void JsonStreamingParser::endString() {
	int popped = pop();
	if (popped == STACK_KEY) {
		buffer[bufferPos] = '\0';
		myListener->key(buffer);
		state = STATE_END_KEY;
	} else if (popped == STACK_STRING) {
		buffer[bufferPos] = '\0';
		myListener->value(buffer);
		state = STATE_AFTER_VALUE;
	} else {
		sprintf_P(errorMessage, PROGMEM_ERR13);
		myListener->error(errorMessage);
		return;
	}
	bufferPos = 0;
}

void JsonStreamingParser::startNumber(char c) {
	state = STATE_IN_NUMBER;
	buffer[bufferPos] = c;
	increaseBufferPointer();
}

void JsonStreamingParser::endNumber() {
	buffer[bufferPos] = '\0';
	myListener->value(buffer);
	bufferPos = 0;
	state = STATE_AFTER_VALUE;
}

void JsonStreamingParser::endTrue() {
	buffer[bufferPos] = '\0';
	// String value = String(buffer);
	if (strncmp(buffer, "true", 4) == 0) {
		myListener->value("true");
	} else {
		sprintf_P(errorMessage, PROGMEM_ERR20);
		myListener->error(errorMessage);
		return;
	}
	bufferPos = 0;
	state = STATE_AFTER_VALUE;
}

void JsonStreamingParser::endFalse() {
	buffer[bufferPos] = '\0';
	// String value = String(buffer);
	if (strncmp(buffer, "false", 5) == 0) {
		myListener->value("false");
	} else {
		sprintf_P(errorMessage, PROGMEM_ERR21);
		myListener->error(errorMessage);
		return;
	}
	bufferPos = 0;
	state = STATE_AFTER_VALUE;
}

void JsonStreamingParser::endNull() {
	buffer[bufferPos] = '\0';
	// String value = String(buffer);
	if (strncmp(buffer, "null", 4) == 0) {
		myListener->value("null");
	} else {
		sprintf_P(errorMessage, PROGMEM_ERR22);
		myListener->error(errorMessage);
		return;
	}
	bufferPos = 0;
	state = STATE_AFTER_VALUE;
}

void JsonStreamingParser::endDocument() {
	myListener->endDocument();
	state = STATE_START_DOCUMENT;
}

boolean JsonStreamingParser::isDigit(char c) {
	// Only concerned with the first character in a number.
	return (c >= '0' && c <= '9') || c == '-';
}

boolean JsonStreamingParser::isHexCharacter(char c) {
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')
			|| (c >= 'A' && c <= 'F');
}

boolean JsonStreamingParser::doesCharArrayContain(char myArray[], int length,
		char c) {
	for (int i = 0; i < length; i++) {
		if (myArray[i] == c) {
			return true;
		}
	}
	return false;
}

void JsonStreamingParser::processEscapeCharacters(char c) {
	if (c == '"') {
		buffer[bufferPos] = '"';
		increaseBufferPointer();
	} else if (c == '\\') {
		buffer[bufferPos] = '\\';
		increaseBufferPointer();
	} else if (c == '/') {
		buffer[bufferPos] = '/';
		increaseBufferPointer();
	} else if (c == 'b') {
		buffer[bufferPos] = 0x08;
		increaseBufferPointer();
	} else if (c == 'f') {
		buffer[bufferPos] = '\f';
		increaseBufferPointer();
	} else if (c == 'n') {
		buffer[bufferPos] = '\n';
		increaseBufferPointer();
	} else if (c == 'r') {
		buffer[bufferPos] = '\r';
		increaseBufferPointer();
	} else if (c == 't') {
		buffer[bufferPos] = '\t';
		increaseBufferPointer();
	} else if (c == 'u') {
		state = STATE_UNICODE;
	} else {
		sprintf_P(errorMessage, PROGMEM_ERR17);
		myListener->error(errorMessage);
		return;
	}
	if (state != STATE_UNICODE) {
		state = STATE_IN_STRING;
	}
}

void JsonStreamingParser::processUnicodeCharacter(char c) {
	if (!isHexCharacter(c)) {
		sprintf_P(errorMessage, PROGMEM_ERR18);
		myListener->error(errorMessage);
		return;
	}

	unicodeBuffer[unicodeBufferPos] = c;
	unicodeBufferPos++;

	if (unicodeBufferPos == 4) {
		int codepoint = getHexArrayAsDecimal(unicodeBuffer, unicodeBufferPos);
		endUnicodeCharacter(codepoint);
		return;
		/*if (codepoint >= 0xD800 && codepoint < 0xDC00) {
		 unicodeHighSurrogate = codepoint;
		 unicodeBufferPos = 0;
		 state = STATE_UNICODE_SURROGATE;
		 } else if (codepoint >= 0xDC00 && codepoint <= 0xDFFF) {
		 if (unicodeHighSurrogate == -1) {
		 // throw new ParsingError($this->_line_number,
		 // $this->_char_number,
		 // "Missing high surrogate for Unicode low surrogate.");
		 }
		 int combinedCodePoint = ((unicodeHighSurrogate - 0xD800) * 0x400) + (codepoint - 0xDC00) + 0x10000;
		 endUnicodeCharacter(combinedCodePoint);
		 } else if (unicodeHighSurrogate != -1) {
		 // throw new ParsingError($this->_line_number,
		 // $this->_char_number,
		 // "Invalid low surrogate following Unicode high surrogate.");
		 endUnicodeCharacter(codepoint);
		 } else {
		 endUnicodeCharacter(codepoint);
		 }*/
	}
}

int JsonStreamingParser::getHexArrayAsDecimal(char hexArray[], int length) {
	int result = 0;
	for (int i = 0; i < length; i++) {
		char current = hexArray[length - i - 1];
		int value = 0;
		if (current >= 'a' && current <= 'f') {
			value = current - 'a' + 10;
		} else if (current >= 'A' && current <= 'F') {
			value = current - 'A' + 10;
		} else if (current >= '0' && current <= '9') {
			value = current - '0';
		}
		result += value * 16 ^ i;
	}
	return result;
}

void JsonStreamingParser::endUnicodeSurrogateInterstitial() {
	char unicodeEscape = unicodeEscapeBuffer[unicodeEscapeBufferPos - 1];
	if (unicodeEscape != 'u') {
		sprintf_P(errorMessage, PROGMEM_ERR19);
		myListener->error(errorMessage);
		return;
	}
	unicodeBufferPos = 0;
	unicodeEscapeBufferPos = 0;
	state = STATE_UNICODE;
}

int JsonStreamingParser::convertDecimalBufferToInt(char myArray[], int length) {
	int result = 0;
	for (int i = 0; i < length; i++) {
		char current = myArray[length - i - 1];
		result += (current - '0') * 10;
	}
	return result;
}

void JsonStreamingParser::endUnicodeCharacter(int codepoint) {
	buffer[bufferPos] = convertCodepointToCharacter(codepoint);
	increaseBufferPointer();
	unicodeBufferPos = 0;
	unicodeHighSurrogate = -1;
	state = STATE_IN_STRING;
}

char JsonStreamingParser::convertCodepointToCharacter(int num) {
	if (num <= 0x7F)
		return (char) (num);
	// if(num<=0x7FF) return (char)((num>>6)+192) + (char)((num&63)+128);
	// if(num<=0xFFFF) return
	// chr((num>>12)+224).chr(((num>>6)&63)+128).chr((num&63)+128);
	// if(num<=0x1FFFFF) return
	// chr((num>>18)+240).chr(((num>>12)&63)+128).chr(((num>>6)&63)+128).chr((num&63)+128);
	return ' ';
}
