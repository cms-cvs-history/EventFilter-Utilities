/*
 * FastMonitor.cc
 *
 *  Created on: Nov 27, 2012
 *      Author: aspataru
 */

#include "../interface/FastMonitor.h"
#include "../interface/ObjectMerger.h"
#include "../interface/JSONHistoCollector.h"
#include "../interface/JSONSerializer.h"
#include "../interface/FileIO.h"
#include <fstream>
#include <iostream>
#include <sstream>

using namespace jsoncollector;
using std::string;
using std::vector;
using std::ofstream;
using std::fstream;
using std::endl;

FastMonitor::FastMonitor(vector<JsonMonitorable*> monitorableVariables,
		string defPath, string fastOutPath, string fullOutPath,
		unsigned int N_M, unsigned int N_m, unsigned int N_u) :
	snappedOnce_(false), monitorableVars_(monitorableVariables),
			fastOutPath_(fastOutPath), fullOutPath_(fullOutPath), N_MACRO(N_M),
			N_MINI(N_m), N_MICRO(N_u) {

	dpd_ = ObjectMerger::getDataPointDefinitionFor(defPath);
	if (dpd_->isPopulated()) {
		for (unsigned int i = 0; i < dpd_->getLegend().size(); i++) {
			string toBeMonitored = dpd_->getLegend()[i].getName();
			monitoredVars_.push_back(getVarForName(toBeMonitored));
		}
		/*
		 ofstream outputFile;
		 outputFile.open(fastOutPath_.c_str());
		 outputFile << dpd_->getName() << endl;
		 outputFile.close();
		 */
	}
}

FastMonitor::~FastMonitor() {
	if (dpd_ != 0)
		delete dpd_;
}

/*
 void FastMonitor::snap() {
 ofstream outputFile;
 std::stringstream ss;
 outputFile.open(fastOutPath_.c_str(), fstream::out | fstream::trunc);
 // FIXME not optimal
 outputFile << dpd_->getName() << endl;
 for (unsigned int i = 0; i < monitoredVars_.size(); i++) {
 if (i == monitoredVars_.size() - 1) {
 outputFile << monitoredVars_[i]->toString();
 ss << monitoredVars_[i]->toString();
 break;
 }
 outputFile << monitoredVars_[i]->toString() << ",";
 ss << monitoredVars_[i]->toString() << ",";
 }
 outputFile << endl;
 outputFile.close();

 string inputStringCSV = ss.str();

 HistoDataPoint* newHDP = new HistoDataPoint(N_MACRO, N_MINI, N_MICRO);
 JSONHistoCollector::onelineCSVToHisto(inputStringCSV, dpd_, newHDP, N_MACRO, N_MINI, N_MICRO);

 if (!snappedOnce_) {
 HistoDataPoint* deletable = hdp_;
 *hdp_ = *newHDP;
 delete newHDP;
 snappedOnce_ = true;
 }

 else {
 vector<HistoDataPoint*> histoDataPoints;
 histoDataPoints.push_back(hdp_);
 histoDataPoints.push_back(newHDP);
 string outcomeMessage;
 HistoDataPoint* mergedHDP = ObjectMerger::mergeHistosButKeepLatestCounters(histoDataPoints,
 outcomeMessage, N_MACRO, N_MINI, N_MICRO);
 *hdp_ = *mergedHDP;

 delete mergedHDP;
 }
 }
 */

void FastMonitor::snap() {
	ofstream outputFile;
	std::stringstream ss;
	outputFile.open(fastOutPath_.c_str(), fstream::out | fstream::trunc);
	// FIXME not optimal
	outputFile << dpd_->getName() << endl;
	for (unsigned int i = 0; i < monitoredVars_.size(); i++) {
		if (i == monitoredVars_.size() - 1) {
			outputFile << monitoredVars_[i]->toString();
			ss << monitoredVars_[i]->toString();
			break;
		}
		outputFile << monitoredVars_[i]->toString() << ",";
		ss << monitoredVars_[i]->toString() << ",";
	}
	outputFile << endl;
	outputFile.close();

	string inputStringCSV = ss.str();

	accumulatedCSV_.push_back(inputStringCSV);
}

void FastMonitor::outputFullHistoDataPoint(int outputFileSuffix) {

	if (accumulatedCSV_.size() > 0) {

		vector<HistoDataPoint*> hdpToMerge;

		for (unsigned int i = 0; i < accumulatedCSV_.size(); i++) {
			string currentCSV = accumulatedCSV_[i];
			HistoDataPoint* currentHistoDP =
					JSONHistoCollector::onelineCSVToHisto(currentCSV, dpd_,
							N_MACRO, N_MINI, N_MICRO);
			hdpToMerge.push_back(currentHistoDP);
		}

		string outputJSONAsString;
		string msg;

		HistoDataPoint* mergedHDP =
				ObjectMerger::mergeHistosButKeepLatestCounters(hdpToMerge, msg,
						N_MACRO, N_MINI, N_MICRO);

		for (unsigned int i = 0; i < hdpToMerge.size(); i++)
			delete hdpToMerge[i];

		JSONSerializer::serialize(mergedHDP, outputJSONAsString);
		stringstream ss;
		ss << fullOutPath_.substr(0, fullOutPath_.rfind(".")) << outputFileSuffix << ".jsh";
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

