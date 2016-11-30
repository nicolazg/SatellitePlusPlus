#pragma once
#include "SimulationData.h"
#include <string>
#include <fstream>
#include "StructData.h"

class DataReceiver
{
public:
	DataReceiver(std::string filename);
	~DataReceiver();
	SimulationData extractData();
	Satelite * extractSatelite(SimulationData * Sd);
	Collection * extractCollection(SimulationData * Sd);
	Image * extractImage(int nb);
	TimeStamp * extractTimeStamp(int nb);

private:
	std::ifstream * infile;

};
