/*
 * FastMonitor.h
 *
 *  Created on: Nov 27, 2012
 *      Author: aspataru
 */

#ifndef FASTMONITOR_H_
#define FASTMONITOR_H_

#include "JsonMonitorable.h"
#include "DataPointDefinition.h"
#include "DataPoint.h"

namespace jsoncollector {

class FastMonitor {

public:
	FastMonitor(std::vector<JsonMonitorable*> monitorableVariables,
			std::string defPath, std::string fastOutPath,
			std::string fullOutPath);
	virtual ~FastMonitor();

	// updates internal HistoDataPoint and prints one-line CSV
	void snap();

	// outputs the contents of the internal histoDataPoint, at the end of lumi
	void outputFullHistoDataPoint(int outputFileSuffix);

private:
	JsonMonitorable* getVarForName(string name) const;

	bool snappedOnce_;
	DataPointDefinition dpd_;
	std::vector<JsonMonitorable*> monitorableVars_;
	std::string fastOutPath_, fullOutPath_;
	std::vector<JsonMonitorable*> monitoredVars_;
	std::vector<string> accumulatedCSV_;
	std::string defPath_;

	static const std::string OUTPUT_FILE_FORMAT;
};

}

#endif /* FASTMONITOR_H_ */
