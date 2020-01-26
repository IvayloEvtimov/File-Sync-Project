#include "File.h"

#include <fstream>
#include <iostream>
#include <functional>

File::File():name(" "),path(" "),size(0),lastModified(),hash(0)
{
}

File::File(const std::string& path) : path(path)
{
	std::filesystem::path filePath(path);

	name = filePath.filename().string();
	size = std::filesystem::file_size(filePath);
	lastModified = std::filesystem::last_write_time(filePath);

	// Opens the file for reading in order to generate a hash based on its contents
	std::ifstream input(filePath.string(), std::ios::binary);
	if (!input.is_open())
		throw std::invalid_argument("Can't open file!");

	char* temp = new char[std::filesystem::file_size(filePath) + 1];

	input.read(temp, std::filesystem::file_size(filePath));
	temp[std::filesystem::file_size(filePath)] = 0;
	input.close();

	std::string hashStr = temp;
	delete[] temp;
	hash = std::hash<std::string>{}(hashStr);
}