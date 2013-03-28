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
#include "../interface/JsonSerializable.h"
#include <iostream>
#include <fstream>
#include <cstdlib>

using namespace jsoncollector;
using std::vector;
using std::string;
using std::cout;
using std::endl;

int JSONFileCollector::mergeJSONFiles(vector<string>& inputJSONFilePaths,
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
	DataPoint* mergedDP =
			ObjectMerger::merge(dataPoints, outcomeMessage, false);
	if (outcomeMessage.compare("OK") != 0) {
		cout << "Merging failed! Reason: " << outcomeMessage << endl;
		delete mergedDP;
		return EXIT_FAILURE;
	}

	string outputJSONAsString;
	string outputJSONDisplayableAsString;
	string dispOutPath = outputFilePath + ".d";

	JSONSerializer::serialize(mergedDP, outputJSONAsString);

	// format for display... the output json file will NOT RESPECT the input format
	// TODO add a test for DISPLAY FORMAT
	if (formatForDisplay) {
		if (!displayFormat(mergedDP, outputJSONDisplayableAsString)) {
			return EXIT_FAILURE;
		}
		FileIO::writeStringToFile(dispOutPath, outputJSONDisplayableAsString);
	}

	FileIO::writeStringToFile(outputFilePath, outputJSONAsString);

	while (!dataPoints.empty()) {
		delete dataPoints.back();
		dataPoints.pop_back();
	}
	delete mergedDP;

	return EXIT_SUCCESS;
}

int JSONFileCollector::mergeFastFiles(vector<string>& fastFiles,
		string& outputFilePath, bool formatForDisplay) {

	vector<DataPoint*> dpToMerge;
	DataPointDefinition dpd;

	// for each fast file:
	for (unsigned int i = 0; i < fastFiles.size(); i++) {

		string currentFastFile = fastFiles[i];
		std::ifstream infile(currentFastFile.c_str());
		string currentValue;

		// read definition
		infile >> currentValue;
		string defPath = currentValue;
		bool defLoaded = ObjectMerger::getDataPointDefinitionFor(defPath, dpd);
		if (!defLoaded)
			return EXIT_FAILURE;

		// read data
		infile >> currentValue;
		string dataCSV = currentValue;

		DataPoint* currentDP = ObjectMerger::csvToJson(dataCSV, &dpd, defPath);

		//delete dpd;

		currentDP->setSource(currentFastFile);
		dpToMerge.push_back(currentDP);
	}

	string outcomeMessage = "OK";

	DataPoint* mergedDP = ObjectMerger::merge(dpToMerge, outcomeMessage, false);

	if (outcomeMessage.compare("OK") != 0) {
		cout << "Merging failed! Reason: " << outcomeMessage << endl;
		delete mergedDP;
		return EXIT_FAILURE;
	}

	string outputJSONAsString;
	string outputJSONDisplayableAsString;
	string dispOutPath = outputFilePath + ".d";

	JSONSerializer::serialize(mergedDP, outputJSONAsString);

	// format for display... the output json file will NOT RESPECT the input format
	if (formatForDisplay) {
		if (!displayFormat(mergedDP, outputJSONDisplayableAsString)) {
			return EXIT_FAILURE;
		}
		FileIO::writeStringToFile(dispOutPath, outputJSONDisplayableAsString);
	}

	FileIO::writeStringToFile(outputFilePath, outputJSONAsString);

	while (!dpToMerge.empty()) {
		delete dpToMerge.back();
		dpToMerge.pop_back();
	}

	delete mergedDP;

	return EXIT_SUCCESS;

}

// TODO ?JSONSerializer --> add serialize for display!!!!
bool JSONFileCollector::displayFormat(DataPoint* dp, std::string& output) {
	Json::Value displayableRoot;
	DataPointDefinition dpd;
	bool defLoaded = ObjectMerger::getDataPointDefinitionFor(
			dp->getDefinition(), dpd);
	if (!defLoaded)
		return false;
	displayableRoot[DataPoint::SOURCE] = dp->getSource();
	displayableRoot[DataPoint::DEFINITION] = dp->getDefinition();
	for (unsigned int i = 0; i < dp->getData().size(); i++) {
		displayableRoot[dpd.getLegend()[i].getName()] = dp->getData()[i];
	}
	//delete dpd;
	Json::StyledWriter writer;
	output = writer.write(displayableRoot);
	return true;
}
