/*
 * FastMonitor.cc
 *
 *  Created on: Nov 27, 2012
 *      Author: aspataru
 */

#include "../interface/FastMonitor.h"
#include "../interface/ObjectMerger.h"
#include "../interface/JSONSerializer.h"
#include "../interface/FileIO.h"
#include "../interface/Utils.h"
#include <fstream>
#include <iostream>
#include <sstream>

using namespace jsoncollector;
using std::string;
using std::vector;
using std::ofstream;
using std::fstream;
using std::endl;

const string FastMonitor::OUTPUT_FILE_FORMAT = ".jsn";

FastMonitor::FastMonitor(vector<JsonMonitorable*> monitorableVariables,
		string defPath, string fastOutPath, string fullOutPath) :
	snappedOnce_(false), monitorableVars_(monitorableVariables),
			fastOutPath_(fastOutPath), fullOutPath_(fullOutPath),
			defPath_(defPath) {

	ObjectMerger::getDataPointDefinitionFor(defPath_, dpd_);
	if (dpd_.isPopulated()) {
		for (unsigned int i = 0; i < dpd_.getLegend().size(); i++) {
			string toBeMonitored = dpd_.getLegend()[i].getName();
			monitoredVars_.push_back(getVarForName(toBeMonitored));
		}
	}
}

FastMonitor::~FastMonitor() {
}

void FastMonitor::snap() {
	ofstream outputFile;
	std::stringstream ss;
	outputFile.open(fastOutPath_.c_str(), fstream::out | fstream::trunc);
	outputFile << defPath_ << endl;
	for (unsigned int i = 0; i < monitoredVars_.size(); i++) {
		if (i == monitoredVars_.size() - 1) {
			ss << monitoredVars_[i]->toString();
			break;
		}
		ss << monitoredVars_[i]->toString() << ",";
	}
	outputFile << ss.str();
	outputFile << endl;
	outputFile.close();

	string inputStringCSV = ss.str();

	accumulatedCSV_.push_back(inputStringCSV);
}

void FastMonitor::outputFullHistoDataPoint(int outputFileSuffix) {
	if (accumulatedCSV_.size() > 0) {

		vector<DataPoint*> dpToMerge;

		for (unsigned int i = 0; i < accumulatedCSV_.size(); i++) {
			string currentCSV = accumulatedCSV_[i];
			DataPoint* currentDP = ObjectMerger::csvToJson(currentCSV, &dpd_,
					defPath_);
			string hpid;
			Utils::getHostAndPID(hpid);
			currentDP->setSource(hpid);
			dpToMerge.push_back(currentDP);
		}

		string outputJSONAsString;
		string msg;

		DataPoint* mergedDP = ObjectMerger::merge(dpToMerge, msg, true);
		mergedDP->setSource(dpToMerge[0]->getSource());

		for (unsigned int i = 0; i < dpToMerge.size(); i++)
			delete dpToMerge[i];

		JSONSerializer::serialize(mergedDP, outputJSONAsString);
		stringstream ss;
		ss << fullOutPath_.substr(0, fullOutPath_.rfind(".")) << "_"
				<< outputFileSuffix << OUTPUT_FILE_FORMAT;
		string finalPath = ss.str();
		FileIO::writeStringToFile(finalPath, outputJSONAsString);

		accumulatedCSV_.clear();
		snappedOnce_ = false;

	}
}

JsonMonitorable* FastMonitor::getVarForName(string name) const {
	for (unsigned int i = 0; i < monitorableVars_.size(); i++)
		if (name.compare(monitorableVars_[i]->getName()) == 0)
			return monitorableVars_[i];
	return 0;
}
