/*
	Sorter
*/
#include <iostream>
#include <algorithm>
#include <stdlib.h>

using namespace std;

int main(int argc, char *argv[])
{
	//system("uname -a");
	if(argc < 2) {
		cerr << "Empty parameters!" << endl;
		return -1;
	}
	//reading block
	FILE* inputFile = fopen(argv[1], "rb");

	if(!inputFile) {
		cerr << "Empty file!" << endl;
		return -2;
	}
	int n;
	fread(&n, sizeof(int), 1, inputFile);
	int array[n];

	for(int i = 0; i < n; i++) {
		fread(&array[i], sizeof(int), 1, inputFile);
	}
	fclose(inputFile);
	delete(inputFile);
	//sorting block
	sort(&array[0], &array[n]);
	//writing block into file
	FILE* outputFile = fopen("outSort", "wb");
	fwrite(&n, sizeof(int), 1, outputFile);

	for(int i = 0; i < n; i++) {
		fwrite(&array[i], sizeof(int), 1, outputFile);
	}
	fclose(outputFile);

	return 0;
}