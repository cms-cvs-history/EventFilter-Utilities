/*
 * RegexWrapper.cc
 *
 *  Created on: Nov 1, 2012
 *      Author: aspataru
 */

#include "../interface/RegexWrapper.h"

bool RegexWrapper::match(regex_t *pexp, char *sz) {
	regmatch_t matches[MAX_MATCHES]; //A list of the matches in the string (a list of 1)
	if (regexec(pexp, sz, MAX_MATCHES, matches, 0) == 0)
		return true;
	else
		return false;
}

bool RegexWrapper::compile(regex_t* exp, const char* exprStr) {
	int rv = regcomp(exp, exprStr, REG_EXTENDED);
	if (rv != 0)
		return false;
	return true;
}

void RegexWrapper::free(regex_t* exp) {
	regfree(exp);
}
