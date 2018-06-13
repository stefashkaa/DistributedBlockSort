/*
	Merger
*/
#include <iostream>
#include <algorithm>
#include <stdlib.h>
#include <cstring>
#include <vector>

using namespace std;

int main(int argc, char *argv[])
{
	//system("uname -a");
	if(argc < 3) {
		cerr << "Empty parameters!" << endl;
		return -1;
	}
	int BLOCK_SIZE = 0, BOTH_SIZE = 0;
	//reading both blocks
	FILE* inputFile = fopen("file1", "rb");

	if(!inputFile) {
		cerr << "Empty first file!" << endl;
		return -2;
	}
	fread(&BLOCK_SIZE, sizeof(int), 1, inputFile);
	int n = BLOCK_SIZE*2;
	vector<int> block_array(n);

	for(int i = 0; i < BLOCK_SIZE; i++) {
		int value;
		fread(&value, sizeof(int), 1, inputFile);
		block_array[i] = value;
	}
	fclose(inputFile);

	inputFile = fopen("file2", "rb");

	if(!inputFile) {
		cerr << "Empty second file!" << endl;
		return -2;
	}
	fread(&BOTH_SIZE, sizeof(int), 1, inputFile);
	if(BLOCK_SIZE != BOTH_SIZE) {
		cerr << "Block size doesn't equal!" << endl;
		return -3;
	}
	for(int i = BLOCK_SIZE; i < n; i++) {
		int value;
		fread(&value, sizeof(int), 1, inputFile);
		block_array[i] = value;
	}
	fclose(inputFile);
	delete(inputFile);
	//merging blocks
	int* tmp_array = (int*)malloc(sizeof(int)*(2 * BLOCK_SIZE));

	if (!tmp_array) {
		cout << "Memory allocation failed!!!" << endl;
		exit(1);
	}

	std::merge(&block_array[0], &block_array[BLOCK_SIZE],
		&block_array[BLOCK_SIZE], &block_array[n],
		&tmp_array[0]);
	std::copy(&tmp_array[0], &tmp_array[BLOCK_SIZE], &block_array[0]);
	std::copy(&tmp_array[BLOCK_SIZE], &tmp_array[n], &block_array[BLOCK_SIZE]);

	free(tmp_array);
	//writing both blocks into files
	char *iString = strdup("outMerge1-"), *jString = strdup("outMerge2-");
	strcat(iString, argv[3]);
	strcat(jString, argv[4]);

	FILE* outputFile = fopen(iString, "wb");
	fwrite(&BLOCK_SIZE, sizeof(int), 1, outputFile);
	for(int i = 0; i < BLOCK_SIZE; i++) {
		fwrite(&block_array[i], sizeof(int), 1, outputFile);
	}
	fclose(outputFile);

	outputFile = fopen(jString, "wb");
	fwrite(&BLOCK_SIZE, sizeof(int), 1, outputFile);
	for(int i = BLOCK_SIZE; i < n; i++) {
		fwrite(&block_array[i], sizeof(int), 1, outputFile);
	}
	fclose(outputFile);

	return 0;
}