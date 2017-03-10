#pragma once
#include <cstdint>
#include <list>
#include <string>
#include <vector>

struct FTMPatternRowEntry {
	int note;
	int instrument;
	int volume;
	char effect1;
	int effect1data;
	char effect2;
	int effect2data;
	char effect3;
	int effect3data;
};

typedef std::list<FTMPatternRowEntry* > FTMPatternRow;
typedef std::list<FTMPatternRow* > FTMPattern;

class FTMtxt
{
public:
	FTMtxt();
	~FTMtxt();
	void parseInstruments(std::ifstream &iFile);
	int loadText(const char *fileName);
	void parseSongInfo(std::ifstream &iFile);
	void parseComment(std::ifstream &iFile);
	void parseDPCMSamples(std::ifstream &iFile);
	void parseTracks(std::ifstream &iFile);
	void parseGlobalSettings(std::ifstream &iFile);
	void parsePattern(std::ifstream &iFile, int patternNum);

	int noteToInt(const char *note, bool noise = false);

	const static int noteNo = 253;
	const static int noteCut = 254;
	const static int noteRelease = 255;

	struct DPCMSample {
		std::string sampleName;
		size_t length;
		uint8_t *sample;
	};

	struct FTMInstrument {
		uint8_t index;
		uint8_t volMacro;
		uint8_t arpMacro;
		uint8_t pitchMacro;
		uint8_t hipitchMacro;
		uint8_t dutyMacro;
		std::string name;

		union {
			struct {
				uint8_t insIndex;
				uint8_t octave;
				uint8_t note;
				uint8_t sample;
				uint8_t pitch;
				uint8_t loop;
				uint8_t loopPoint;
				int8_t delta;
			} keyDPCM;
		} extra;
	};

	std::string songTitle;
	std::string songAuthor;
	std::string songCopyright;
	std::string songComment;

	bool isPal;
	int fps;

	enum expansionChips {
		EC_NONE = 0x00,
		EC_VRC6 = 0x01,
		EC_VRC7 = 0x02,
		EC_FDS = 0x04,
		EC_MMC5 = 0x08,
		EC_N163 = 0x10,
		EC_S5B = 0x20,
	};

	enum Channel2A03 {
		Square1 = 0,
		Square2,
		Triangle,
		Noise,
		DMC,
	};
	int channelCount;
	int expansionColSize;
	expansionChips expansion;
	int split;

	int n163chanells;

	std::list<DPCMSample* > sampleList;

	int patternLength;
	int trackSpeed;
	int trackTempo;
	std::vector<int> columnSize;
	std::list<std::vector<int>> patternOrder;
	std::list<FTMPattern* > PatternList;
	std::vector<FTMInstrument* > instrumentList;
};

