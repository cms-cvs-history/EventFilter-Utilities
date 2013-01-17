/*
 * FileIO.cc
 *
 *  Created on: Sep 25, 2012
 *      Author: aspataru
 */

#include "../interface/FileIO.h"
#include "../interface/RegexWrapper.h"

#include <iostream>
#include <fstream>
#include <streambuf>
#include <cstdlib>
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <cstring>

using namespace jsoncollector;
using std::string;
using std::ofstream;
using std::vector;
using std::ifstream;
using std::strlen;

void FileIO::writeStringToFile(string& filename, string& content) {
	ofstream outputFile;
	outputFile.open(filename.c_str());
	outputFile << content;
	outputFile.close();
}

bool FileIO::readStringFromFile(string& filename, string& content) {
	if (!fileExists(filename))
		return false;

	std::ifstream inputFile(filename.c_str());

	inputFile.seekg(0, std::ios::end);
	content.reserve(inputFile.tellg());
	inputFile.seekg(0, std::ios::beg);

	content.assign((std::istreambuf_iterator<char>(inputFile)),
			std::istreambuf_iterator<char>());

	inputFile.close();
	return true;
}

bool FileIO::fileExists(string& path) {
	ifstream ifile(path.c_str());
	return ifile;
}

bool FileIO::isDir(string& path) {
	struct stat s;
	if (stat(path.c_str(), &s) == 0) {
		if (s.st_mode & S_IFDIR)
			return true;
		else {
			return false;
		}
	} else {
		return false;
	}
}

void FileIO::getFileList(string& dir, vector<string>& fileList,
		string& outcome, string fileType, string fnRegex) {
	DIR *dp;
	struct dirent *dirp;

	char lastChar = dir[strlen(dir.c_str()) - 1];

	if (lastChar != '/')
		dir.append("/");

	// check if dir can be opened
	if ((dp = opendir(dir.c_str())) == NULL) {
		outcome = "Could not open dir: ";
		outcome.append(dir);
		outcome.append(" Not a directory?");
		return;
	}

	while ((dirp = readdir(dp)) != NULL) {
		string currentFile = string(dirp->d_name);
		size_t found = currentFile.find(fileType);
		regex_t exp;
		/* bool compRes = */
		RegexWrapper::compile(&exp, fnRegex.c_str());
		if (found != string::npos) {
			string fileWithoutExtension = currentFile.substr(0, found);
			if (RegexWrapper::match(&exp, (char*) fileWithoutExtension.c_str())) {
				string absoluteFilePath = dir;
				absoluteFilePath.append(currentFile);
				fileList.push_back(absoluteFilePath);
			}
		}
		RegexWrapper::free(&exp);
	}

	closedir(dp);
}
