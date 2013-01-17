/*
 * JSONFileCollector.cc
 *
 *  Created on: Sep 25, 2012
 *      Author: aspataru
 */

#include "../interface/JSONFileCollector.h"
#include "../interface/ObjectMerger.h"
#include "../interface/JSONSerializer.h"
#include "../interface/FileIO.h"
#include "../interface/DataPoint.h"
#include "../interface/JsonSerializable.h"
#include <iostream>
#include <string>
#include <vector>

using namespace jsoncollector;
using std::vector;
using std::string;
using std::cout;
using std::endl;

int JSONFileCollector::mergeFiles(vector<string>& inputJSONFilePaths,
		string& outputFilePath, bool formatForDisplay) {

	vector<DataPoint*> dataPoints;
	for (unsigned int i = 0; i < inputJSONFilePaths.size(); i++) {
		DataPoint* currentDP = new DataPoint();
		string inputJSONAsString;
		bool readOK = FileIO::readStringFromFile(inputJSONFilePaths[i],
				inputJSONAsString);
		if (!readOK) {
			cout << "Merging failed! Reason: data from "
					<< inputJSONFilePaths[i] << " could not be read!" << endl;
			return EXIT_FAILURE;
		}
		JSONSerializer::deserialize(currentDP, inputJSONAsString);
		dataPoints.push_back(currentDP);
	}

	string outcomeMessage = "OK";
	DataPoint* mergedDP = ObjectMerger::merge(dataPoints, outcomeMessage);
	if (outcomeMessage.compare("OK") != 0) {
		cout << "Merging failed! Reason: " << outcomeMessage << endl;
		delete mergedDP;
		return EXIT_FAILURE;
	}

	string outputJSONAsString;

	if (!formatForDisplay) {
		JSONSerializer::serialize(mergedDP, outputJSONAsString);
	}
	// format for display... the output json file will NOT RESPECT the input format
	// TODO add a test for DISPLAY FORMAT
	else {
		Json::Value displayableRoot;
		DataPointDefinition* dpd = ObjectMerger::getDataPointDefinitionFor(
				mergedDP->getDefinition());
		displayableRoot[DataPoint::SOURCE] = mergedDP->getSource();
		displayableRoot[DataPoint::DEFINITION] = mergedDP->getDefinition();
		for (unsigned int i = 0; i < mergedDP->getData().size(); i++) {
			displayableRoot[dpd->getLegend()[i].getName()]
					= mergedDP->getData()[i];
		}
		delete dpd;
		Json::StyledWriter writer;
		outputJSONAsString = writer.write(displayableRoot);
	}

	FileIO::writeStringToFile(outputFilePath, outputJSONAsString);

	while (!dataPoints.empty()) {
		delete dataPoints.back();
		dataPoints.pop_back();
	}
	delete mergedDP;

	return EXIT_SUCCESS;
}
