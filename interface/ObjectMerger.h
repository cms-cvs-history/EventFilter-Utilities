/*
 * ObjectMerger.h
 *
 *  Created on: Sep 24, 2012
 *      Author: aspataru
 */

#ifndef OBJECTMERGER_H_
#define OBJECTMERGER_H_

#include <vector>
#include "DataPoint.h"
#include "HistoDataPoint.h"
#include "DataPointDefinition.h"

namespace jsoncollector {
class ObjectMerger {

public:
	/**
	 * Merges the DataPoint objects in the vector by getting their definition and applying the required operations
	 */
	static DataPoint* merge(std::vector<DataPoint*> objectsToMerge,
			std::string& outcomeMessage);
	/**
	 * Merges the HistoDataPoint objects in the vector by getting their definition and applying the required operations
	 */
	static HistoDataPoint* merge(std::vector<HistoDataPoint*>& objectsToMerge,
			std::string& outcomeMessage, unsigned int N_M, unsigned int N_m,
			unsigned int N_u);
	/**
	 * TODO comment
	 */
	static HistoDataPoint* mergeHistosButKeepLatestCounters(
			std::vector<HistoDataPoint*> objectsToMerge,
			std::string& outcomeMessage, unsigned int N_M, unsigned int N_m,
			unsigned int N_u);

	/**
	 * Returns a pointer to a new DataPointDefinition object loaded from the specified path
	 */
	// FIXME return void, no *new* object
	static DataPointDefinition* getDataPointDefinitionFor(
			std::string defFilePath);

private:
	static std::string applyOperation(std::vector<std::string> dataVector,
			std::string operationName);
	static bool checkConsistency(std::vector<DataPoint*> objectsToMerge,
			std::string& outcomeMessage);

};
}

#endif /* OBJECTMERGER_H_ */
