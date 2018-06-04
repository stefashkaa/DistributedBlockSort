/*
	Is Sorted
*/
#include <iostream>
#include <stdlib.h>
#include <list>
#include <vector>
#include <fstream>

using namespace std;
vector<int> array;

bool isSorted()
{
	int prev = array.at(0);

	for(int i = 1; i < (int)array.size(); i++) {
		if (prev > array.at(i)) {
			return false;
		}
		prev = array.at(i);
	}
	return true;
}

int main(int argc, char *argv[])
{
	//system("uname -a");
	if(argc < 3) {
		cerr << "Wrong parameters!" << endl;
		return -1;
	}
	int BLOCK_SIZE = 0;

	for(int i = 1; i < argc; i++) {
		FILE* inputFile = fopen(argv[i], "rb");
		fread(&BLOCK_SIZE, sizeof(int), 1, inputFile);
		for(int j = 0; j < BLOCK_SIZE; j++) {
			int element = 0;
			fread(&element, sizeof(int), 1, inputFile);
			array.push_back(element);
		}
		fclose(inputFile);
	}
	ofstream out("answer");

	if (isSorted()) {
		out << "is sorted = true";
	} else {
		out << "is sorted = false";
	}
	out.close();
	return 0;
}