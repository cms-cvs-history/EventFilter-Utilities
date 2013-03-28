/*
 * JSONFileCollector.h
 *
 *  Created on: Sep 25, 2012
 *      Author: aspataru
 */

#ifndef JSONFILECOLLECTOR_H_
#define JSONFILECOLLECTOR_H_

#include <vector>
#include <string>
#include "DataPoint.h"
#include "DataPointDefinition.h"

namespace jsoncollector {
class JSONFileCollector {
public:
	/**
	 * Collects input from the specified JSON file paths and creates an output JSON file at the specified output path
	 */
	static int mergeJSONFiles(std::vector<std::string>& inputJSONFilePaths,
			std::string& outputFilePath, bool formatForDisplay);

	/**
	 * Collects CSV fast files and creates a JSON histogram of contents
	 */
	static int mergeFastFiles(std::vector<std::string>& inputFiles,
			std::string& outputFilePath, bool formatForDisplay);

private:
	static bool displayFormat(DataPoint* dp, std::string& output);

};
}

#endif /* JSONFILECOLLECTOR_H_ */
