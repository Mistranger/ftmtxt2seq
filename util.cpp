#include "util.h"

Utility::Utility()
{
}


Utility::~Utility()
{
}

void Utility::writeLE16(std::ofstream &stream, uint16_t data)
{
	stream.put(data & 0xFF);
	stream.put(data >> 8);
}

void Utility::writeLE32(std::ofstream &stream, uint32_t data)
{
	stream.put(data & 0xFF);
	stream.put((data >> 8) & 0xFF);
	stream.put((data >> 16) & 0xFF);
	stream.put((data >> 24) & 0xFF);
}
