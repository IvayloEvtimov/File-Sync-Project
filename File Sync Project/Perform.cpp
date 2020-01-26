#include "Perform.h"

#include <iostream>
#include <fstream>
#include <filesystem>

// Used for block Copy
// Sample size
const int blockSize = 67108864; // 64 MB

Perform::Perform(const std::string & filename):filename(filename),leftSide(),rightSide(),blockCopy(false)
{
	std::ifstream input(filename);
	if (!input.is_open())
		throw std::invalid_argument("Invalid filename given");

	// Reads the absolute path to the directories
	// substr remove a whitespace
	// which getline takes

	std::string temp;
	input >> temp>>temp;
	std::getline(input, leftSide, '\n');
	leftSide=leftSide.substr(1);

	input >> temp >> temp;
	std::getline(input, rightSide, '\n');
	rightSide=rightSide.substr(1);

	// Looks for special command
	// Block copy is used for faster overwriting of files
	getline(input, temp, '\n');
	if (temp == "# Use block copy")
		blockCopy = true;

	input.close();
}

void Perform::start()
{ 
	std::ifstream input(filename);
	if (!input.is_open())
		throw std::invalid_argument("Can't open the file");

	std::string temp;
	std::string command;
	std::string src;
	std::string outp;

	// Skips the first few lines
	// which tells the absolute path to the directories
	// and a special commands if existing
	getline(input, temp, '\n');
	getline(input, temp, '\n');
	getline(input, temp, '\n');
	if (temp[0] == '#')
		getline(input, temp, '\n');

	try
	{
		while (true)
		{	// Reads the next command that has to be completed
			input >> command;

			if (input.eof())
				return;

			if (command == "CREATE-DIR")
				createDir(input);
			else if (command == "DELETE-DIR")
				deleteDir(input);
			else if (command == "COPY")
				copyFile(input);
			else if (command == "DELETE")
				deleteFile(input);
			else if (command == "MOVE")
				moveFile(input);

		}

		input.close();
	}
	catch (std::filesystem::filesystem_error &err)
	{
		input.close();
		std::cerr << err.what() << std::endl;
	}

}

// Used for parsing the relative path to the files or directories
// when there are whitespaces between the names
void Perform::parse(std::string & path, std::ifstream & input)
{
	std::string temp;

	input >> temp;

	do
	{
		path = path + temp;
		input >> temp;
	} while (temp[0] != '#');
}


// Same as before but for when two paths are requied for a successful operation
void Perform::parse(std::string &firstPath, std::string &secondPath, std::ifstream & input)
{
	std::string temp;

	input >> temp;
	firstPath += temp;
	while (true)
	{
		input >> temp;
		if (input.eof())
			throw std::invalid_argument("File with operations is invalid!");
		// The symbol ':' is used only when a root directory is behind it
		// Elsewhere it is forbidden
		// It marks that this a new path
		if (temp[1] == ':')
			break;
		firstPath += temp;
	}

	secondPath += temp;
	input >> temp;

	// In the output file the '#' symbol is used for explaining why the command is executed
	while (true)
	{
		if (temp[0] == '#' || temp[0] == '\n')
			break;
		secondPath += temp;
		input >> temp;
	}

}

void Perform::createDir(std::ifstream & input)
{
	std::string newDir;

	parse(newDir, input);

	// Makes the directory path full by substituting
	// the 'L' and 'R' markers with the full path
	if (newDir[0] == 'L')
		newDir = leftSide + newDir.substr(2);
	else
		newDir = rightSide + newDir.substr(2);

	std::filesystem::create_directory(newDir);
	std::cout << "New Directory created in: " << newDir << std::endl;
	input.ignore(blockSize, '\n');
}

void Perform::deleteDir(std::ifstream &input)
{
	std::string delDir;

	parse(delDir, input);

	// Makes the directory path full by substituting
	// the 'L' and 'R' markers with the full path
	if (delDir[0] == 'L')
		delDir = leftSide + delDir.substr(2);
	else
		delDir = rightSide + delDir.substr(2);

	std::filesystem::remove_all(delDir);
	std::cout << "Directory " << delDir << " is removed" << std::endl;
	input.ignore(blockSize, '\n');
}

void Perform::copyFile(std::ifstream &input)
{
	std::string srcFile;
	std::string targetFile;

	parse(srcFile, targetFile, input);

	// Makes the directory path full by substituting
	// the 'L' and 'R' markers with the full path

	if (srcFile[0] == 'L')
		srcFile = leftSide + srcFile.substr(2);
	else
		srcFile = rightSide + srcFile.substr(2);

	if (targetFile[0] == 'L')
		targetFile = leftSide + targetFile.substr(2);
	else
		targetFile = rightSide + targetFile.substr(2);


	// Look for the instuctions left by the analyzing step
	// When the target file already exists then it must be checked for permissions
	std::string instruction;
	bool overwrite = false;

	std::getline(input, instruction, '\n');

	if (instruction == " File in L is newer" || instruction == " File in R is newer")
		overwrite = true;

	bool fail = false;

	// Checks if the files can be opened for the copy opration
	// perms::all is used because when owner::read, group::read and other::read
	// is used it gives an error 
	if (std::filesystem::status(srcFile).permissions() != std::filesystem::perms::all)
	{
		std::cout << "Can't open " << srcFile << "for reading " << std::endl;
		fail = true;
	}

	if (overwrite && blockCopy && std::filesystem::status(targetFile).permissions() != std::filesystem::perms::all)
	{
		std::cout << "Can't open" << targetFile << " for reading and writing" << std::endl;
		fail = true;
	}

	if (fail)
		return;

	// To used block Copy it must be enabled and the target file
	// must already exist and be able to be opened for reading and writing
	// and the source file size must be larger than the block size
	// because if it isn't it would be a normal copy
	// else normal copy is used
	if (overwrite && blockCopy && std::filesystem::file_size(srcFile) > blockSize)
	{
		size_t sizeFirst = std::filesystem::file_size(srcFile);
		size_t sizeSecond = std::filesystem::file_size(targetFile);

		std::ifstream firstFile(srcFile, std::ios::binary);
		std::fstream secondFile(targetFile, std::ios::binary | std::ios::in | std::ios::out);

		if (!firstFile.is_open() || !secondFile.is_open())
		{
			std::cout << "Coudn't open files for copy operation" << std::endl;
			return;
		}

		int writePos = 0;

		while (true)
		{
			std::string str1;
			str1.resize(blockSize);
			std::string str2;
			str2.resize(blockSize);

			// Calculates the remaining size left to read
			// so that the exact length is read

			if (sizeFirst < blockSize)
			{

				firstFile.read(const_cast<char*>(str1.c_str()), sizeFirst);
				const_cast<char*>(str1.c_str())[sizeFirst] = 0;
			}
			else
			{
				firstFile.read(const_cast<char*>(str1.c_str()), blockSize);
				const_cast<char*>(str1.c_str())[blockSize] = 0;
			}

			if (sizeSecond < blockSize)
			{
				secondFile.read(const_cast<char*>(str2.c_str()), sizeSecond);
				const_cast<char*>(str2.c_str())[sizeSecond] = 0;
			}
			else
			{
				secondFile.read(const_cast<char*>(str2.c_str()), blockSize);
				const_cast<char*>(str2.c_str())[blockSize] = 0;
			}


			if (std::hash<std::string>{}(str1) != std::hash<std::string>{}(str2)) {
				secondFile.seekp(writePos);

				// Since the target must become like the source
				// and for the last block the size of the source is taken

				if (sizeFirst < blockSize)
					secondFile.write(str1.c_str(), sizeFirst);
				else
					secondFile.write(str1.c_str(), blockSize);
			}

			writePos += blockSize;

			if (sizeFirst < blockSize)
				break;

			sizeFirst -= blockSize;
			sizeSecond -= blockSize;
		}

		firstFile.close();
		secondFile.close();
	}
	else
		std::filesystem::copy_file(srcFile, targetFile, std::filesystem::copy_options::overwrite_existing);
	// Copy the source file to the target location even if it overwrites the existing one
}

void Perform::deleteFile(std::ifstream &input)
{
	std::string srcFile;

	parse(srcFile, input);

	// Makes the directory path full by substituting
	// the 'L' and 'R' markers with the full path
	if (srcFile[0] == 'L')
		srcFile = leftSide + srcFile.substr(2);
	else
		srcFile = rightSide + srcFile.substr(2);

	if (std::filesystem::status(srcFile).permissions() != std::filesystem::perms::all)
	{
		std::cout << "Can't open " << srcFile << std::endl;
		return;
	}
	
	std::filesystem::remove(srcFile);
	std::cout << "File: " << srcFile << " deleted" << std::endl;
	input.ignore(blockSize, '\n');
}

void Perform::moveFile(std::ifstream &input)
{
	std::string oldPath;
	std::string newPath;

	parse(oldPath, newPath, input);

	// Makes the directory path full by substituting
	// the 'L' and 'R' markers with the full path

	if (oldPath[0] == 'L')
		oldPath = leftSide + oldPath.substr(2);
	else
		oldPath = rightSide + oldPath.substr(2);

	if (newPath[0] == 'L')
		newPath = leftSide + newPath.substr(2);
	else
		newPath = rightSide + newPath.substr(2);

	

	if (std::filesystem::status(oldPath).permissions() != std::filesystem::perms::all) //Adds catch exceptions !!!!
	{
		std::cout << "Can't open " << oldPath << std::endl;
		return;
	}

	// In filesystem library the rename method is moves files and fodlers
	std::filesystem::rename(oldPath, newPath);
	std::cout << "File " << oldPath << " renamed to: " << newPath<<std::endl;
	input.ignore(blockSize, '\n');
}
