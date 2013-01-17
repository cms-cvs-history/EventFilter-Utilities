/*
 * ObjectMerger.cc
 *
 *  Created on: Sep 24, 2012
 *      Author: aspataru
 */

#include "../interface/ObjectMerger.h"
#include "../interface/Operations.h"
#include "../interface/Utils.h"
#include "../interface/FileIO.h"
#include "../interface/JSONSerializer.h"
#include <sstream>
#include <iostream>

using namespace jsoncollector;
using std::vector;
using std::string;
using std::stringstream;
using std::cout;
using std::endl;

DataPoint* ObjectMerger::merge(vector<DataPoint*> objectsToMerge,
		string& outcomeMessage) {
	// vector of vectors containing all data in datapoints
	vector<vector<string> > mergedData;

	// check consistency of input files
	if (!checkConsistency(objectsToMerge, outcomeMessage))
		return NULL;

	// 1. Get the definition of these data points
	DataPointDefinition* dpd = getDataPointDefinitionFor(
			objectsToMerge[0]->getDefinition());

	// 1.1 Check if definition has exact no of elements specified
	if (objectsToMerge[0]->getData().size() != dpd->getLegend().size()) {
		outcomeMessage
				= "JSON files and DEFINITION do not have the same number of elements in vectors!";
		delete dpd;
		return NULL;
	}

	// 2. Assemble merged vector
	for (unsigned int nMetric = 0; nMetric
			< objectsToMerge[0]->getData().size(); nMetric++) {
		// 2.1. assemble vector of n-th data elements
		vector<string> metricVector;
		for (unsigned int nObj = 0; nObj < objectsToMerge.size(); nObj++)
			metricVector.push_back(objectsToMerge[nObj]->getData()[nMetric]);
		mergedData.push_back(metricVector);
	}

	// 3. Apply defined operation for each measurement in all data poutcomeMessageoints
	vector<string> outputValues;
	for (unsigned int i = 0; i < mergedData.size(); i++) {
		string strVal = applyOperation(mergedData[i],
				dpd->getLegendFor(i).getOperation());
		outputValues.push_back(strVal);
	}

	// 4. Assemble output DataPoint
	stringstream ss;
	for (unsigned int i = 0; i < objectsToMerge.size(); i++) {
		ss << objectsToMerge[i]->getSource();
		if (i != objectsToMerge.size() - 1)
			ss << ", ";
	}

	string source = ss.str();
	string definition = objectsToMerge[0]->getDefinition();
	DataPoint* outputDP = new DataPoint(source, definition, outputValues);

	// delete dpd after no longer needed
	delete dpd;

	return outputDP;
}

HistoDataPoint* ObjectMerger::merge(vector<HistoDataPoint*>& objectsToMerge,
		string& outcomeMessage, unsigned int N_M, unsigned int N_m,
		unsigned int N_u) {

	// use DataPoint merge first
	string dpOut;
	vector<DataPoint*> initial;

	for (unsigned int i = 0; i < objectsToMerge.size(); i++) {
		DataPoint* current = dynamic_cast<DataPoint*> (objectsToMerge[i]);
		initial.push_back(current);
	}
	DataPoint* mergedDP = ObjectMerger::merge(initial, dpOut);

	// now fill the rest
	HistoDataPoint* outputHDP = new HistoDataPoint(N_M, N_m, N_u);

	outputHDP->setDefinition(mergedDP->getDefinition());
	outputHDP->setData(mergedDP->getData());
	outputHDP->setSource(mergedDP->getSource());

	vector<unsigned int> uSt;
	vector<unsigned int> mSt;
	vector<unsigned int> MSt;
	for (unsigned int i = 0; i < N_M; i++)
		MSt.push_back(0);
	for (unsigned int i = 0; i < N_m; i++)
		mSt.push_back(0);
	for (unsigned int i = 0; i < N_u; i++)
		uSt.push_back(0);

	// compose new mStates Histo
	for (unsigned int i = 0; i < objectsToMerge.size(); i++) {
		// iterate on the MState histo
		for (unsigned int j = 0; j < objectsToMerge[i]->getMStates().size(); j++) {
			MSt[j] += objectsToMerge[i]->getMStates()[j];
		}
		for (unsigned int j = 0; j < objectsToMerge[i]->getmStates().size(); j++) {
			mSt[j] += objectsToMerge[i]->getmStates()[j];
		}
		for (unsigned int j = 0; j < objectsToMerge[i]->getuStates().size(); j++) {
			uSt[j] += objectsToMerge[i]->getuStates()[j];
		}
	}

	outputHDP->setMStates(MSt);
	outputHDP->setmStates(mSt);
	outputHDP->setuStates(uSt);

	delete mergedDP;

	return outputHDP;
}

HistoDataPoint* ObjectMerger::mergeHistosButKeepLatestCounters(vector<HistoDataPoint*> objectsToMerge,
		string& outcomeMessage, unsigned int N_M, unsigned int N_m,
		unsigned int N_u) {

	// get last data point in array
	DataPoint* lastDP = dynamic_cast<DataPoint*> (objectsToMerge[objectsToMerge.size() - 1]);

	// now fill the rest
	HistoDataPoint* outputHDP = new HistoDataPoint(N_M, N_m, N_u);

	outputHDP->setDefinition(lastDP->getDefinition());
	outputHDP->setData(lastDP->getData());
	outputHDP->setSource(lastDP->getSource());

	vector<unsigned int> uSt;
	vector<unsigned int> mSt;
	vector<unsigned int> MSt;
	for (unsigned int i = 0; i < N_M; i++)
		MSt.push_back(0);
	for (unsigned int i = 0; i < N_m; i++)
		mSt.push_back(0);
	for (unsigned int i = 0; i < N_u; i++)
		uSt.push_back(0);

	// compose new mStates Histo
	for (unsigned int i = 0; i < objectsToMerge.size(); i++) {
		// iterate on the MState histo
		for (unsigned int j = 0; j < objectsToMerge[i]->getMStates().size(); j++) {
			MSt[j] += objectsToMerge[i]->getMStates()[j];
		}
		for (unsigned int j = 0; j < objectsToMerge[i]->getmStates().size(); j++) {
			mSt[j] += objectsToMerge[i]->getmStates()[j];
		}
		for (unsigned int j = 0; j < objectsToMerge[i]->getuStates().size(); j++) {
			uSt[j] += objectsToMerge[i]->getuStates()[j];
		}
	}

	outputHDP->setMStates(MSt);
	outputHDP->setmStates(mSt);
	outputHDP->setuStates(uSt);

	//delete lastDP;

	return outputHDP;
}

DataPointDefinition* ObjectMerger::getDataPointDefinitionFor(string defFilePath) {
	DataPointDefinition* dpd = new DataPointDefinition();
	string dpdString;
	bool readOK = FileIO::readStringFromFile(defFilePath, dpdString);
	// data point definition is bad!
	if (!readOK) {
		cout << "Cannot read from JSON definition path: " << defFilePath
				<< endl;
		return dpd;
	}
	JSONSerializer::deserialize(dpd, dpdString);
	return dpd;
}

string ObjectMerger::applyOperation(std::vector<string> dataVector,
		std::string operationName) {
	string opResultString = "N/A";

	if (operationName.compare(Operations::SUM) == 0) {
		stringstream ss;
		double opResult = Operations::sum(
				Utils::vectorStringToDouble(dataVector));
		ss << opResult;
		opResultString = ss.str();

	} else if (operationName.compare(Operations::AVG) == 0) {
		stringstream ss;
		double opResult = Operations::avg(
				Utils::vectorStringToDouble(dataVector));
		ss << opResult;
		opResultString = ss.str();
	}
	/*
	 * ADD MORE OPERATIONS HERE
	 */
	else if (operationName.compare(Operations::SAME) == 0) {
		opResultString = Operations::same(dataVector);
	}

	else if (operationName.compare(Operations::M_HISTO) == 0) {
		opResultString = Operations::M_HISTO;
	}

	else if (operationName.compare(Operations::m_HISTO) == 0) {
		opResultString = Operations::m_HISTO;
	}

	else if (operationName.compare(Operations::u_HISTO) == 0) {
		opResultString = Operations::u_HISTO;
	}

	// OPERATION WAS NOT DEFINED
	else {
		cout << "Operation " << operationName << " is NOT DEFINED!" << endl;
	}

	return opResultString;
}

bool ObjectMerger::checkConsistency(std::vector<DataPoint*> objectsToMerge,
		std::string& outcomeMessage) {

	for (unsigned int i = 0; i < objectsToMerge.size() - 1; i++) {
		// 1. Check if all have the same definition
		if (objectsToMerge[i]->getDefinition().compare(
				objectsToMerge[i + 1]->getDefinition()) != 0) {
			outcomeMessage = "JSON files have inconsistent definitions!";
			return false;
		}
		// 2. Check if objects to merge have the same number of elements in data vector
		if (objectsToMerge[i]->getData().size()
				!= objectsToMerge[i + 1]->getData().size()) {
			outcomeMessage
					= "JSON files have inconsistent number of elements in the data vector!";
			return false;
		}
	}
	return true;
}
