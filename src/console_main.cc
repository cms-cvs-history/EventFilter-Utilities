/*
 * console_main.cc
 *
 *  Created on: Oct 11, 2012
 *      Author: aspataru
 */

#include "../interface/JSONFileCollector.h"
#include "../interface/JSONHistoCollector.h"
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

// FIXME handle wrong args
// FIXME remove possibility of input files

int main(int argc, char **argv) {
	bool displayModeOn = false;
	bool jsnSimpleMode = false;
	bool jsnHistoMode = false;
	bool fastCSVMode = false;
	//bool fullCSVMode = false;
	unsigned int M_bins = 10, m_bins = 10, u_bins = 10;
	// FIXME
	string THEDIR = "";
	// the regex is defaulted to everything
	string regex = ".*";
	string outputFilePath;
	vector<string> inputFilePaths;

	if (argc < 5) {
		cout
				<< "Usage is [-jsn/-jsh/-fastcsv] [-nb 10 10 10] [-d] [-r <regex>] -o <outfile> -i <indir1> <indir2>...<infileN>\n";
		exit(0);
	} else { // if we got enough parameters...
		for (int i = 1; i < argc; i++) {
			if (i + 1 != argc) {
				if (Utils::matchExactly(argv[i], "-jsn"))
					jsnSimpleMode = true;
				else if (Utils::matchExactly(argv[i], "-jsh"))
					jsnHistoMode = true;
				else if (Utils::matchExactly(argv[i], "-fastcsv"))
					fastCSVMode = true;
				/*
				 else if (Utils::matchExactly(argv[i], "-fullcsv"))
				 fullCSVMode = true;
				 */
				// display mode is on
				else if (Utils::matchExactly(argv[i], "-nb")) {
					M_bins = atoi(argv[i + 1]);
					m_bins = atoi(argv[i + 2]);
					u_bins = atoi(argv[i + 3]);
				}
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
							THEDIR = currentArg;
							string dirLoadOutcome = "OK";

							string inputFormat = ".jsn";
							if (jsnHistoMode)
								inputFormat = ".jsh";
							else if (fastCSVMode/* || fullCSVMode*/)
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
		outcome = JSONFileCollector::mergeFiles(inputFilePaths, outputFilePath,
				displayModeOn);
	else if (jsnHistoMode)
		outcome = JSONHistoCollector::mergeHistoFiles(THEDIR, outputFilePath,
				M_bins, m_bins, u_bins);
	else if (fastCSVMode)
		outcome = JSONHistoCollector::fastHistoMerge(THEDIR, outputFilePath,
				M_bins, m_bins, u_bins);
	/*
	 else if (fullCSVMode)
	 outcome = JSONHistoCollector::fullHistoMerge(THEDIR, outputFilePath,
	 50, 50, 50);
	 */
	else
		cout << "FAIL!!" << endl;
	gettimeofday(&finish, NULL);

	// compute and print the elapsed time in ms
	elapsedTime = (finish.tv_sec - start.tv_sec) * 1000.0; // sec to ms
	elapsedTime += (finish.tv_usec - start.tv_usec) / 1000.0; // us to ms

	if (outcome == EXIT_SUCCESS) {
		cout << inputFilePaths.size()
				<< " JSONCOLLECTOR: Files merged successfully!" << endl;
		cout << " ---> total time (ms): " << elapsedTime << endl;
	}

	return outcome;
}
