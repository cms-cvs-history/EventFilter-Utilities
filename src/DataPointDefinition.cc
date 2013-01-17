/*
 * DataPointDefinition.cc
 *
 *  Created on: Sep 24, 2012
 *      Author: aspataru
 */

#include "../interface/DataPointDefinition.h"

using namespace jsoncollector;
using std::string;
using std::vector;

const string DataPointDefinition::FILE = "file";
const string DataPointDefinition::LEGEND = "legend";
const string DataPointDefinition::PARAM_NAME = "name";
const string DataPointDefinition::OPERATION = "operation";

DataPointDefinition::DataPointDefinition() :
	name_("") {
}

DataPointDefinition::DataPointDefinition(string name, vector<LegendItem> legend) :
	name_(name), legend_(legend) {
}

DataPointDefinition::~DataPointDefinition() {
}

void DataPointDefinition::serialize(Json::Value& root) const {
	root[FILE] = getName();
	for (unsigned int i = 0; i < getLegend().size(); i++) {
		Json::Value currentDef;
		currentDef[PARAM_NAME] = getLegendFor(i).getName();
		currentDef[OPERATION] = getLegendFor(i).getOperation();
		root[LEGEND].append(currentDef);
	}
}

void DataPointDefinition::deserialize(Json::Value& root) {
	name_ = root.get(FILE, "").asString();
	if (root.get(LEGEND, "").isArray()) {
		unsigned int size = root.get(LEGEND, "").size();
		for (unsigned int i = 0; i < size; i++) {
			LegendItem currentLegendItem(
					root.get(LEGEND, "")[i].get(PARAM_NAME, "").asString(),
					root.get(LEGEND, "")[i].get(OPERATION, "").asString());
			legend_.push_back(currentLegendItem);
		}
	}
}

LegendItem DataPointDefinition::getLegendFor(unsigned int index) const {
	return legend_[index];
}

bool DataPointDefinition::isPopulated() const {
	if (getLegend().size() > 0)
		return true;
	return false;
}
