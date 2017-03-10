#pragma once

#include "ftmtxt.h"

class OutputFile
{
public:
	OutputFile();
	virtual ~OutputFile();

	virtual void writeOutput(const char *filename) = 0;
	virtual void convertFTMtxt(const FTMtxt &ftmtxt) = 0;
};

