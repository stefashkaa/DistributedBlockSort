/*
	Merger
*/
#include <iostream>
#include <algorithm>
#include <stdlib.h>

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
	int array1[BLOCK_SIZE], array2[BLOCK_SIZE], block_array[n];

	fread(&array1, sizeof(int), BLOCK_SIZE, inputFile);
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
	fread(&array2, sizeof(int), BLOCK_SIZE, inputFile);
	fclose(inputFile);
	delete(inputFile);
	//merging blocks
	int* tmp_array = (int*)malloc(sizeof(int)*(2 * BLOCK_SIZE));

	if (!tmp_array) {
		std::cout << "Memory allocation failed!!!\n";
		exit(1);
	}

	std::merge(&array1[0], &array1[BLOCK_SIZE],
		&array2[0], &array2[BLOCK_SIZE],
		&tmp_array[0]);
	std::copy(&tmp_array[0], &tmp_array[BLOCK_SIZE], &block_array[0]);
	std::copy(&tmp_array[BLOCK_SIZE], &tmp_array[2 * BLOCK_SIZE], &block_array[BLOCK_SIZE]);

	free(tmp_array);
	//writing both blocks
	FILE* outputFile = fopen("outMerge1", "wb");
	fwrite(&BLOCK_SIZE, sizeof(int), 1, outputFile);
	for(int i = 0; i < BLOCK_SIZE; i++) {
		fwrite(&block_array[i], sizeof(int), 1, outputFile);
	}
	fclose(outputFile);
	outputFile = fopen("outMerge2", "wb");
	fwrite(&BLOCK_SIZE, sizeof(int), 1, outputFile);
	for(int i = BLOCK_SIZE; i < n; i++) {
		fwrite(&block_array[i], sizeof(int), 1, outputFile);
	}
	fclose(outputFile);
	return 0;
}