/*
 * console_main.cc
 *
 *  Created on: Oct 11, 2012
 *      Author: aspataru
 */

#include "../interface/JSONFileCollector.h"
#include "../interface/FileIO.h"
#include "../interface/Utils.h"
#include <sys/time.h>

#include <sstream>
#include <iostream>
#include <cstdlib>
#include <cstring>

using namespace jsoncollector;
using std::vector;
using std::string;
using std::stringstream;
using std::cout;
using std::endl;

static const string VERSION = "0.5 - 22.03.2013";

// FIXME handle wrong args

int main(int argc, char **argv) {
	bool displayModeOn = false;
	bool jsnSimpleMode = false;
	bool fastCSVMode = false;

	// the regex is defaulted to everything
	string regex = ".*";
	string outputFilePath;
	vector<string> inputFilePaths;

	if (argc < 6) {
		cout << VERSION
				<< "   Usage is [-json/-csv] [-d] [-r <regex>] -o <outfile> -i <indir1> <indir2>...<infileN>"
				<< endl;
		exit(0);
	} else { // if we got enough parameters...
		for (int i = 1; i < argc; i++) {
			if (i + 1 != argc) {
				if (Utils::matchExactly(argv[i], "-json"))
					jsnSimpleMode = true;
				else if (Utils::matchExactly(argv[i], "-csv"))
					fastCSVMode = true;

				// display mode is on
				else if (Utils::matchExactly(argv[i], "-d")) {
					displayModeOn = true;
				}
				// file name regex is given
				else if (Utils::matchExactly(argv[i], "-r")) {
					regex = argv[i + 1];
				}
				// output file path
				else if (Utils::matchExactly(argv[i], "-o")) {
					outputFilePath = argv[i + 1];
				}
				// input paths (variable length)
				else if (Utils::matchExactly(argv[i], "-i")) {
					// load remaining arguments
					for (int j = i + 1; j < argc; j++) {
						// check if argument is file or folder
						string currentArg = argv[j];
						if (FileIO::isDir(currentArg)) {
							string dirLoadOutcome = "OK";

							string inputFormat = ".jsn";
							if (fastCSVMode)
								inputFormat = ".fast";

							FileIO::getFileList(currentArg, inputFilePaths,
									dirLoadOutcome, inputFormat, regex);
							if (dirLoadOutcome.compare("OK") != 0) {
								cout << "Loading input file list from: "
										<< currentArg << " failed! Reason: "
										<< dirLoadOutcome << endl;
								return EXIT_FAILURE;
							}
						}

						else {
							inputFilePaths.push_back(currentArg);
						}
					}
					break;
				}
			}
		}
	}

	// check if there are files to merge
	// 1 file is acceptable for conformity
	if (inputFilePaths.size() == 0) {
		cout << "There are no input files to aggregate!" << endl;
		return EXIT_FAILURE;
	}

	timeval start, finish;
	double elapsedTime;
	int outcome = EXIT_FAILURE;

	gettimeofday(&start, NULL);
	if (jsnSimpleMode)
		outcome = JSONFileCollector::mergeJSONFiles(inputFilePaths,
				outputFilePath, displayModeOn);
	else if (fastCSVMode)
		outcome = JSONFileCollector::mergeFastFiles(inputFilePaths,
				outputFilePath, displayModeOn);
	else
		cout << "FAIL!!" << endl;

	gettimeofday(&finish, NULL);

	// compute and print the elapsed time in ms
	elapsedTime = (finish.tv_sec - start.tv_sec) * 1000.0; // sec to ms
	elapsedTime += (finish.tv_usec - start.tv_usec) / 1000.0; // us to ms

	if (outcome == EXIT_SUCCESS) {
		cout << "JSONCOLLECTOR: " << inputFilePaths.size()
				<< " files merged successfully!" << endl;
		cout << " >>> time (ms): " << elapsedTime << endl;
	}

	return outcome;
}
