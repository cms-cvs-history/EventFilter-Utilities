/*
 * Operations.cc
 *
 *  Created on: Sep 24, 2012
 *      Author: aspataru
 */

#include "../interface/Operations.h"
#include "../interface/Utils.h"

using namespace jsoncollector;
using std::vector;
using std::string;

const string Operations::SUM = "sum";
const string Operations::AVG = "avg";
const string Operations::SAME = "same";

const string Operations::u_HISTO = "uHisto";
const string Operations::m_HISTO = "mHisto";
const string Operations::M_HISTO = "MHisto";

double Operations::sum(vector<double> elems) {
	double added = elems[0];
	for (unsigned int i = 1; i < elems.size(); i++)
		added += elems[i];
	return added;
}

double Operations::avg(vector<double> elems) {
	return sum(elems) / elems.size();
}

string Operations::same(vector<string> elems) {
	for (unsigned int i = 0; i < elems.size() - 1; i++)
		if (!Utils::matchExactly(elems[i], elems[i + 1]) || elems[i].length() == 0)
			return "N/A";
	return elems[0];
}
