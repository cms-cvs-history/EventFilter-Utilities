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

namespace jsoncollector {
class JSONFileCollector {
public:
	/**
	 * Collects input from the specified JSON file paths and creates an output JSON file at the specified output path
	 */
	static int mergeFiles(std::vector<std::string>& inputJSONFilePaths,
			std::string& outputFilePath, bool formatForDisplay);
};
}

#endif /* JSONFILECOLLECTOR_H_ */
