/*
 * DataPointMonitor.cc
 *
 *  Created on: Oct 29, 2012
 *      Author: aspataru
 */

#include "../interface/DataPointMonitor.h"
#include "../interface/ObjectMerger.h"

using namespace jsoncollector;

DataPointMonitor::DataPointMonitor(
		vector<JsonMonitorable*> monitorableVariables,
		vector<string> toBeMonitored) :
	monitorableVars_(monitorableVariables), toBeMonitored_(toBeMonitored),
			dpd_(0) {

	for (unsigned int i = 0; i < toBeMonitored_.size(); i++) {
		if (isStringMonitorable(toBeMonitored_[i]))
			monitoredVars_.push_back(getVarForName(toBeMonitored_[i]));
	}
}

DataPointMonitor::DataPointMonitor(
		vector<JsonMonitorable*> monitorableVariables, string defPath) :
	monitorableVars_(monitorableVariables) {

	dpd_ = ObjectMerger::getDataPointDefinitionFor(defPath);
	if (dpd_->isPopulated()) {
		for (unsigned int i = 0; i < dpd_->getLegend().size(); i++) {
			string toBeMonitored = dpd_->getLegend()[i].getName();
			monitoredVars_.push_back(getVarForName(toBeMonitored));
		}
	}
}

DataPointMonitor::DataPointMonitor(
		vector<JsonMonitorable*> monitorableVariables, DataPointDefinition* def) :
	monitorableVars_(monitorableVariables), dpd_(def) {

	if (dpd_->isPopulated()) {
		for (unsigned int i = 0; i < dpd_->getLegend().size(); i++) {
			string toBeMonitored = dpd_->getLegend()[i].getName();
			monitoredVars_.push_back(getVarForName(toBeMonitored));
		}
	}
}

DataPointMonitor::~DataPointMonitor() {
	if (dpd_ != 0)
		delete dpd_;
}

void DataPointMonitor::snap(DataPoint& outputDataPoint) {
	outputDataPoint.resetData();
	for (unsigned int i = 0; i < monitoredVars_.size(); i++)
		outputDataPoint.addToData(monitoredVars_[i]->toString());
	outputDataPoint.setSource("");
	if (dpd_ == 0)
		outputDataPoint.setDefinition("");
	else
		outputDataPoint.setDefinition(dpd_->getName());
}

bool DataPointMonitor::isStringMonitorable(string key) const {
	for (unsigned int i = 0; i < toBeMonitored_.size(); i++)
		if (key.compare(toBeMonitored_[i]) == 0)
			return true;
	return false;
}

JsonMonitorable* DataPointMonitor::getVarForName(string name) const {
	for (unsigned int i = 0; i < monitorableVars_.size(); i++)
		if (name.compare(monitorableVars_[i]->getName()) == 0)
			return monitorableVars_[i];
	return 0;
}

