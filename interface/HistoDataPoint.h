/*
 * HistoDataPoint.h
 *
 *  Created on: Nov 26, 2012
 *      Author: aspataru
 */

#ifndef HISTODATAPOINT_H_
#define HISTODATAPOINT_H_

#include "DataPoint.h"

namespace jsoncollector {

class HistoDataPoint: public DataPoint {

public:
	HistoDataPoint(unsigned int N_M, unsigned int N_m, unsigned int N_u);
	HistoDataPoint(std::string source, std::string definition,
			std::vector<std::string> data, std::vector<unsigned int> uStates,
			std::vector<unsigned int> mStates, std::vector<unsigned int> MStates);
	virtual ~HistoDataPoint();
	void operator=(HistoDataPoint& other);

	/**
	 * JSON serialization procedure for this class
	 */
	virtual void serialize(Json::Value& root) const;
	/**
	 * JSON deserialization procedure for this class
	 */
	virtual void deserialize(Json::Value& root);

	std::vector<unsigned int> getuStates() const {
		return uStates_;
	}
	std::vector<unsigned int> getmStates() const {
		return mStates_;
	}
	std::vector<unsigned int> getMStates() const {
		return MStates_;
	}

	void setData(std::vector<std::string> data) {
		data_ = data;
	}
	void setuStates(std::vector<unsigned int> uStates) {
		uStates_ = uStates;
	}
	void setmStates(std::vector<unsigned int> mStates) {
		mStates_ = mStates;
	}
	void setMStates(std::vector<unsigned int> MStates) {
		MStates_ = MStates;
	}

	static const std::string u_STATES;
	static const std::string m_STATES;
	static const std::string M_STATES;

	unsigned int N_MACROSTATES;
	unsigned int N_MINISTATES;
	unsigned int N_MICROSTATES;


private:
	std::vector<unsigned int> uStates_, mStates_, MStates_;

};
}

#endif /* HISTODATAPOINT_H_ */
