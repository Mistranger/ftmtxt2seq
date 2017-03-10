#pragma once

#include "outputfile.h"

class ITFile : public OutputFile
{
public:
	ITFile();
	virtual ~ITFile();

	std::string songName;
	uint16_t orderNum;
	uint16_t insNum;
	uint16_t smpNum;
	uint16_t patNum;

	uint8_t gVolume;
	uint8_t mVolume;
	uint8_t speed;
	uint8_t tempo;
	uint8_t panSep;

	std::string songMessage;
	std::vector<int> orders;

	virtual void writeOutput(const char *filename);
	virtual void convertFTMtxt(const FTMtxt &ftmtxt);
private:
	void writeITHeader(std::ofstream &oFile);
	void generateNoiseSamples();
	void generate2A03Instruments();

	struct ITChannel {
		uint8_t pan;
		uint8_t volume;
	};

	class ITInstrument {
	public:
		ITInstrument();
		static int getFileStructSize() {
			return 0x22A;
		}
		void writeToITFile(std::ofstream &oFile);
		char name[12];
		uint8_t nna; // New Note Action
		uint8_t dct; // Duplicate Check Type
		uint8_t dca; // Duplicate Check Action
		uint16_t fadeout;
		int8_t pps; // Pitch-Pan separation
		int8_t ppc; // Pitch-Pan center
		uint8_t gbv; // Global Volume
		uint8_t dfp; // Default Pan
		uint8_t rv; // Random volume variation
		uint8_t rp; // Random panning variation
		uint16_t trkVers; // Tracker version
		uint8_t nos; // Number of samples
		char instrumentName[26];
		uint8_t ifc; // Initial Filter cutoff
		uint8_t ifr; // Initial Filter resonance
		uint8_t mch; // MIDI Channel
		uint8_t mpr; // MIDI Program
		uint16_t midibnk;

		uint16_t noteSample[120];
		struct envelope {
			uint8_t flags;
			uint8_t nodes;
			uint8_t lpB;
			uint8_t lpE;
			uint8_t slb;
			uint8_t sle;
			uint8_t nodep[75];
		} volEnv, panEnv, pitchEnv;
	};

	class ITSample {
	public:
		ITSample();

		static int getFileStructSize() {
			return 0x50;
		}
		void convertDPCMSample(const FTMtxt::DPCMSample &sample);
		void writeToITFile(std::ofstream &oFile, uint32_t *offset) const;
		char name[12];
		uint8_t gVolume;
		uint8_t flags;
		uint8_t volume;
		char sampleName[26];
		uint8_t cvt;
		uint8_t dfp;
		uint32_t length;
		uint32_t loopBegin;
		uint32_t loopEnd;
		uint32_t c5Speed;
		uint32_t susloopBegin;
		uint32_t susloopEnd;
		uint32_t pointer;
		uint8_t vis;
		uint8_t vid;
		uint8_t vir;
		uint8_t vit;

		std::vector<int8_t> samples;
		
	};

	class ITPattern {
	public:
		ITPattern();
		int getFileStructSize() const;
		uint16_t length;
		uint16_t rows;
		uint8_t *data;

		void convertFTMpattern(const FTMPattern &pattern, const FTMtxt &txt);
		void writeToITFile(std::ofstream &oFile) const;
	};

	ITChannel channels[64];
	std::list<ITInstrument *> instruments;
	std::list<ITSample *> samples;
	std::list<ITPattern *> patterns;
};

