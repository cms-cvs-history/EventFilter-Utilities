/*
 * JSONHistoCollector.h
 *
 * Files merged by this class do not have a class model, work is done directly on the files.
 *
 *  Created on: Nov 21, 2012
 *      Author: aspataru
 */

#ifndef JSONHISTOCOLLECTOR_H_
#define JSONHISTOCOLLECTOR_H_

#include "EventFilter/Utilities/src/json.h"
#include "DataPointDefinition.h"
#include "HistoDataPoint.h"
#include <string>

namespace jsoncollector {

class JSONHistoCollector {

public:
	/**
	 * Collects .fast csv files from directory and converts to HistoDataPoint objects, with or without per-file history of states
	 */
	static int fastToH(std::string& inputDir,
			std::vector<HistoDataPoint*>& outHistoData,
			bool includePerFileHistory, unsigned int N_M, unsigned int N_m,
			unsigned int N_u);

	static HistoDataPoint* onelineCSVToHisto(std::string& olCSV,
			DataPointDefinition* dpd, unsigned int N_M, unsigned int N_m,
			unsigned int N_u);

	static int fastHistoMerge(std::string& inputDirPath,
			std::string& outputFilePath, unsigned int N_M, unsigned int N_m,
			unsigned int N_u);

	static int fullHistoMerge(std::string& inputDirPath,
			std::string& outputFilePath, unsigned int N_M, unsigned int N_m,
			unsigned int N_u);

	/**
	 * Collects .fast csv files from directory and converts to HistoDataPoint files
	 * TODO remove old
	 */
	static int convertFastToHisto(std::string& inputDir, unsigned int N_M,
			unsigned int N_m, unsigned int N_u);

	/**
	 * Collects JSON histo files and creates a JSON histogram of contents
	 * * TODO remove old
	 */
	static int mergeHistoFiles(std::string& inputDirPath,
			std::string& outputFilePath, unsigned int N_M, unsigned int N_m,
			unsigned int N_u);

};
}

#endif /* JSONHISTOCOLLECTOR_H_ */
