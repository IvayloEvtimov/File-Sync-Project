#ifndef SYSTEM_H
#define SYSTEM_H

#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>


#include "Directory.h"
#include "File.h"



using directories= std::unordered_map<std::string,Directory>;
using filesInDir =  std::unordered_map<std::string,File>;

// All the files marked for copying when performing mirror analyze
using copyFiles = std::vector<std::pair<File, std::string>>;
// All the files marked for deletion when performing mirror analyze
using deleteFile = std::unordered_map<size_t, File>;
// Used when one file is marked for deletion 
// and another file with the same content is marked for copying
// the files are swapped for faster operation


class System
{
public:
    System(const std::string&,const std::string&,const bool=false,const bool =true);
    System(const System&)=delete;
    System& operator=(const System&)=delete;
    ~System()=default;

    void mirror(); // Makes the right directory the exact copy of the left one
    void safe(); // If a file a missing in one directory and not in the other it is copied but deletion is forbidden
	void standard();// Same as safe, but if a newer copy is in one of the directories it is copied to the other
private:
    void scanDirectory(Directory&,const std::string&); // Makes a tree based on the files and folders in the direct
    void mirrorAnalyze(Directory&,Directory&,std::unordered_set<std::string>&,std::unordered_set<std::string>&,copyFiles&,std::ofstream&);
    // Looks into directory R and marks files and folders that aren't in L
	void mirrorNotInL(Directory&, std::unordered_set<std::string>&, std::unordered_set<std::string>&, deleteFile&,std::vector<std::string>&,std::ofstream&);
	void safeAnalyze(Directory&, Directory&, const std::string&, const std::string&, std::ofstream&,const std::string&);
	void standartAnalyze(Directory&, Directory&, const std::string&, const std::string&, std::ofstream &, const std::string& );
	// When empty folder is found in one directory
    // All of its folders and files are copied to the other directory
    // Without checking 
    void emptyFolder(Directory&,const std::string&, const std::string&, std::ofstream&, const std::string&);
	// Used when comparing two files byte by byte
	// In case there is a collission when hashing
	bool compareByteByByte(const File&, const File&);
private:
    std::string leftDirPath;
    Directory leftDir;

    std::string rightDirPath;
    Directory rightDir;

	const bool blockCopy;
	const bool hashOnly;
};

#endif // !SYSTEM_H
