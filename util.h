#pragma once
#include <cstdint>
#include <fstream>

class Utility
{
public:
	Utility();
	~Utility();

	static int hexCharToInt(char hex) {
		return (hex >= 'A') ? (hex - 'A' + 10) : ((hex >= 'a') ? (hex - 'a' + 10) : (hex - '0'));
	}

	static int letterToInt(char let) {
		return (let >= 'A' && let <= 'Z') ? (let - 'A') : (let - 'a');
	}

	static void writeLE16(std::ofstream &stream, uint16_t data);
	static void writeLE32(std::ofstream &stream, uint32_t data);
};

