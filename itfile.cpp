#include "itfile.h"
#include "util.h"

#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

ITFile::ITFile(): orderNum(0), insNum(0), smpNum(0), patNum(0), 
gVolume(0x40), mVolume(0x40), speed(0), tempo(0), panSep(0)
{
	for (size_t i = 0; i < 64; ++i) {
		this->channels[i].pan = 32;
		this->channels[i].volume = 64;
	}
}


ITFile::~ITFile()
{
}

void ITFile::writeOutput(const char *filename)
{
	std::ofstream oFile(filename, std::ios::out | std::ios::binary);
	writeITHeader(oFile);

	uint32_t fileOffset = 0x00C0 + this->orderNum + 4*this->insNum + 4*this->smpNum + 4*this->patNum + this->songMessage.length() + 1;
	for (auto it = this->instruments.begin(); it != this->instruments.end(); ++it) {
		Utility::writeLE32(oFile, fileOffset);
		fileOffset += ITInstrument::getFileStructSize();
	}
	for (auto it = this->samples.begin(); it != this->samples.end(); ++it) {
		Utility::writeLE32(oFile, fileOffset);
		fileOffset += ITSample::getFileStructSize();
	}
	for (auto it = this->patterns.begin(); it != this->patterns.end(); ++it) {
		Utility::writeLE32(oFile, fileOffset);
		fileOffset += (*it)->getFileStructSize();
	}

	oFile << this->songMessage.c_str();
	oFile.put('\0');

	for (auto it = this->instruments.begin(); it != this->instruments.end(); ++it) {
		ITInstrument &ins = **it;
		ins.writeToITFile(oFile);
	}
	for (auto it = this->samples.begin(); it != this->samples.end(); ++it) {
		ITSample &smp = **it;
		smp.writeToITFile(oFile, &fileOffset);
		fileOffset += smp.length * (smp.flags & 0x02 ? 2 : 1);
	}
	for (auto it = this->patterns.begin(); it != this->patterns.end(); ++it) {
		ITPattern &pat = **it;
		pat.writeToITFile(oFile);
	}

	for (auto it = this->samples.begin(); it != this->samples.end(); ++it) {
		ITSample &smp = **it;
		for (size_t i = 0; i < smp.length; ++i) {
			oFile.put(smp.samples[i * ((smp.flags & 0x02) ? 2 : 1)]);
			if (smp.flags & 0x02) {
				oFile.put(smp.samples[2*i + 1]);
			}
		}
	}

	oFile.close();
}

void ITFile::generate2A03Instruments()
{
	for (int i = 0; i < 5; ++i) {
		ITSample *sample = new ITSample();
		sample->flags = 0x01 + 0x10;
		if (i == 0) {
			strncpy(sample->sampleName, "Square wave 12.5 duty", 26);
		} else if (i == 1) {
			strncpy(sample->sampleName, "Square wave 25 duty", 26);
		} else if (i == 2) {
			strncpy(sample->sampleName, "Square wave 50 duty", 26);
		} else if (i == 3) {
			strncpy(sample->sampleName, "Square wave 25 neg duty", 26);
		} else if (i == 4) {
			strncpy(sample->sampleName, "Triangle wave", 26);
		}
		
		const int period = 256;
		sample->length = period;
		sample->loopEnd = period;
		sample->c5Speed = period * 1046 / (i == 4 ? 2 : 1);
		int c = 0;
		for (size_t j = 0 ; j < sample->length; ++j, c = (c == period - 1 ? 0 : c + 1)) {
			if (i == 0) { // 12.5%
				if ((c / (period / 8)) == 1) {
					sample->samples.push_back(0x7F);
				} else {
					sample->samples.push_back(0x80);
				}
			} else if (i == 1) { // 25%
				if ((c/ (period / 8)) == 1 || (c/ (period / 8)) == 2) {
					sample->samples.push_back(0x7F);
				} else {
					sample->samples.push_back(0x80);
				}
			} else if (i == 2) { // 50%
				if ((c/ (period / 8)) >= 1 && (c/ (period / 8)) <= 4) {
					sample->samples.push_back(0x7F);
				} else {
					sample->samples.push_back(0x80);
				}
			} else if (i == 3) { // 25% neg
				if ((c/ (period / 8)) == 1 || (c/ (period / 8)) == 2) {
					sample->samples.push_back(0x80);
				} else {
					sample->samples.push_back(0x7F);
				}
			} else if (i == 4) { // triangle
				const int triTable[] = {15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
					0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15};
				const int val = triTable[c / (period / 32)];
				sample->samples.push_back((int8_t)(val * 17 - 0x80));
			} else if (i == 5) { // noise 93 bit

			}
		}
		this->samples.push_back(sample);
	}
	this->generateNoiseSamples();
}

void ITFile::generateNoiseSamples()
{
	const int noisePeriod[] = {0x004, 0x008, 0x010, 0x020, 0x040, 0x060, 0x080, 0x0A0, 
		0x0CA, 0x0FE, 0x17C, 0x1FC, 0x2FA, 0x3F8, 0x7F2, 0xFE4};
	uint16_t shiftReg = 1;
	for (int i = 0; i < 16; ++i) {
		ITSample *sample = new ITSample();
		sample->flags = 0x01 + 0x10;
		const int period = 441000;
		sample->c5Speed = 44100;
		int c = 0;
		int8_t smp = 0;
		for (size_t j = 0 ; j < period; ++j, ++c) {
			if (c % noisePeriod[15] == 0) {
				bool b1 = shiftReg & 1;
				bool b2 = (shiftReg >> 1) & 1;
				shiftReg >>=  1;
				shiftReg |= (b1 ^ b2) << 14;
			}
			if (shiftReg & 1) {
				sample->samples.push_back(0x80);
				++sample->length;
				++sample->loopEnd;
			} else {
				sample->samples.push_back(0x7F);
				++sample->length;
				++sample->loopEnd;
			}
		}
		this->samples.push_back(sample);
	}
}

void ITFile::writeITHeader(std::ofstream &oFile)
{
	oFile << 'I' << 'M' << 'P' << 'M';
	char sname[26];

	strncpy(sname, this->songName.c_str(), 26);
	sname[25] = '\0';
	oFile.write(sname, 26);
	oFile.put('\0').put('\0'); // PHiligt
	Utility::writeLE16(oFile, this->orderNum);
	Utility::writeLE16(oFile, this->insNum);
	Utility::writeLE16(oFile, this->smpNum);
	Utility::writeLE16(oFile, this->patNum);
	oFile.put(0x0E).put(0x02); // Cwt
	oFile.put(0x0E).put(0x02); // Cmwt
	uint16_t flags = 0x0F;
	Utility::writeLE16(oFile, flags);
	uint16_t special = 0x01;
	Utility::writeLE16(oFile, special);
	oFile.put(this->gVolume);
	oFile.put(this->mVolume);
	oFile.put(this->speed);
	oFile.put(this->tempo);
	oFile.put(this->panSep);
	oFile.put(0x00); // PWD
	Utility::writeLE16(oFile, this->songMessage.length() + 1);
	uint32_t messageOffset = 0x00C0 + this->orderNum + 4 * this->insNum + 4 * this->smpNum + 4 * this->patNum;
	Utility::writeLE32(oFile, messageOffset);
	oFile.put(0x00).put(0x00).put(0x00).put(0x00); // Reserved

	for (size_t i = 0; i < 64; ++i) {
		oFile.put(this->channels[i].pan);
	}
	for (size_t i = 0; i < 64; ++i) {
		oFile.put(this->channels[i].volume);
	}

	for (size_t i = 0; i < orderNum; ++i) {
		oFile.put(this->orders[i]);
	}
}

void ITFile::convertFTMtxt(const FTMtxt &ftmtxt)
{
	this->instruments.clear();
	this->samples.clear();
	this->patterns.clear();

	this->speed = ftmtxt.trackSpeed;
	this->tempo = ftmtxt.trackTempo;
	this->songName = ftmtxt.songTitle;
	this->songMessage = "Converted by "; // fixme

	this->generate2A03Instruments();
	this->orderNum = ftmtxt.patternOrder.size();
	this->insNum = 0; // fixme
	this->smpNum = ftmtxt.sampleList.size() + 5 + 16;
	this->patNum = ftmtxt.PatternList.size();

	int c = 0;
	for (auto it = ftmtxt.patternOrder.begin(); it != ftmtxt.patternOrder.end(); ++it) {
		this->orders.push_back(c++);
	}

	for (auto it = ftmtxt.PatternList.begin(); it != ftmtxt.PatternList.end(); ++it) {
		FTMPattern &pat = **it;

		ITPattern *itpat = new ITPattern();
		itpat->convertFTMpattern(pat, ftmtxt);
		this->patterns.push_back(itpat);
	}

	for (auto it = ftmtxt.sampleList.begin(); it != ftmtxt.sampleList.end(); ++it) {
		FTMtxt::DPCMSample &sample = **it;

		ITSample *itsmp = new ITSample();
		itsmp->convertDPCMSample(sample);
		this->samples.push_back(itsmp);
	}
}

enum mask {
	M_None = 0x00,
	M_Note = 0x01,
	M_Instrument = 0x02,
	M_Volume = 0x04,
	M_Effect = 0x08,
	M_LastNote = 0x10,
	M_LastInstr = 0x20,
	M_LastVolume = 0x40,
	M_LastEffect = 0x80,
};

inline mask operator|(mask a, mask b)
{
	return static_cast<mask>(static_cast<int>(a) | static_cast<int>(b));
}

inline mask operator&(mask a, mask b)
{
	return static_cast<mask>(static_cast<int>(a) & static_cast<int>(b));
}

ITFile::ITPattern::ITPattern(): length(0), rows(0), data(nullptr)
{
}

int ITFile::ITPattern::getFileStructSize() const
{
	return length + 8;
}

void ITFile::ITPattern::convertFTMpattern(const FTMPattern &pattern, const FTMtxt &txt)
{
	this->rows = pattern.size();
	uint8_t channel;
	uint8_t channelVar = 0;
	mask maskVar[64];
	mask maskVarPrev[64];
	memset(maskVar, M_None, 64 * sizeof(mask));
	memset(maskVarPrev, M_None, 64 * sizeof(mask));

	int channelCount = (**pattern.begin()).size();
	int *note = new int[channelCount];
	for (int i = 0; i < channelCount; ++i) {
		note[i] = FTMtxt::noteNo;
	}
	int *instrument = new int[channelCount];
	for (int i = 0; i < channelCount; ++i) {
		instrument[i] = -1;
	}
	int *volume = new int[channelCount];
	for (int i = 0; i < channelCount; ++i) {
		volume[i] = -1;
	}
	int *effect = new int[channelCount];
	for (int i = 0; i < channelCount; ++i) {
		effect[i] = 0;
	}
	int *effectdata = new int[channelCount];
	for (int i = 0; i < channelCount; ++i) {
		effectdata[i] = 0;
	}

	int *portamento = new int[channelCount];
	for (int i = 0; i < channelCount; ++i) {
		portamento[i] = 0x80;
	}

	std::vector<uint8_t> packedStream;
	for (auto it = pattern.begin(); it != pattern.end(); ++it) {
		FTMPatternRow &row = **it;
		channel = 0;
		for (auto it2 = row.begin(); it2 != row.end(); ++it2) {
			channelVar = ++channel;
			FTMPatternRowEntry &entry = **it2;

			if (entry.note != FTMtxt::noteNo) {
				if (entry.note == note[channel - 1]) {
				} else {
					maskVar[channel - 1] = static_cast<mask>(maskVar[channel - 1] | M_Note);
					note[channel - 1] = entry.note;
				}
			} else {
				maskVar[channel - 1] = static_cast<mask>(maskVar[channel - 1] & ~M_Note);
			}
			if (entry.instrument != -1) {
				maskVar[channel - 1] = static_cast<mask>(maskVar[channel - 1] | M_Instrument);
				instrument[channel - 1] = entry.instrument;
			} else {
				maskVar[channel - 1] = static_cast<mask>(maskVar[channel - 1] & ~M_Instrument);
			}
			if (entry.volume != -1) {
				maskVar[channel - 1] = static_cast<mask>(maskVar[channel - 1] | M_Volume);
				volume[channel - 1] = entry.volume * 4;
			} else {
				maskVar[channel - 1] = static_cast<mask>(maskVar[channel - 1] & ~M_Volume);
			}
			if (entry.effect1 != 0) {
				
				int itEffect = 0;
				int itData = 0;
				if (entry.effect1 == 'P' ) {
					if (entry.effect1data > portamento[channel - 1]) {
						itEffect = 'F' - 'A' + 1;
					} else if (entry.effect1data < portamento[channel - 1]) { 
						itEffect = 'E' - 'A' + 1;
					}
					itData = entry.effect1data - portamento[channel - 1];
					portamento[channel - 1] += itData;
					itData = 0xE0 + (abs(itData) & 0x0F);
				}
				maskVar[channel - 1] = static_cast<mask>(maskVar[channel - 1] | M_Effect);
				effect[channel - 1] = itEffect;
				effectdata[channel - 1] = itData;
				
			} else {
				maskVar[channel - 1] = static_cast<mask>(maskVar[channel - 1] & ~M_Effect);
			}

			// special square duty handling
			if (entry.effect2 == 'V' && (channel == 1 || channel == 2)) { // square duty
				maskVar[channel - 1] = static_cast<mask>(maskVar[channel - 1] | M_Instrument);
				instrument[channel - 1] = entry.effect2data + 1; // fixme
			}

			if (maskVar[channel - 1] != maskVarPrev[channel - 1]) {
				channelVar += 0x80;
				packedStream.push_back(channelVar);
				packedStream.push_back(maskVar[channel - 1]);
				maskVarPrev[channel - 1] = maskVar[channel - 1];
			} else {
				packedStream.push_back(channelVar);
			}
			if (maskVar[channel - 1] & M_Note) {
				packedStream.push_back((uint8_t)note[channel - 1]);
			}
			if (maskVar[channel - 1] & M_Instrument) {
				packedStream.push_back((uint8_t)instrument[channel - 1]);
			}
			if (maskVar[channel - 1] & M_Volume) {
				packedStream.push_back((uint8_t)volume[channel - 1]);
			}
			if (maskVar[channel - 1] & M_Effect) {
				packedStream.push_back((uint8_t)effect[channel - 1]);
				packedStream.push_back((uint8_t)effectdata[channel - 1]);
			}		
		}
		packedStream.push_back(0); // end of row
	}

	this->length = packedStream.size();
	this->data = new uint8_t[this->length];
	for (int i = 0; i < this->length; ++i) {
		data[i] = packedStream[i];
	}

	delete[] note;
	delete[] instrument;
	delete[] volume;
	delete[] effect;
	delete[] effectdata;
}

void ITFile::ITPattern::writeToITFile(std::ofstream &oFile) const
{
	Utility::writeLE16(oFile, this->length);
	Utility::writeLE16(oFile, this->rows);
	oFile.put(0x00).put(0x00).put(0x00).put(0x00);
	for (size_t i = 0; i < this->length; ++i) {
		oFile.put(this->data[i]);
	}
}

ITFile::ITInstrument::ITInstrument():
	nna(0), dct(0), dca(0), fadeout(0), pps(0), ppc(0), 
	gbv(0), dfp(0), rv(0), rp(0), trkVers(0), nos(0), 
	ifc(0), ifr(0), mch(0), mpr(0), midibnk(0)
{
	memset(this->name, 0, 12);
	memset(this->instrumentName, 0, 26);
}

void ITFile::ITInstrument::writeToITFile(std::ofstream &oFile)
{
	oFile << 'I' << 'M' << 'P' << 'I';
	oFile.write(this->name, 12);
	oFile.put(0x00);
	oFile.put(this->nna);
	oFile.put(this->dct);
	oFile.put(this->dca);
	Utility::writeLE16(oFile, this->fadeout);
	oFile.put(this->pps);
	oFile.put(this->ppc);
	oFile.put(this->gbv);
	oFile.put(this->dfp);
	oFile.put(this->rv);
	oFile.put(this->rp);
	oFile.put(0x0E).put(0x02);
	oFile.put(this->nos);
	oFile.put(0x00);
	oFile.write(this->instrumentName, 26);
	oFile.put(this->ifc);
	oFile.put(this->ifr);
	oFile.put(this->mch);
	oFile.put(this->mpr);
	oFile.put(0x00).put(0x00);
	for (size_t i = 0; i < 120; ++i) {
		Utility::writeLE16(oFile, this->noteSample[i]);
	}
	oFile.put(this->volEnv.flags);
	oFile.put(this->volEnv.nodes);
	oFile.put(this->volEnv.lpB);
	oFile.put(this->volEnv.lpE);
	oFile.put(this->volEnv.slb);
	oFile.put(this->volEnv.sle);
	for (size_t i = 0; i < 75; ++i) {
		oFile.put(this->volEnv.nodep[i]);
	}
	oFile.put(0x00);
	oFile.put(this->panEnv.flags);
	oFile.put(this->panEnv.nodes);
	oFile.put(this->panEnv.lpB);
	oFile.put(this->panEnv.lpE);
	oFile.put(this->panEnv.slb);
	oFile.put(this->panEnv.sle);
	for (size_t i = 0; i < 75; ++i) {
		oFile.put(this->panEnv.nodep[i]);
	}
	oFile.put(0x00);
	oFile.put(this->pitchEnv.flags);
	oFile.put(this->pitchEnv.nodes);
	oFile.put(this->pitchEnv.lpB);
	oFile.put(this->pitchEnv.lpE);
	oFile.put(this->pitchEnv.slb);
	oFile.put(this->pitchEnv.sle);
	for (size_t i = 0; i < 75; ++i) {
		oFile.put(this->pitchEnv.nodep[i]);
	}
	oFile.put(0x00).put(0x00).put(0x00).put(0x00).put(0x00);
}

ITFile::ITSample::ITSample():
	gVolume(0x40), flags(0), volume(0x40), cvt(0x01), dfp(0), length(0), loopBegin(0),
	loopEnd(0), c5Speed(0), susloopBegin(0), susloopEnd(0), pointer(0), vis(0), 
	vid(0), vir(0), vit(0)
{
	memset(this->name, 0, 12);
	memset(this->sampleName, 0, 26);
}

void ITFile::ITSample::convertDPCMSample(const FTMtxt::DPCMSample &sample)
{
	int currentSample = 0;
	this->samples.clear();
	for (size_t i = 0; i < sample.length; ++i) {
		for (int j = 7; j >= 0; --j) {
			currentSample += ((sample.sample[i] >> j) & 1) ? 1 : -1;
			if (currentSample > 0x3F) {
				currentSample = 0x3F;
			} else if (currentSample < -0x40F) {
				currentSample = -0x40F;
			}
			for (int k = 0; k < 50; ++k) {
				samples.push_back(static_cast<int8_t>(currentSample * 2));
			}
			
		}
	}
	this->length = sample.length * 8 * 50;
	this->flags = 0x01;
	this->cvt = 0x01;
	this->gVolume = 0x40;
	this->volume = 0x40;
	this->c5Speed = 4181;

}

void ITFile::ITSample::writeToITFile(std::ofstream &oFile, uint32_t *offset) const
{
	oFile << 'I' << 'M' << 'P' << 'S';
	oFile.write(this->name, 12);
	oFile.put(0x00);
	oFile.put(this->gVolume);
	oFile.put(this->flags);
	oFile.put(this->volume);
	oFile.write(this->sampleName, 26);
	oFile.put(this->cvt);
	oFile.put(this->dfp);

	Utility::writeLE32(oFile, this->length);
	Utility::writeLE32(oFile, this->loopBegin);
	Utility::writeLE32(oFile, this->loopEnd);
	Utility::writeLE32(oFile, this->c5Speed);
	Utility::writeLE32(oFile, this->susloopBegin);
	Utility::writeLE32(oFile, this->susloopEnd);
	Utility::writeLE32(oFile, *offset);
	oFile.put(this->vis);
	oFile.put(this->vid);
	oFile.put(this->vir);
	oFile.put(this->vit);
}
