/*
 * Utils.cc
 *
 *  Created on: 1 Oct 2012
 *      Author: secre
 */

#include "../interface/Utils.h"
#include <sstream>
#include <cstdlib>

using namespace jsoncollector;
using std::string;
using std::vector;
using std::stringstream;
using std::atof;

vector<string> Utils::vectorDoubleToString(vector<double> doubleVector) {
	vector<string> strVector;
	stringstream ss;
	for (unsigned int i = 0; i < doubleVector.size(); i++) {
		ss << doubleVector[i];
		strVector.push_back(ss.str());
		ss.str("");
	}
	return strVector;
}

vector<double> Utils::vectorStringToDouble(vector<string> stringVector) {
	vector<double> dblVector;
	for (unsigned int i = 0; i < stringVector.size(); i++)
		dblVector.push_back(atof(stringVector[i].c_str()));
	return dblVector;
}

bool Utils::matchExactly(string s1, string s2) {
	if (s1.find(s2) == 0 && s2.find(s1) == 0)
		return true;
	return false;
}
