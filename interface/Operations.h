/*
 * Operations.h
 *
 *  Created on: Sep 24, 2012
 *      Author: aspataru
 */

#ifndef OPERATIONS_H_
#define OPERATIONS_H_

#include <stdint.h>
#include <vector>
#include <string>

namespace jsoncollector {
class Operations {

public:
	/*
	 * KNOWN OPERATIONS
	 */
	static double sum(std::vector<double>);
	static double avg(std::vector<double>);
	static std::string same(std::vector<std::string>);

	/*
	 * KNOWN OPERATION NAMES
	 */
	static const std::string SUM;
	static const std::string AVG;
	static const std::string SAME;

	static const std::string u_HISTO;
	static const std::string m_HISTO;
	static const std::string M_HISTO;

};
}


#endif /* OPERATIONS_H_ */
