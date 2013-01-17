/*
 * Utils.h
 *
 *  Created on: 1 Oct 2012
 *      Author: secre
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <string>
#include <vector>

namespace jsoncollector {
class Utils {

public:
	/**
	 * Convenience method to convert a vector of doubles into a vector of strings
	 */
	static std::vector<std::string> vectorDoubleToString(std::vector<double> doubleVector);
	/**
	 * Convenience method to convert a vector of strings into a vector of doubles
	 */
	static std::vector<double> vectorStringToDouble(std::vector<std::string> stringVector);
	/**
	 * Returns true if the strings match exactly
	 */
	static bool matchExactly(std::string s1, std::string s2);
};
}


#endif /* UTILS_H_ */
