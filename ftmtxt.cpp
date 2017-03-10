#include "ftmtxt.h"
#include "util.h"
#include <fstream>
#include <iostream>
#include <string>


std::string strGetBetween(std::string &str, char c = '\"')
{
	std::string::size_type start_position;
	std::string::size_type end_position;
	std::string found_text;

	start_position = str.find(c);
	if (start_position != std::string::npos) {
		++start_position; // start after the double quotes.
						  // look for end position;
		end_position = str.find(c, start_position);
		if (end_position != std::string::npos) {
			found_text = str.substr(start_position, end_position - start_position - 1);
		}
	}
	return found_text;
}

std::vector<std::string> tokenize(const char *str, char c = ' ')
{
	std::vector<std::string> result;

	do
	{
		while (*str == c && *str) {
			str++;
		}

		const char *begin = str;
		while (*str != c && *str) {
			str++;
		}

		result.push_back(std::string(begin, str));
	} while (0 != *str++);

	return result;
}

FTMtxt::FTMtxt(): isPal(false), fps(0), channelCount(0), expansionColSize(0), expansion(EC_NONE),
                  split(0), n163chanells(0), patternLength(0), trackSpeed(0), trackTempo(0)
{
}


FTMtxt::~FTMtxt()
{
}

void FTMtxt::parseInstruments(std::ifstream &iFile)
{
	std::string buf;
	do {
		std::getline(iFile, buf);
		if (!buf.empty()) {
			std::vector<std::string> tokens = tokenize(buf.c_str());
			if (!tokens[0].compare("INST2A03")) {
				FTMInstrument *ins = new FTMInstrument;
				ins->index = atoi(tokens[1].c_str());
				ins->volMacro = atoi(tokens[2].c_str());
				ins->arpMacro = atoi(tokens[3].c_str());
				ins->pitchMacro = atoi(tokens[4].c_str());
				ins->hipitchMacro = atoi(tokens[5].c_str());
				ins->dutyMacro = atoi(tokens[6].c_str());
				ins->name = strGetBetween(tokens[7]);
				this->instrumentList.push_back(ins);
			} else if (!tokens[0].compare("KEYDPCM")) {
				FTMInstrument *ins = this->instrumentList.at(atoi(tokens[1].c_str()));
				if (ins) {
					ins->extra.keyDPCM.octave = atoi(tokens[2].c_str());
					ins->extra.keyDPCM.note = atoi(tokens[3].c_str());
					ins->extra.keyDPCM.sample = atoi(tokens[4].c_str());
					ins->extra.keyDPCM.pitch = atoi(tokens[5].c_str());
					ins->extra.keyDPCM.loop = atoi(tokens[6].c_str());
					ins->extra.keyDPCM.loopPoint = atoi(tokens[7].c_str());
					ins->extra.keyDPCM.delta = atoi(tokens[8].c_str());
				}
			}
		}
	} while (!buf.empty());
}

int FTMtxt::loadText(const char *fileName)
{
	std::ifstream iFile(fileName, std::ios::in);
	if (!iFile.is_open()) {
		return -1;
	}

	std::string buf;
	std::getline(iFile, buf);
	if (buf.find("# FamiTracker text export") == std::string::npos) {
		return -1;
	}

	while (!iFile.eof()) {
		std::getline(iFile, buf);
		if (buf.find("#") != std::string::npos) {
			if (buf.find("# Song information") != std::string::npos) {
				this->parseSongInfo(iFile);
			} else if (buf.find("# Song comment") != std::string::npos) {
				this->parseComment(iFile);
			} else if (buf.find("# Global settings") != std::string::npos) {
				this->parseGlobalSettings(iFile);
			} else if (buf.find("# Macros") != std::string::npos) {
			} else if (buf.find("# DPCM samples") != std::string::npos) {
				this->parseDPCMSamples(iFile);
			} else if (buf.find("# Instruments") != std::string::npos) {
				this->parseInstruments(iFile);
			} else if (buf.find("# Tracks") != std::string::npos) {
				this->parseTracks(iFile);
			}
		}
	}
	return 0;
}

void FTMtxt::parseSongInfo(std::ifstream &iFile)
{
	std::string buf;
	std::getline(iFile, buf);
	this->songTitle = strGetBetween(buf);
	std::getline(iFile, buf);
	this->songAuthor = strGetBetween(buf);
	std::getline(iFile, buf);
	this->songCopyright = strGetBetween(buf);
}

void FTMtxt::parseComment(std::ifstream &iFile)
{
	std::string buf;
	std::getline(iFile, buf);
	this->songComment = strGetBetween(buf, '\"');
}

void FTMtxt::parseDPCMSamples(std::ifstream &iFile)
{
	this->sampleList.clear();
	std::string buf;

	while (true) {
		std::getline(iFile, buf);
		std::vector<std::string> tokens = tokenize(buf.c_str());
		if (tokens.size() != 4 || tokens[0].compare("DPCMDEF")) {
			break;
		}

		DPCMSample *sample = new DPCMSample;
		sample->length = atoi(tokens[2].c_str());
		sample->sampleName = strGetBetween(tokens[3]);
		uint8_t *samples = new uint8_t[sample->length];
		int i = 0;
		while (i < sample->length) {
			std::getline(iFile, buf);
			tokens = tokenize(buf.c_str());
			for (size_t j = 0; j < tokens.size(); ++j) {
				if (!tokens[j].compare("DPCM") || !tokens[j].compare(":")) {
					continue;
				}
				samples[i++] = std::stoi(tokens[j], nullptr, 0x10);
			}
		}
		sample->sample = samples;
		sampleList.push_back(sample);
	}
}

void FTMtxt::parseTracks(std::ifstream &iFile)
{
	this->PatternList.clear();
	this->columnSize.clear();
	this->patternOrder.clear();

	std::string buf;
	do {
		std::getline(iFile, buf);
		if (!buf.empty()) {
			std::vector<std::string> tokens = tokenize(buf.c_str());
			if (!tokens[0].compare("TRACK")) {
				this->patternLength = atoi(tokens[1].c_str());
				this->trackSpeed = atoi(tokens[2].c_str());
				this->trackTempo = atoi(tokens[3].c_str());
			} else if (!tokens[0].compare("COLUMNS")) {
				for (size_t i = 2; i < tokens.size(); ++i) {
					this->columnSize.push_back(atoi(tokens[i].c_str()));
				}
			} else if (!tokens[0].compare("ORDER")) {
				std::vector<int> order;
				for (size_t i = 3; i < tokens.size(); ++i) {
					order.push_back(std::stoul(tokens[i], nullptr, 0x10));
				}
				this->patternOrder.push_back(order);
			} else if (!tokens[0].compare("PATTERN")) {
				this->parsePattern(iFile, atoi(tokens[1].c_str()));
			} 
		}
	} while (buf.compare("# End of export"));
}

void FTMtxt::parseGlobalSettings(std::ifstream &iFile)
{
	std::string buf;
	do {
		std::getline(iFile, buf);
		if (!buf.empty()) {
			std::vector<std::string> tokens = tokenize(buf.c_str());
			if (!tokens[0].compare("MACHINE")) {
				this->isPal = static_cast<bool>(atoi(tokens[1].c_str()));
			} else if (!tokens[0].compare("EXPANSION")) {
				this->expansion = static_cast<expansionChips>(atoi(tokens[1].c_str()));
				switch (this->expansion) {
				case EC_NONE:
					channelCount = 5;
					break;
				case EC_VRC6:
					channelCount = 8;
					break;
				case EC_VRC7:
					channelCount = 11;
					break;
				case EC_FDS:
					channelCount = 6;
					break;
				case EC_MMC5:
					channelCount = 7;
					break;
				case EC_N163:
					channelCount = 13;
					break;
				case EC_S5B:
					channelCount = 8;
					break;
				default:
					break;
				}
			}
		}
	} while (!buf.empty());
}

void FTMtxt::parsePattern(std::ifstream &iFile, int patternNum)
{
	std::string buf;
	FTMPattern *pattern = new FTMPattern;
	for (int i = 0; i < this->patternLength; ++i) {
		FTMPatternRow *row = new FTMPatternRow;
		std::getline(iFile, buf);
		std::vector<std::string> tokens = tokenize(buf.c_str());
		int ct = 2; //current token
		for (int j = 0; j < channelCount; ++j) {
			FTMPatternRowEntry *entry = new FTMPatternRowEntry;
			if (!tokens[++ct].compare("---")) {
				entry->note = noteCut;
			} else if (!tokens[ct].compare("...")) {
				entry->note = noteNo;
			} else if (!tokens[ct].compare("===")) {
				entry->note = noteRelease;
			} else {
				entry->note = noteToInt(tokens[ct].c_str(), j == 3);
			}
			if (!tokens[++ct].compare("..")) {
				entry->instrument = -1;
			} else {
				entry->instrument = atoi(tokens[ct].c_str());
			}
			
			if (tokens[++ct][0] != '.') {
				entry->volume = Utility::hexCharToInt(tokens[ct][0]);
			} else {
				entry->volume = -1;
			}
			if (!tokens[++ct].compare("...")) {
				entry->effect1 = 0;
				entry->effect1data = -1;
			} else {
				entry->effect1 = tokens[ct][0];
				entry->effect1data = std::stoi(tokens[ct].substr(1, 2), nullptr, 0x10);
			}
			
			if (columnSize[j] >= 2) {
				if (!tokens[++ct].compare("...")) {
					entry->effect2 = 0;
					entry->effect2data = -1;
				} else {
					entry->effect2 = tokens[ct][0];
					entry->effect2data =  std::stoi(tokens[ct].substr(1, 2), nullptr, 0x10);
				}
				
			}
			if (columnSize[j] >= 3) {
				if (!tokens[++ct].compare("...")) {
					entry->effect3 = 0;
					entry->effect3data = -1;
				} else {
					entry->effect3 = tokens[ct][0];
					entry->effect3data =  std::stoi(tokens[ct].substr(1, 2), nullptr, 0x10);
				}
			}
			++ct; // : delimeter	
			row->push_back(entry);
		}
		pattern->push_back(row);
	}
	this->PatternList.push_back(pattern);
}

int FTMtxt::noteToInt(const char *note, bool noise)
{
	if (noise) {
		return Utility::hexCharToInt(note[0]);
	}
	const char notes[] = { 'C', 'C', 'D', 'D', 'E', 'F', 'F', 'G', 'G', 'A', 'A', 'B' };
	int res;
	for (int i = 0; i < sizeof(notes) / sizeof(char); ++i) {
		if (note[0] == notes[i]) {
			res = i;
			if (note[1] == '#') {
				res += 1;
			}
			break;
		}
	}
	return res + 12 * atoi(&note[2]);
}

