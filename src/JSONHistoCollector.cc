/*
 * JSONHistoCollectorOLD.cc
 *
 *  Created on: Nov 21, 2012
 *      Author: aspataru
 */

#include "../interface/JSONHistoCollector.h"
#include "../interface/FileIO.h"
#include "../interface/ObjectMerger.h"
#include "../interface/HistoDataPoint.h"
#include "../interface/Operations.h"
#include "../interface/JSONSerializer.h"

#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

using namespace jsoncollector;
using std::vector;
using std::string;
using std::stringstream;
using std::cout;
using std::endl;

// FIXME optimal implementation
// TODO multithread this operation
int JSONHistoCollector::fastToH(string& inputDirPath,
		vector<HistoDataPoint*>& outHistoData, bool includePerFileHistory,
		unsigned int N_M, unsigned int N_m, unsigned int N_u) {

	vector<string> inputFastFilePaths;
	string outcome;
	// get a list of all *.fast files in the input dir
	FileIO::getFileList(inputDirPath, inputFastFilePaths, outcome, ".fast");

	// read each file and add to histo datapoint
	for (unsigned int i = 0; i < inputFastFilePaths.size(); i++) {

		HistoDataPoint* hdp = new HistoDataPoint(N_M, N_m, N_u);

		string currentFastFile = inputFastFilePaths[i];
		std::ifstream infile(currentFastFile.c_str());
		// TODO fix
		hdp->setSource(currentFastFile);

		string currentValue;

		// read definition
		infile >> currentValue;
		DataPointDefinition* dpd = ObjectMerger::getDataPointDefinitionFor(
				currentValue);
		hdp->setDefinition(dpd->getName());

		vector<unsigned int> uHisto(N_u);
		vector<unsigned int> mHisto(N_m);
		vector<unsigned int> MHisto(N_M);

		// read each line and update histograms
		while (infile >> currentValue) {
			if (includePerFileHistory) {
				vector<string> tokens;
				std::stringstream ss(currentValue);
				while (!ss.eof()) {
					string field;
					getline(ss, field, ',');
					tokens.push_back(field);
				}
				for (unsigned int i = 0; i < tokens.size(); i++) {
					string currentOperation =
							dpd->getLegendFor(i).getOperation();
					int index = atoi(tokens[i].c_str());
					if (currentOperation.compare(Operations::M_HISTO) == 0)
						MHisto[index]++;
					if (currentOperation.compare(Operations::m_HISTO) == 0)
						mHisto[index]++;
					if (currentOperation.compare(Operations::u_HISTO) == 0)
						uHisto[index]++;
				}
			}
		}
		// re-tokenize last line and load into data array
		vector<string> tokens;
		std::istringstream ss(currentValue);
		while (!ss.eof()) {
			string field;
			getline(ss, field, ',');
			tokens.push_back(field);
		}

		hdp->resetData();

		for (unsigned int j = 0; j < tokens.size(); j++) {
			string currentOperation = dpd->getLegendFor(j).getOperation();
			// add last line of histo data if not using history
			if (!includePerFileHistory) {
				int index = atoi(tokens[j].c_str());
				if (currentOperation.compare(Operations::M_HISTO) == 0)
					MHisto[index]++;
				if (currentOperation.compare(Operations::m_HISTO) == 0)
					mHisto[index]++;
				if (currentOperation.compare(Operations::u_HISTO) == 0)
					uHisto[index]++;
			}

			if (currentOperation.compare(Operations::M_HISTO) == 0)
				hdp->addToData("M");
			else if (currentOperation.compare(Operations::m_HISTO) == 0)
				hdp->addToData("m");
			else if (currentOperation.compare(Operations::u_HISTO) == 0)
				hdp->addToData("u");
			else
				hdp->addToData(tokens[j]);
		}

		hdp->setMStates(MHisto);
		hdp->setmStates(mHisto);
		hdp->setuStates(uHisto);
		delete dpd;

		outHistoData.push_back(hdp);
	}

	return EXIT_SUCCESS;
}

HistoDataPoint* JSONHistoCollector::onelineCSVToHisto(string& olCSV,
		DataPointDefinition* dpd, unsigned int N_M,
		unsigned int N_m, unsigned int N_u) {

	HistoDataPoint* hdp = new HistoDataPoint(N_M, N_m, N_u);
	hdp->setDefinition(dpd->getName());

	vector<unsigned int> MHisto(N_M);
	vector<unsigned int> mHisto(N_m);
	vector<unsigned int> uHisto(N_u);

	vector<string> tokens;
	std::istringstream ss(olCSV);
	while (!ss.eof()) {
		string field;
		getline(ss, field, ',');
		tokens.push_back(field);
	}

	hdp->resetData();

	for (unsigned int j = 0; j < tokens.size(); j++) {
		string currentOperation = dpd->getLegendFor(j).getOperation();
		// add last line of histo data if not using history

		int index = atoi(tokens[j].c_str());
		if (currentOperation.compare(Operations::M_HISTO) == 0) {
			hdp->addToData(Operations::M_HISTO);
			MHisto[index]++;
		} else if (currentOperation.compare(Operations::m_HISTO) == 0) {
			hdp->addToData(Operations::m_HISTO);
			mHisto[index]++;
		} else if (currentOperation.compare(Operations::u_HISTO) == 0) {
			hdp->addToData(Operations::u_HISTO);
			uHisto[index]++;
		} else
			hdp->addToData(tokens[j]);
	}

	hdp->setMStates(MHisto);
	hdp->setmStates(mHisto);
	hdp->setuStates(uHisto);

	//delete dpd;

	return hdp;
}

int JSONHistoCollector::fastHistoMerge(string& inputDirPath,
		string& outputFilePath, unsigned int N_M, unsigned int N_m,
		unsigned int N_u) {

	vector<HistoDataPoint*> histoDataPoints;
	fastToH(inputDirPath, histoDataPoints, false, N_M, N_m, N_u);

	string outcomeMessage = "OK";
	HistoDataPoint* mergedHDP = ObjectMerger::merge(histoDataPoints,
			outcomeMessage, N_M, N_m, N_u);
	if (outcomeMessage.compare("OK") != 0) {
		cout << "Merging failed! Reason: " << outcomeMessage << endl;
		delete mergedHDP;
		return EXIT_FAILURE;
	}

	string outputJSONAsString;

	JSONSerializer::serialize(mergedHDP, outputJSONAsString);

	FileIO::writeStringToFile(outputFilePath, outputJSONAsString);

	while (!histoDataPoints.empty()) {
		delete histoDataPoints.back();
		histoDataPoints.pop_back();
	}
	delete mergedHDP;

	return EXIT_SUCCESS;

}

int JSONHistoCollector::fullHistoMerge(string& inputDirPath,
		string& outputFilePath, unsigned int N_M, unsigned int N_m,
		unsigned int N_u) {

	vector<HistoDataPoint*> histoDataPoints;
	fastToH(inputDirPath, histoDataPoints, true, N_M, N_m, N_u);

	string outcomeMessage = "OK";
	HistoDataPoint* mergedHDP = ObjectMerger::merge(histoDataPoints,
			outcomeMessage, N_M, N_m, N_u);
	if (outcomeMessage.compare("OK") != 0) {
		cout << "Merging failed! Reason: " << outcomeMessage << endl;
		delete mergedHDP;
		return EXIT_FAILURE;
	}

	string outputJSONAsString;

	JSONSerializer::serialize(mergedHDP, outputJSONAsString);

	FileIO::writeStringToFile(outputFilePath, outputJSONAsString);

	while (!histoDataPoints.empty()) {
		delete histoDataPoints.back();
		histoDataPoints.pop_back();
	}
	delete mergedHDP;

	return EXIT_SUCCESS;

}

// TODO implement --> combined operation with no file writing fast->merged jsh
// DEPRECATED?

int JSONHistoCollector::convertFastToHisto(string& inputDirPath,
		unsigned int N_M, unsigned int N_m, unsigned int N_u) {

	vector<string> inputFastFilePaths;
	string outcome;
	// get a list of all *.fast files in the input dir
	FileIO::getFileList(inputDirPath, inputFastFilePaths, outcome, ".fast");

	// declare the Json histogram file
	//Json::Value root;
	HistoDataPoint hdp(N_M, N_m, N_u);

	// read each file and create
	for (unsigned int i = 0; i < inputFastFilePaths.size(); i++) {
		string currentFastFile = inputFastFilePaths[i];
		std::ifstream infile(currentFastFile.c_str());
		// TODO fix
		hdp.setSource(currentFastFile);

		string currentValue;

		// read definition
		infile >> currentValue;
		DataPointDefinition* dpd = ObjectMerger::getDataPointDefinitionFor(
				currentValue);
		hdp.setDefinition(dpd->getName());

		vector<unsigned int> MHisto(N_M);
		vector<unsigned int> mHisto(N_m);
		vector<unsigned int> uHisto(N_u);

		// each line
		while (infile >> currentValue) {
			vector<string> tokens;
			std::stringstream ss(currentValue);
			while (!ss.eof()) {
				string field;
				getline(ss, field, ',');
				tokens.push_back(field);
			}
			for (unsigned int i = 0; i < tokens.size(); i++) {
				string currentOperation = dpd->getLegendFor(i).getOperation();
				int index = atoi(tokens[i].c_str());
				if (currentOperation.compare(Operations::M_HISTO) == 0)
					MHisto[index]++;
				if (currentOperation.compare(Operations::m_HISTO) == 0)
					mHisto[index]++;
				if (currentOperation.compare(Operations::u_HISTO) == 0)
					uHisto[index]++;
			}
		}

		// FIXME D-R-Y!
		// re-tokenize last line and load into data array
		vector<string> tokens;
		std::istringstream ss(currentValue);
		while (!ss.eof()) {
			string field;
			getline(ss, field, ',');
			tokens.push_back(field);
		}

		hdp.resetData();
		for (unsigned int j = 0; j < tokens.size(); j++) {
			string currentOperation = dpd->getLegendFor(j).getOperation();
			if (currentOperation.compare(Operations::M_HISTO) == 0)
				hdp.addToData(Operations::M_HISTO);
			else if (currentOperation.compare(Operations::m_HISTO) == 0)
				hdp.addToData(Operations::m_HISTO);
			else if (currentOperation.compare(Operations::u_HISTO) == 0)
				hdp.addToData(Operations::u_HISTO);
			else
				hdp.addToData(tokens[j]);
		}

		hdp.setMStates(MHisto);
		hdp.setmStates(mHisto);
		delete dpd;

		// serialize the HistoDataPoint
		string output;
		JSONSerializer::serialize(&hdp, output);

		// print to file

		std::stringstream strStream;
		strStream << i << ".jsh";
		string iterAsString = strStream.str();
		string currentOutPath = inputDirPath;
		currentOutPath.append(iterAsString);
		FileIO::writeStringToFile(currentOutPath, output);
	}

	return EXIT_SUCCESS;

}

int JSONHistoCollector::mergeHistoFiles(string& inputDirPath,
		string& outputFilePath, unsigned int N_M, unsigned int N_m,
		unsigned int N_u) {

	vector<HistoDataPoint*> histoDataPoints;
	vector<string> inputHistoFilePaths;
	string outcome;
	// get a list of all *.jsh files in the input dir
	FileIO::getFileList(inputDirPath, inputHistoFilePaths, outcome, ".jsh");

	for (unsigned int i = 0; i < inputHistoFilePaths.size(); i++) {
		HistoDataPoint* currentHDP = new HistoDataPoint(N_M, N_m, N_u);
		string inputJSONAsString;
		bool readOK = FileIO::readStringFromFile(inputHistoFilePaths[i],
				inputJSONAsString);
		if (!readOK) {
			cout << "Merging failed! Reason: data from "
					<< inputHistoFilePaths[i] << " could not be read!" << endl;
			return EXIT_FAILURE;
		}
		if (!JSONSerializer::deserialize(currentHDP, inputJSONAsString)) {
			cout << "Merging failed! Reason: data from "
					<< inputHistoFilePaths[i] << " could not be deserialized!"
					<< endl;
			return EXIT_FAILURE;
		}
		histoDataPoints.push_back(currentHDP);
	}

	string outcomeMessage = "OK";
	HistoDataPoint* mergedHDP = ObjectMerger::merge(histoDataPoints,
			outcomeMessage, N_M, N_m, N_u);
	if (outcomeMessage.compare("OK") != 0) {
		cout << "Merging failed! Reason: " << outcomeMessage << endl;
		delete mergedHDP;
		return EXIT_FAILURE;
	}

	string outputJSONAsString;

	//if (!formatForDisplay) {
	JSONSerializer::serialize(mergedHDP, outputJSONAsString);
	//}
	// format for display... the output json file will NOT RESPECT the input format
	// TODO add a test for DISPLAY FORMAT
	/*
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
	 */
	FileIO::writeStringToFile(outputFilePath, outputJSONAsString);

	while (!histoDataPoints.empty()) {
		delete histoDataPoints.back();
		histoDataPoints.pop_back();
	}
	delete mergedHDP;

	return EXIT_SUCCESS;
}
