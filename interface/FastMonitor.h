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
#include "HistoDataPoint.h"

namespace jsoncollector {

class FastMonitor {

public:
	FastMonitor(std::vector<JsonMonitorable*> monitorableVariables,
			std::string defPath, std::string fastOutPath,
			std::string fullOutPath, unsigned int N_M, unsigned int N_m,
			unsigned int N_u);
	virtual ~FastMonitor();

	// updates internal HistoDataPoint and prints one-line CSV
	void snap();

	// outputs the contents of the internal histoDataPoint, at the end of lumi
	void outputFullHistoDataPoint(int outputFileSuffix);

private:
	JsonMonitorable* getVarForName(string name) const;

	bool snappedOnce_;
	DataPointDefinition* dpd_;
	std::vector<JsonMonitorable*> monitorableVars_;
	std::string fastOutPath_, fullOutPath_;
	std::vector<JsonMonitorable*> monitoredVars_;
	std::vector<string> accumulatedCSV_;
	unsigned int N_MACRO, N_MINI, N_MICRO;
};

}

#endif /* FASTMONITOR_H_ */
