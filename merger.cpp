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
	FILE* inputFile1 = fopen("file1", "rb");
	FILE* inputFile2 = fopen("file2", "rb");
	if(!inputFile1 || !inputFile2) {
		cerr << "Empty file!" << endl;
		return -2;
	}
	//reading both blocks
	fread(&BLOCK_SIZE, sizeof(int), 1, inputFile1);
	int n = BLOCK_SIZE*2;
	int array1[BLOCK_SIZE], array2[BLOCK_SIZE], block_array[n];

	for(int i = 0; i < BLOCK_SIZE; i++) {
		fread(&array1[i], sizeof(int), 1, inputFile1);
	}
	fclose(inputFile1);

	fread(&BOTH_SIZE, sizeof(int), 1, inputFile2);
	if(BLOCK_SIZE != BOTH_SIZE) {
		cerr << "Block size doesn't equal!" << endl;
		return -3;
	}
	for(int i = BLOCK_SIZE; i < n; i++) {
		fread(&array2[i], sizeof(int), 1, inputFile2);
	}
	fclose(inputFile2);
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
	FILE* outputFile1 = fopen("outMerge1", "wb");
	fwrite(&BLOCK_SIZE, sizeof(int), 1, outputFile1);
	for(int i = 0; i < BLOCK_SIZE; i++) {
		fwrite(&block_array[i], sizeof(int), 1, outputFile1);
	}
	fclose(outputFile1);
	FILE* outputFile2 = fopen("outMerge2", "wb");
	fwrite(&BLOCK_SIZE, sizeof(int), 1, outputFile2);
	for(int i = BLOCK_SIZE; i < n; i++) {
		fwrite(&block_array[i], sizeof(int), 1, outputFile2);
	}
	fclose(outputFile2);
	return 0;
}