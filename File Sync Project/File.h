#ifndef FILE_H
#define FILE_H

#include <filesystem>
#include <iostream>

// A structure respresenting the basic information about a file

struct File 
{
	File();
    File(const std::string&);

	// Only the filename is saved
	// Used later the hashing the name of the File in the Direcory struct
    std::string name;
	// The absolute path to the file
    std::string path;
	// The size of the fily in bytes
    size_t size;
	// When the last change in the file
	std::filesystem::file_time_type lastModified;
	// The of the file's content using std::hash
    size_t hash;

};

#endif // !FILE_H
