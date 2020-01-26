#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <unordered_map>
#include <string>

#include "File.h"

//Structure respresenting a tree 
// consisting of files and folders in the directory

// unordered_map used when comparing with an other directory
// for faster search in the other dir instead of looking through all the files

struct Directory
{
	Directory() :dirName(" "), dirs(), files() {}
    Directory(const std::string&);


    std::string dirName;
    std::unordered_map<std::string,Directory> dirs;
    std::unordered_map<std::string,File> files;

};

void insertDir(Directory&,const std::string&);
void insertFile(Directory&,const File&);

#endif // !DIERCTORY_H
