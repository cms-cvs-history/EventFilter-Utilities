/*
 * RegexWrapper.h
 *
 *  Created on: Nov 1, 2012
 *      Author: aspataru
 */

#ifndef REGEXWRAPPER_H_
#define REGEXWRAPPER_H_

#include <regex.h>
#include <sys/types.h>
#include <stdio.h>

#define MAX_MATCHES 1 //The maximum number of matches allowed in a single string

class RegexWrapper {

public:
	static bool compile(regex_t* exp, const char* exprStr);
	static bool match(regex_t* pexp, char* sz);
	static void free(regex_t* exp);
};

#endif /* REGEXWRAPPER_H_ */
