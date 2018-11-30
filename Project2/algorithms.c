/*=======================================================================================
*	This code was written by:
*								Antonin Aumètre - antonin.aumetre@gmail.com
*								Céline Moureau -  cemoureau@gmail.com
*	For: High Performance Scientific course at ULiège, 2018-19
*	Project 2
*
*	Under GNU General Public License 11/2018
=======================================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/*=====================================================================================
* Contains general purpose algorithms, can be used anywhere
=====================================================================================*/

// Merges two sorted lists and stores the result in a third one. Removes multiple occurences.
// Return the length of the merged list.
int merge_sorted_lists(int list_A[], int list_B[], int list_C[], int size_A, int size_B) {
	int i=0, j=0, k=0;

      // Do the first iteration out of the loop to manage the case k=0
	if (list_A[i] <= list_B[j]) {
		list_C[k] = list_A[i];
		++i;
	}
	else {
		list_C[k] = list_B[j];
		++j;
	}
	++k;
    // Main loop
	while (i < size_A && j < size_B) {
		if (list_A[i] <= list_B[j]) {
			list_C[k] = list_A[i];
			++i;
			++k;
		}
		else if (list_B[j] != list_C[k-1]) {
			list_C[k] = list_B[j];
			++j;
			++k;
		}
		else {
			++j;
		}
	}
    // If one list is exhausted, fill C with the remains of the other
	if (i < size_A) {
		for (int p = i; p < size_A; ++p) {
			if (list_A[p] != list_C[k-1]){
				list_C[k] = list_A[p];
				++k;
			}
		}
	}
	else {
		for (int p = j; p < size_B; ++p) {
			if (list_B[p] != list_C[k-1]){
				list_C[k] = list_B[p];
				++k;
			}
		}
	}
	return k;
}

// Creates a list of the directionnal communications to be done
int * getCommListSlices(unsigned int size){
	int *list = malloc(sizeof(int)*4*(size-1));

	int max_value_begin;
	if (size%2 == 0) max_value_begin = size;
	else max_value_begin = size - 1;

	for (int i = 0; i < max_value_begin; ++i){
		list[i] = i;
	}
	for (int i = 0; i < max_value_begin; ++i){ // Permutations
		if (i%2 == 0) list[max_value_begin+i] = list[i+1];
		else list[max_value_begin+i] = list[i-1];
	}

	int max_value;
	if (size%2 == 0) max_value = size - 1;
	else max_value = size;
	for (int i = 1; i < max_value; ++i){
		list[2*max_value_begin+i-1] = i;
	}
	for (int i = 0; i < max_value-1; ++i){ // Permutations
		if (i%2 ==0) list[2*max_value_begin+max_value-1+i] = list[2*max_value_begin+1+i];
		else list[2*max_value_begin+max_value-1+i] = list[2*max_value_begin-1+i];
	}
	return list;
}

// Natural round-off
double myRound(double value){
	if (ceil(value)-value < value-floor(value)) return ceil(value);
	else return floor(value);
}

// Finds the index at which a key is present in a sorted list and returns it. Returns -1 if the key was not found.
// O(ln(n)+1)
int findIndex(int list[], int key, int size){
	int min = 0, max = size, mid = 0;
	while (min <= max){
		mid = floor((max+min)/2);
		if (list[mid] < key){
			min = mid + 1;
		}
		else if (list[mid] > key){
			max = mid - 1;
		}
		else {
			return mid;
		}
	}
	return -1;
}

// Function that shares the workload
// TODO : use 3 or 4 different numbers of slices, m, n, o, p. Will improve load sharing.
int * shareWorkload(int problemSize, int nNodes){
	int *solution = malloc(3*sizeof(int));
	solution[2] = 1000;

	if (nNodes == 1) {
		solution[0] = 0;
		solution[1] = problemSize;
		solution[2] = problemSize;
		return solution;
	}

	// Get a bunch of solution and score them
	for (int m = 1 ; m < problemSize; ++ m){
		if ((problemSize-m)%(nNodes-1) == 0){
			//printf("m=%d, n=%d, delta=%d\n", m, (problemSize-m)/(nNodes-1), abs(m-(problemSize-m)/(nNodes-1)));
			// Keep the best one (lowest delta)
			if (abs(m-(problemSize-m)/(nNodes-1)) < solution[2]){
				solution[0] = m;
				solution[1] = (problemSize-m)/(nNodes-1);
				solution[2] = abs(m-solution[1]);
			}
		}
	}
	// The first argument is the number of slices to give to the last node
	// The second argument is the number of slices to give to every other nodes
	return solution;
}

// Inserts a key in a list, returns the index, does not prevent redundance.
int insertKey_int(int list[], int key, int size){
	int index = size-1;
	while(index > 0 && key < list[index-1]){
		list[index] = list[index -1];
		-- index;
	}
	list[index] = key;
	return index;
}

// Inserts a key in a list, returns the index, does not prevent redundance.
void insertKeyAt_double(double list[], double key, int size, int target_index){
	for (int i = size-1; i > target_index; --i){
		list[i] = list[i-1]; // Shifting
	}
	list[target_index] = key;
}