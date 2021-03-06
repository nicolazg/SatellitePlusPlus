#include "NaiveResolver.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <numeric>
#include <chrono>
#include <thread> 

NaiveResolver::NaiveResolver(SimulationData * simDat, std::string filename) {
	simData = simDat;
	outFilename = filename;
	initialData = new Satelite[simData->getNbSatelite()];
	memcpy(initialData, simData->getArraySat(), sizeof(Satelite)*simData->getNbSatelite());
}



/*
Check if a picture can be taken at the given turn for the given collection.
*/
bool isInTimeStamp(int turn, Collection coll) {

	for (int i = 0; i < coll.nbTimeSt; i++)
	{
		TimeStamp ts = coll.listTimeSt[i];
		if (turn >= ts.minTime && turn <= ts.maxTime) {
			return true;
		}
	}

	return false;
}

/*
Check a possible conflict if the satelite take the given image at the given turn.
*/
bool isConflict(Satelite * sat, Image im, int turn) {
	// Get the position of the previous picture taken by the satelite, relative to it.
	Image * a = sat->lastShotRelativePosition;
	
	// Compute the relative position of the input image
	Image b;
	b.la = sat->la - im.la;
	b.lo = sat->lo - im.lo;
	
	// If there is no previous image taken, then the relative position is (0,0)
	if (a == NULL) {
		a = new Image;
		a->la = 0;
		a->lo = 0;
		// We return the fact that there is a conflict or not.
		return std::sqrt(std::pow(b.la - a->la, 2.0) + std::pow(b.lo - a->lo, 2.0)) > (sat->speedRot * (turn - sat->lastShotTurn));
	}
	else {
		// We return the fact that there is a conflict or not.
		return std::sqrt(std::pow(b.la - a->la, 2.0) + std::pow(b.lo - a->lo, 2.0)) > (sat->speedRot * (turn - sat->lastShotTurn));
	}	
}

void NaiveResolver::resetTakenPictures() {
	int colNb = simData->getNbCollection();

	for (int i = 0; i < colNb; i++)
	{
		Collection * coll = &simData->getArrayCol()[i];

		if (coll->isValid) {
			int picNb = coll->nbImg;

			for (int j = 0; j < picNb; j++)
			{
				coll->listImg[j].taken = false;
			}
		}
	}

}

bool NaiveResolver::checkUncompleteCollections() {

	int colNb = simData->getNbCollection();

	bool hasChanged = false;

	for (int i = 0; i < colNb; i++)
	{
		Collection * coll = &simData->getArrayCol()[i];

		if (coll->isValid) {
			int picNb = coll->nbImg;

			bool valid = true;

			for (int j = 0; j < picNb; j++)
			{
				if (!coll->listImg[j].taken) {
					valid = false;
					hasChanged = true;
					break;
				}
			}

			coll->isValid = valid;
		}
	}

	return hasChanged;

}

void NaiveResolver::threadResolv(int i, int n ,bool verbose,std::string * result) {
	int maxTurns = simData->getDuration();
	int satNb = simData->getNbSatelite();
	int colNb = simData->getNbCollection();

	// For each turn of the simulation...
	for (size_t x = i; x < satNb; x = x + n)
	{
		auto start = std::chrono::high_resolution_clock::now();
		Satelite * sat = &simData->getArraySat()[x];

		// For each satelite...
		for (int turn = 0; turn < maxTurns; turn++) {
			// We get the current satelite, for better understanding
			// ... for each existing collection ...
			for (int j = 0; j < colNb; j++) {
				
				// We get the current collection for better understanding
				Collection coll = simData->getArrayCol()[j];
				
				if (coll.isValid) {
					
					// Is the right time to take a picture in this collection ?
					if (isInTimeStamp(turn, coll)) {
						// Yes, so we can iterate through all its images.
						for (int k = 0; k < coll.nbImg; k++) {
							// If the image can be shot, and there is no conflict, then we take the picture.
							if (isInRange(sat, &coll.listImg[k]) && !isConflict(sat, coll.listImg[k], turn) && !coll.listImg[k].taken) {

								// We set the last shot of the satelite to be this picture, at this turn.
								coll.listImg[k].taken = true;
								Image tmp_im = coll.listImg[k];
								tmp_im.la = sat->la - coll.listImg[k].la;
								tmp_im.lo = sat->lo - coll.listImg[k].lo;
								sat->lastShotRelativePosition = &tmp_im;
								sat->lastShotTurn = turn;

								// And we save the result in our result string.
								result[i] += std::to_string(coll.listImg[k].la) + " ";
								result[i] += std::to_string(coll.listImg[k].lo) + " ";
								result[i] += "" + std::to_string(turn);
								result[i] += " " + std::to_string(x);
								result[i] += '\n';
								// we don't forget to increment the number of pictures taken, isn't it ?
								nbPict++;

								
							}
						}
					}
				}
			}
			// And finally, we move the satelite.
			moveSatelite(sat);

		}

		if (verbose) {
			auto end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> diff = end - start;
			std::cout << " satelite  " << x << " thread no " << i << "/" << n << " time elapsed " << diff.count() << " s\n";
			std::cout << "[I] Number of picture taken : " << nbPict << std::endl;
		}

	}

}

void NaiveResolver::launchResolution(bool verbose) {
	// Get data for better reading.
	int satNb = simData->getNbSatelite();
	int tmp = 4;
	std::thread *t = new std::thread[tmp];
	NaiveResolver * th = this;

	

	if (verbose) {
		std::cout << "[I] Start simulation ..." << std::endl;
		std::cout << "\r[I] Complete : 0%";
	}

	// And we launch the simulation.

	bool redoSimulation = true;
	int pass = 1;

	std::string * result = new std::string[tmp];

	//while (redoSimulation) {

		if (pass > 1) {
			resetTakenPictures();
		}

		// Initialize the variables which stores the resulting data
		result = new std::string[tmp];

		nbPict = 0;

		//if (verbose)
			std::cout << "[I] Pass nb " << pass << std::endl;

		for (int i = 0; i < tmp; i++) {
			result[i] = std::string();
			t[i] = std::thread([&th, i, tmp, result, verbose]() { th->threadResolv(i, tmp, verbose, result); });
		}
		for (size_t i = 0; i < tmp; i++)
		{
			t[i].join();
		}

		redoSimulation = checkUncompleteCollections();
		memcpy(simData->getArraySat(), initialData, sizeof(Satelite)*simData->getNbSatelite());

		pass++;
	//}
	
	// When the simulation is over, we write in the output file.
	std::ofstream myfile;
	myfile.open(outFilename);
	myfile << std::to_string(nbPict) << std::endl;
	for (size_t i = 0; i < tmp; i++)
		myfile << result[i];

	myfile.close();
	

}