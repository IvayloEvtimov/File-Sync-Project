///////////////////////////////
// Project: Analyzing Directories
// Author: Ivaylo Evtimov
// FN: 45252
/////////////////////////////


#include <iostream>
#include <fstream>
#include <string>
#include <functional>

#include "System.h"
#include "Perform.h"


void start(int argc,char** argv)
{
	if (strcmp(argv[1], "analyze") == 0)
	{
		std::string leftPath;
		std::string rightPath;

		bool blockCopy = false;
		bool hashOnly = false;

		int count = 3;

		// Checks if there are any other special conditions
		if (argv[count][1] != ':')
		{
			if (strcmp(argv[count++], "block") == 0)
				blockCopy = true;
			else if (strcmp(argv[count - 1], "hash-only") == 0)
				hashOnly = true;
			else
			{
				std::cout << "Wrong command!" << std::endl;
				return;
			}
		}

		leftPath = argv[count++];

		// Looks if there is any whitepace in the left directory's path
		while (argv[count][1] != ':')
		{
			if (count >= argc)
			{
				std::cout << "Invalid arguments" << std::endl;
				return;
			}

			leftPath =leftPath+" "+ argv[count++];
		}

		rightPath = argv[count++];

		// If there are more argv left then they are added to the right directory's path
		if (count != argc)
		{
			while (true)
			{
				rightPath = rightPath + " " + argv[count++];

				if (count == argc)
					break;
			}
		}

	//	// If any of the two directories don't have the : symbol
	//	// Then the paths can't be reached
		if (leftPath[1] != ':' || rightPath[1] != ':')
		{
			std::cout << "Invalid path entered" << std::endl;
			return;
		}

		System S(leftPath, rightPath, blockCopy);

		if (strcmp(argv[2], "mirror") == 0)
			S.mirror();
		else if (strcmp(argv[2], "safe") == 0)
			S.safe();
		else if (strcmp(argv[2], "standard") == 0)
			S.standard();
		else
		{
			std::cout << "Wrong type of analyze" << std::endl;
			return;
		}
	}
	else if(strcmp(argv[1],"perform")==0)
	{
		Perform P(argv[2]);
		P.start();
	}
	else
	{
		std::cout << "Wrong command!" << std::endl;
		return;
	}

	//System S("C:\\Users\\Ivo Evtimov\\Desktop\\Directories\\Sample 2 -Overwrite\\DirA", "C:\\Users\\Ivo Evtimov\\Desktop\\Directories\\Sample 2 -Overwrite\\DirB", true);
	//S.mirror();

	//Perform P("analyze.txt");
	//P.start();

	//std::ifstream input("analyze.txt",std::ios::binary);
	//std::ifstream firstFile("C:\\Users\\Ivo Evtimov\\Desktop\\Directories\\Sample 2 -Overwrite\\DirA\\Vikings.mkv", std::ios::binary);
	//std::string temp;
	//temp.resize(67108864);

	//firstFile.read( const_cast<char*>(temp.c_str()), 67108864);
	//const_cast<char*>(temp.c_str())[67108864] = 0;


}

int main(int argc,char** argv)
{
	start(argc,argv);

	return 0;
}