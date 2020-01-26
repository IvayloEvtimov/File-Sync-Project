#ifndef PERFORM_H
#define PERFORM_H

#include "File.h"
#include "Directory.h"

// Used when using the perform operation
// By giving a path to file 
// Reads the contents and performs the commands in that file

class Perform
{
public:
	Perform(const std::string& filename);
	Perform(const Perform&) = delete;
	Perform& operator=(const Perform&) = delete;
	~Perform() = default;


	void start();
private:
	// Used when reading a path to the file or directories
	// In order to perform the required operation
	void parse(std::string&, std::ifstream&);
	void parse(std::string&, std::string&,std::ifstream&);
	// Creates a new directory in a given path
	void createDir(std::ifstream&);
	// Deletes a certain directory and all its files if there are any
	void deleteDir(std::ifstream&);
	// Copies a file to a location
	// Overwrites the prevoius one if existing
	// Uses blockCopy if enabled
	void copyFile(std::ifstream&);
	// Deletes a file in a given directory
	void deleteFile(std::ifstream&);
	// Moves a file to a new location and renames it if necessary
	void moveFile(std::ifstream&);

private:
	std::string filename;
	std::string leftSide;
	std::string rightSide;

	bool blockCopy;
};

#endif // !PERFORM_H
