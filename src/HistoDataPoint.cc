/*
 * HistoDataPoint.cc
 *
 *  Created on: Nov 26, 2012
 *      Author: aspataru
 */

#include "../interface/HistoDataPoint.h"

using namespace jsoncollector;
using std::string;
using std::vector;

const string HistoDataPoint::u_STATES = "uHisto";
const string HistoDataPoint::m_STATES = "mHisto";
const string HistoDataPoint::M_STATES = "MHisto";

HistoDataPoint::HistoDataPoint(unsigned int N_M, unsigned int N_m,
		unsigned int N_u) :
	DataPoint(), N_MACROSTATES(N_M), N_MINISTATES(N_m), N_MICROSTATES(N_u) {
	for (unsigned int i = 0; i < N_MACROSTATES; i++)
		MStates_.push_back(0);
	for (unsigned int i = 0; i < N_MINISTATES; i++)
		mStates_.push_back(0);
	for (unsigned int i = 0; i < N_MICROSTATES; i++)
		uStates_.push_back(0);

}

HistoDataPoint::HistoDataPoint(string source, string definition,
		vector<string> data, vector<unsigned int> uStates, vector<unsigned int> mStates,
		vector<unsigned int> MStates) :
	DataPoint(source, definition, data), uStates_(uStates), mStates_(mStates),
			MStates_(MStates) {

}

HistoDataPoint::~HistoDataPoint() {
}

void HistoDataPoint::operator=(HistoDataPoint& other) {
	this->source_ = other.getSource();
	this->data_ = other.getData();
	this->definition_ = other.getDefinition();
	this->uStates_ = other.getuStates();
	this->mStates_ = other.getmStates();
	this->MStates_ = other.getMStates();
}

void HistoDataPoint::serialize(Json::Value& root) const {
	root[SOURCE] = getSource();
	root[DEFINITION] = getDefinition();
	for (unsigned int i = 0; i < getData().size(); i++)
		root[DATA].append(getData()[i]);
	for (unsigned int i = 0; i < getuStates().size(); i++)
		root[u_STATES].append(getuStates()[i]);
	for (unsigned int i = 0; i < getmStates().size(); i++)
		root[m_STATES].append(getmStates()[i]);
	for (unsigned int i = 0; i < getMStates().size(); i++)
		root[M_STATES].append(getMStates()[i]);
}

void HistoDataPoint::deserialize(Json::Value& root) {
	source_ = root.get(SOURCE, "").asString();
	definition_ = root.get(DEFINITION, "").asString();
	if (root.get(DATA, "").isArray()) {
		unsigned int size = root.get(DATA, "").size();
		for (unsigned int i = 0; i < size; i++) {
			data_.push_back(root.get(DATA, "")[i].asString());
		}
	}
	MStates_.clear();
	if (root.get(M_STATES, "").isArray()) {
		unsigned int size = root.get(M_STATES, "").size();
		for (unsigned int i = 0; i < size; i++) {
			MStates_.push_back(root.get(M_STATES, "")[i].asInt());
		}
	}
	mStates_.clear();
	if (root.get(m_STATES, "").isArray()) {
		unsigned int size = root.get(m_STATES, "").size();
		for (unsigned int i = 0; i < size; i++) {
			mStates_.push_back(root.get(m_STATES, "")[i].asInt());
		}
	}
	uStates_.clear();
	if (root.get(u_STATES, "").isArray()) {
		unsigned int size = root.get(u_STATES, "").size();
		for (unsigned int i = 0; i < size; i++) {
			uStates_.push_back(root.get(u_STATES, "")[i].asInt());
		}
	}
}
