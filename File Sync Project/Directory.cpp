#include "Directory.h"


Directory::Directory(const std::string& dirName):dirName(dirName),dirs(),files()
{
}

void insertDir(Directory& currentDir,const std::string& newDirName)
{
    Directory temp(newDirName);
    currentDir.dirs.insert({newDirName,{newDirName}});
}

void insertFile(Directory& currentDir,const File& file)
{
    currentDir.files.insert({file.name,file});
}