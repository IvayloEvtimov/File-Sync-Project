#include "System.h"

#include <filesystem>
#include <fstream>
#include <unordered_set>

System::System(const std::string& leftDirPath,const std::string& rightDirPath,const bool blockCopy,const bool hashOnly):leftDirPath(leftDirPath),
    leftDir(leftDirPath),
    rightDirPath(rightDirPath),
    rightDir(rightDirPath),
	blockCopy(blockCopy),
	hashOnly(hashOnly)
{
    scanDirectory(leftDir,leftDirPath);
    scanDirectory(rightDir,rightDirPath);
}

void System::mirror()
{

	std::ofstream output("analyze.txt");
	if (!output.is_open())
		return;

	output << "L is " << leftDirPath << std::endl;
	output << "R is " << rightDirPath << std::endl;
	if(blockCopy)
		output << "# Use block copy" << std::endl;
	output << std::endl;

	// Used later for checking folders that are in R but no in L
	std::unordered_set<std::string> includedDirs;
	// Used later for checking files that are in R but not in L
	std::unordered_set<std::string> includedFiles;

	// Files which are marked for copying
	// these files are missing in R
	copyFiles arrFiles;
	
	//Files with are marked for deletion
	// These files are missing in L
	deleteFile mapFiles;

	// Folders which are missing in L
	// These folders are marked for deletion at the end
	// Because there may be files inside the which could be moved to other folder
	// For increasing the speed of the operation and limiting the amount of copying
	std::vector<std::string> redundantFolders;

	// Analyzes for folder and files that are in L but are missing in R
	mirrorAnalyze(leftDir, rightDir, includedDirs, includedFiles,arrFiles, output);
	// Analyzes for folder and files that are in R but are missing in L
	mirrorNotInL(rightDir, includedDirs, includedFiles, mapFiles,redundantFolders, output);


	// If there's a file that needs copying and a file with same content is marked for deletion
	// move the file and rename it if neccessery instead of copying and deleting

	for (copyFiles::iterator copyItr = arrFiles.begin(); copyItr != arrFiles.end(); ++copyItr)
	{
		File& markedForCopy = (*copyItr).first;
		//File& markedForCopy = std::get<0>((*copyItr));
		if (mapFiles.count(markedForCopy.hash) != 0)
		{
			File& markedForDel = mapFiles[((*copyItr)).first.hash];

			output << "MOVE " <<"R:"+ markedForDel.path.substr(rightDirPath.size()) << " " << (*copyItr).second << std::endl;
			mapFiles.erase(markedForDel.hash);
		}
		else
			output << "COPY " <<"L:"+ (*copyItr).first.path.substr(rightDirPath.size()) << " " << (*copyItr).second << std::endl;
	}

	for (deleteFile::iterator delItr = mapFiles.begin(); delItr != mapFiles.end(); ++delItr)
		output << "DELETE " <<"R:"+ (*delItr).second.path.substr(rightDirPath.size()) << " # Missing on one side" << std::endl;

	for (std::string& folder : redundantFolders)
		output<<"DELETE-DIR " << folder << " # Missing in L" << std::endl;

	output.close();
}

void System::safe()
{
	std::ofstream output("analyze.txt");
	if (!output.is_open())
		return;

	output << "L is " << leftDirPath << std::endl;
	output << "R is " << rightDirPath << std::endl;
	output << std::endl;

	// Looks for directories and files which are missing in R
	// But not removing or changing any existing ones
	safeAnalyze(leftDir, rightDir, leftDirPath, rightDirPath, output,"R:");
	// Looks for directories and files which are missing in L
	// But not removing or changing any existing ones
	safeAnalyze(rightDir, leftDir, rightDirPath, leftDirPath, output,"L:");	

	output.close();
}

void System::standard()
{
	std::ofstream output("analyze.txt");
	if (!output.is_open())
		return;

	output << "L is " << leftDirPath << std::endl;
	output << "R is " << rightDirPath << std::endl;
	output << std::endl;

	standartAnalyze(leftDir, rightDir, leftDirPath, rightDirPath, output,"R:");
	standartAnalyze(rightDir, leftDir, rightDirPath, leftDirPath, output, "L:");

	output.close();
}

// Makes a tree consisting of files and folders
void System::scanDirectory(Directory& currentDir,const std::string& dirName )
{
    for(auto& itr : std::filesystem::directory_iterator(dirName))
    {   // If there's another directory
		if (itr.is_directory())
		{   // Adds it to the rest and scan its contents
			currentDir.dirs.insert({ itr.path().string().substr(dirName.size()), {itr.path().string().substr(dirName.size())} } );
            scanDirectory(currentDir.dirs[itr.path().string().substr(dirName.size())],itr.path().string());
        }else // Adds the files to the tree
            currentDir.files.insert( {itr.path().string().substr(dirName.size()), {itr.path().string()} } );
    }
}

// Analyze for mirror scaning
void System::mirrorAnalyze(Directory& firstDir,Directory& secondDir,std::unordered_set<std::string>& includedDirs,
	std::unordered_set<std::string>& includedFiles,
	copyFiles& arrFiles,std::ofstream& output)
{
    // Check all the directoris in first Dir 
    // and mark those that aren't
    for(directories::iterator dirItr=firstDir.dirs.begin();dirItr!=firstDir.dirs.end();++dirItr)
    {
        if(secondDir.dirs.count((*dirItr).first)!=0)
        {	// Mark this directory because it is both in L and R
            includedDirs.insert((*dirItr).first);
            mirrorAnalyze((*dirItr).second,secondDir.dirs[(*dirItr).first],includedDirs,includedFiles,arrFiles,output);
        }else if(secondDir.dirs.count((*dirItr).first)==0)
			output << "CREATE-DIR " << "R:" + (*dirItr).first.substr(leftDirPath.size()) << " # Missing in R" << std::endl;
        
    }

    for(filesInDir::iterator filesItr=firstDir.files.begin();filesItr!=firstDir.files.end();++filesItr)
    {
        if(secondDir.files.count((*filesItr).first)!=0)
        {
            includedFiles.insert((*filesItr).first);
            File& leftTemp=(*filesItr).second;
            File& rightTemp=secondDir.files[(*filesItr).first];

			if (!hashOnly && leftTemp.hash == rightTemp.hash && leftTemp.size == rightTemp.size && leftTemp.lastModified == rightTemp.lastModified)
			{
				if (!compareByteByByte(leftTemp, rightTemp))
					arrFiles.push_back({ leftTemp,"R:" + secondDir.files[(*filesItr).first].path.substr(rightDirPath.size()) + " # File in L is newer" });
			}
            // If there is any discrepancy between any of the properties of the two files the right files becomes copy of the left
			else if (leftTemp.hash != rightTemp.hash || leftTemp.size != rightTemp.size || leftTemp.lastModified != rightTemp.lastModified)
			{
					arrFiles.push_back({ leftTemp,"R:"+secondDir.files[(*filesItr).first].path.substr(rightDirPath.size())+" # File in L is newer" });
									//Full path of the file on the left	 //Directory path of the right folder + realtive path of the left file to be copied 
			}

		}
		else	//Full path of the file on the left			 //Directory path of the right folder + realtive path of the left file to be copied
				arrFiles.push_back({ (*filesItr).second,"R:" + (*filesItr).second.path.substr(leftDirPath.size())+" # Missing in R" });
        
    }
}

void System::mirrorNotInL(Directory &firstDir, std::unordered_set<std::string>&includedDirs,
	std::unordered_set<std::string>&includedFiles, deleteFile &mapFiles,
	std::vector<std::string>& redundantFolders, std::ofstream &output)
{
	// Looks for directories that are in R but aren't included in L
	for (directories::iterator dirItr = firstDir.dirs.begin(); dirItr != firstDir.dirs.end(); ++dirItr)
	{
		mirrorNotInL((*dirItr).second,  includedDirs, includedFiles, mapFiles,redundantFolders, output);
		if (includedDirs.count((*dirItr).first) == 0)
			redundantFolders.push_back("R:" + (*dirItr).first);
	}

	// Looks for files that are in R but are not included in L
	for (filesInDir::iterator fileItr = firstDir.files.begin(); fileItr != firstDir.files.end(); ++fileItr)
		if (includedFiles.count((*fileItr).first) == 0)
			mapFiles.insert({ (*fileItr).second.hash, (*fileItr).second });
}

void System::safeAnalyze(Directory &firstDir, Directory &secondDir,const std::string& firstDirPath,
	const std::string& secondDirPath, std::ofstream &output,
	const std::string& direction)
{
	std::string opositeDirection;

	if (direction == "R:")
		opositeDirection = "L:";
	else
		opositeDirection = "R:";

	for (directories::iterator dirItr = firstDir.dirs.begin(); dirItr != firstDir.dirs.end(); ++dirItr)
	{
		if (secondDir.dirs.count((*dirItr).first) != 0)
			safeAnalyze((*dirItr).second, secondDir.dirs[(*dirItr).first], firstDirPath, secondDirPath, output, direction);
		else
		{
			output << "CREATE-DIR " << direction + (*dirItr).first << " # Missing in " << direction << std::endl;
			emptyFolder((*dirItr).second, firstDirPath, secondDirPath, output, direction);
		}
	}

	for (filesInDir::iterator filesItr = firstDir.files.begin(); filesItr != firstDir.files.end(); ++filesItr)
	{
		if (secondDir.files.count((*filesItr).first) == 0)
				output << "COPY " << opositeDirection +(*filesItr).second.path.substr(firstDirPath.size()) << " " << direction + (*filesItr).second.path.substr(firstDirPath.size()) << " # Missing in "<<direction << std::endl;
		else
		{
			File& firstFile = (*filesItr).second;
			File& secondFile = secondDir.files[(*filesItr).first];

			if (!hashOnly && firstFile.hash == secondFile.hash &&  firstFile.size == secondFile.size && firstFile.lastModified == secondFile.lastModified)
			{
				if (!compareByteByByte(firstFile, secondFile))
					output << "COPY" << opositeDirection + firstFile.path.substr(firstDirPath.size()) << " " << direction + firstFile.path.substr(firstDirPath.size());
			}
			else if(firstFile.hash == secondFile.hash ||  firstFile.size == secondFile.size || firstFile.lastModified == secondFile.lastModified)
				output << "COPY" << opositeDirection + firstFile.path.substr(firstDirPath.size()) << " " << direction + firstFile.path.substr(firstDirPath.size());
		}
	}
}

void System::standartAnalyze(Directory &firstDir, Directory &secondDir,
	const std::string &firstPath,const std::string &secondPath,
	std::ofstream &output,const std::string& direction)
{
	std::string opositeDirection;

	if (direction == "R:")
		opositeDirection = "L:";
	else
		opositeDirection = "R:";

	for (directories::iterator dirItr = firstDir.dirs.begin(); dirItr != firstDir.dirs.end(); ++dirItr)
	{
		if (secondDir.dirs.count((*dirItr).first) != 0)
			standartAnalyze((*dirItr).second, secondDir.dirs[(*dirItr).first], firstPath, secondPath, output, direction);
		else
		{
			output << "CREATE-DIR " << direction + (*dirItr).first << " # Missing in " << direction << std::endl;
			emptyFolder((*dirItr).second, firstPath, secondPath, output, direction);
		}
	}

	for (filesInDir::iterator filesItr = firstDir.files.begin(); filesItr != firstDir.files.end(); ++filesItr)
	{
		if (secondDir.files.count((*filesItr).first) != 0)
		{
			File& firstFile = (*filesItr).second;
			File& secondFile = secondDir.files[(*filesItr).first];

			if (!hashOnly && firstFile.hash == secondFile.hash && firstFile.size == secondFile.size)
			{
				if(!compareByteByByte(firstFile,secondFile) && firstFile.lastModified < secondFile.lastModified)
					output << "COPY " << opositeDirection + firstFile.path.substr(firstPath.size()) << " " << direction + secondFile.path.substr(secondPath.size()) << " # File in " << direction << " is newer" << std::endl;
			}
			else if (firstFile.hash != secondFile.hash || firstFile.size != secondFile.size)
			{
				if (firstFile.lastModified < secondFile.lastModified)
					output << "COPY " << opositeDirection + firstFile.path.substr(firstPath.size()) << " " << direction + secondFile.path.substr(secondPath.size()) << " # File in " << direction << " is newer" << std::endl;
			}
		}
		else
				output << "COPY " <<opositeDirection + (*filesItr).second.path.substr(firstPath.size()) << " " << direction+ (*filesItr).second.path.substr(firstPath.size()) << " # Missing in " << direction << std::endl;
	}
}

void System::emptyFolder(Directory &firstDir, const std::string &firstPath, const std::string &secondPath, std::ofstream &output, const std::string& direction)
{
	std::string opositeDirection;

	if (direction == "R:")
		opositeDirection = "L:";
	else
		opositeDirection = "R:";

	for (directories::iterator dirItr = firstDir.dirs.begin(); dirItr != firstDir.dirs.end(); ++dirItr)
	{
		output << "CREATE-DIR " << direction + (*dirItr).first << " # Missing in " << direction << std::endl;
		emptyFolder((*dirItr).second, firstPath, secondPath, output, direction);
	}

	for (filesInDir::iterator filesItr = firstDir.files.begin(); filesItr != firstDir.files.end(); ++filesItr)
			output << "COPY " << opositeDirection  + (*filesItr).second.path.substr(firstPath.size()) << " " << direction+ (*filesItr).second.path.substr(firstPath.size()) << " # Missing in " << direction << std::endl;
}

bool System::compareByteByByte(const File &firstFile, const File &secondFile)
{
	std::ifstream firstInput(firstFile.path,std::ios::binary);
	std::ifstream secondInput(secondFile.path,std::ios::binary);

	if (!firstInput.is_open() || !secondInput.is_open())
	{
		std::cout << "One or two of the files coudn't be opened for reading" << std::endl;
		return false;
	}

	char* firstChar = new char;
	char* secondChar = new char;

	while (true)
	{

		firstInput.read(firstChar, sizeof(char));
		secondInput.read(secondChar, sizeof(char));

		if (firstInput.eof() || secondInput.eof())
			break;

		if (strcmp(firstChar, secondChar) != 0)
		{
			delete firstChar;
			firstInput.close();
			delete secondChar;
			secondInput.close();
			return false;
		}
	}

	return true;
}
