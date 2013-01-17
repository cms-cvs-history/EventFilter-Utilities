/*
 * FileIO.h
 *
 *  Created on: Sep 25, 2012
 *      Author: aspataru
 */

#ifndef FILEIO_H_
#define FILEIO_H_

#include <string>
#include <vector>

namespace jsoncollector {
class FileIO {
public:
	/**
	 * Writes given string to specified file path
	 */
	static void writeStringToFile(std::string& filename, std::string& content);
	/**
	 * Reads string from specified path, returns false if file does not exist
	 */
	static bool readStringFromFile(std::string& filename, std::string& content);
	/**
	 * Checks if path points to an existing file
	 */
	static bool fileExists(std::string& path);
	/**
	 * Checks if path is a directory
	 */
	static bool isDir(std::string& path);
	/**
	 * Gets a list of <fileType> files from the specified directory using a regex if given
	 */
	static void getFileList(std::string& dir,
			std::vector<std::string>& fileList, std::string& outcome,
			std::string fileType, std::string fnRegex = ".*");
};
}
#endif /* FILEIO_H_ */

