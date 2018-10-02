#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <malloc.h>

#define DEBUG 0

/*=======================================================================================
*	This code was written by: Antonin Aumètre - antonin.aumetre@gmail.com
*	For: High Performance Scientific course at ULiège, 2018-19
*	Project 1
*
*	Under GNU General Public License 09/2018
=======================================================================================*/


int main(int argc, char **argv){
	//======================= PRE-PROCESSING ===========================//
	//Reads the content of the image file and stores it in an array
	/*TODO :
		- do everything in a cleaner way
		- generalize offset size
	*/
	
	printf("Usage: ./Main <file_name.ppm> <Fs> <Fl>\n-----------------\n");
	//Get some parameters
	char *filename = argv[1];
	int Fs = atoi(argv[2]);
	int Fl = atoi(argv[3]);

	printf("File: %s\n", filename);
	printf("Fs: %d\n", Fs);
	printf("Fl: %d\n", Fl);

	int depth = 0, width = 0, height = 0;
	long fileLength = 0;

	//Read the file
	FILE *pFile; //File pointer
	pFile = fopen(filename, "rb"); //Opens the file  
	if (pFile == NULL) {//Checks if the file was opened correctly
		printf("Error while opening the file.\n");
		exit(0);
	}

	//Read the next parameters as one string
	char c, str[40];
	int j=0; // CHANGE THIS VARIABLE'S NAME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	for (int i=0; i< 4; ++i){
			c = fgetc(pFile);
		while ((c != '\n') && (c != '\r')&&(c != '\0') && (c != ' ')){
			str[j] = c;
			c = fgetc(pFile);
			++j;
		} 
		str[j] = ' ';
		++j;
	}
	str[j+1] = '\0';

	//Separates the strings and assign values
	char s_width[8], s_height[8], s_depth[4], s_magic[3];
	j = 0;
	int rep = 0;

	//Make a fucking loop !!
	while ((str[j] != ' ') && (str[j] != '\r') && (str[j] != '\0') && (str[j] != '\n')){
		s_magic[j-rep] = str[j];
		++j;
	}
	s_magic[j-rep] = '\0';
	if ((s_magic[0] != 'P') || (s_magic[1] != '6')){
		printf("Wrong file format. Aborting ...\n");
		exit(0);
	}
	++j;
	rep = j;

	while ((str[j] != ' ') && (str[j] != '\r') && (str[j] != '\0') && (str[j] != '\n')) {
		s_width[j-rep] = str[j];
		++j;
	}
	s_width[j] = '\0';
	width = atoi(s_width);
	++j;
	rep = j;
	while ((str[j] != ' ') && (str[j] != '\r') && (str[j] != '\0') && (str[j] != '\n')){
		s_height[j-rep] = str[j];
		++j;
	}
	s_height[j-rep] = '\0';
	height = atoi(s_height);
	++j;
	rep = j;

	while ((str[j] != ' ') && (str[j] != '\r') && (str[j] != '\0') && (str[j] != '\n')){
		s_depth[j-rep] = str[j];
		++j;
	}
	s_depth[j-rep] = '\0';
	depth = atoi(s_depth);

	printf("Image properties\n-----------------\nWidth : %d\nHeight: %d\nColor depth: %d\n-----------------\n", width, height, depth);
	
	//Getting the file length
	fseek(pFile, 0, SEEK_END);
	fileLength = ftell(pFile);
	if(DEBUG)printf("File length: %ld\n", fileLength);
	fseek(pFile, 0, SEEK_SET);

	//Allocate memory
	unsigned char * buffer = (char *)malloc(fileLength+1); //Creates a buffer
	if(DEBUG)printf("Buffer created\n");
	//Read
	fread(buffer, fileLength+1, 1, pFile);
	fclose(pFile);

	
	int offset = j+1; //Number of bytes preceeding the image data
	printf("The offset is: %d\n", offset);
	//unsigned char pic[height][width][3]; //(x, y, c)
	unsigned char * pic = (unsigned char *)malloc(3*height*width*sizeof(char));


	//Create an array of integers instead of binary values
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			for (int k = 0; k < 3 ; ++k) {
				pic[3*(i*width + j) + k] = buffer[offset + 3*(i*width + j) + k];
			}
		}
	}

	free(buffer);
	clock_t begin = clock();

	//======================= ALGORITHM ===========================//
	/*
	Input :
		- filter size Fs
		- filter level Fl
		- width and height extracted from the file
		- array pic[width][height]
	Output :
		- array containing the filtered image
	*/
	
	printf("Algorithm started...\n");
	//unsigned char newPic[height][width][3];
	unsigned char * newPic = (unsigned char *)malloc(3*height*width*sizeof(char));

	int k=0;
	//Creates a square mask
	int adresses[4*Fs*Fs][2]; //Declare a larger than needed array
	for (int a = Fs; a >= -Fs; --a) {
		for (int b = Fs; b >= -Fs; --b) {
			if (a*a + b*b <= Fs*Fs) {
				//Append (a,b) to adresses
				adresses[k][0] = a;
				adresses[k][1] = b;
				++k;
			}
		}
	} //We now have k neighbors

	//Applying the algorithm to the whole image
	for (int i = 0; i < height; ++i){
		if (DEBUG == 0){
			int progress = (int)100*(i+1.0)/height;
			printf("%d %%\n", progress);
		}
		for (int j = 0; j < width; ++j) {
			//Get the neighboring pixels for (i,j)
			//REMARK : the neighbors include (i,j)
			if (DEBUG)printf("Line: %d, row: %d\n", i, j);

			int actual_neighbors = 0;
			int temp_Pic[k][3];//Create a local copy of the useful portion of the image
			for (int l = 0; l < k; ++l) {
				//Check if the neighbor is not outside the picture
				if ((i + adresses[l][1] >= 0) && (i + adresses[l][1] < height) && (j + adresses[l][0] >= 0) && (j + adresses[l][0] < width)) {
					temp_Pic[actual_neighbors][0] = pic[3*((i + adresses[l][1])*width + j + adresses[l][0]) + 0];
					temp_Pic[actual_neighbors][1] = pic[3*((i + adresses[l][1])*width + j + adresses[l][0]) + 1];
					temp_Pic[actual_neighbors][2] = pic[3*((i + adresses[l][1])*width + j + adresses[l][0]) + 2];
					actual_neighbors++;
				}
			}

			if (DEBUG){
				printf("Actual neighbors: %d\n", actual_neighbors);
				for (int l = 0; l < actual_neighbors; ++l) {
					printf("Position(%d:%d) ", adresses[l][0], adresses[l][1]);
					printf("Color: %d %d %d\n", temp_Pic[l][0], temp_Pic[l][1], temp_Pic[l][2]);
				}
			}

			//Compute the color intensity (int)
			int intensities[actual_neighbors];
			for (int l = 0; l < actual_neighbors; ++l) {
				intensities[l] = floor((temp_Pic[l][0] + temp_Pic[l][1] + temp_Pic[l][2]) / (3 * Fl));
			}
			
			if (DEBUG){
				printf("Intensities: ");
				for (int l = 0; l < actual_neighbors; l++) printf("%d ", intensities[l]);
					printf("\n");
			}

			//Count occurences of Ik in the set of intensities
			int occurences[depth+1];
			int Imax = 0;
			for (int l = 0; l <= floor(depth/Fl)+1; ++l) {
				occurences[l] = 0;
				for (int m = 0; m < actual_neighbors; ++m) {
					if (intensities[m] == l) {
						occurences[l]++;
						if (occurences[l] > Imax) {
							Imax = occurences[l];
						}
					}
				}
			}

			//Compute the color intensities
			int Irgb[3][depth+1];
			int I_max_rgb[3]={0,0,0};
			for (int m = 0; m <= depth; ++m) {//For each intensity level (256 values)
				Irgb[0][m] = 0;
				Irgb[1][m] = 0;
				Irgb[2][m] = 0;
				for (int l = 0; l < actual_neighbors; ++l) {
					if (intensities[l] == m){
						Irgb[0][m] += temp_Pic[l][0];// Red
						Irgb[1][m] += temp_Pic[l][1];// Green
						Irgb[2][m] += temp_Pic[l][2];// Blue
						if (Irgb[0][m] > I_max_rgb[0])I_max_rgb[0]=Irgb[0][m];//Red
						if (Irgb[1][m] > I_max_rgb[1])I_max_rgb[1]=Irgb[1][m];//Green
						if (Irgb[2][m] > I_max_rgb[2])I_max_rgb[2]=Irgb[2][m];//Blue
						if (DEBUG)printf("Intensity: %d, Sum RED:%d\n", intensities[l], Irgb[0][m]);
					}
				}
			}

			//Find max(occurences)
			if (DEBUG)printf("Max intensity: %d\n", Imax);
			//Find max(colors intensity)s
			if(DEBUG)printf("Rm:%d Gm:%d Bm:%d\n", I_max_rgb[0], I_max_rgb[1], I_max_rgb[2]);
			//Assign new values
			newPic[3*(i*width + j) + 0] = I_max_rgb[0] / Imax;//Red
			newPic[3*(i*width + j) + 1] = I_max_rgb[1] / Imax;//Green
			newPic[3*(i*width + j) + 2] = I_max_rgb[2] / Imax;//Blue
			if (DEBUG){
				printf("New pixel values: %d %d %d\n", newPic[3*(i*width + j) + 0], newPic[3*(i*width + j) + 1], newPic[3*(i*width + j) + 2]);
				printf("================================\n");
			}
		}
	}
	clock_t end = clock();
	double time_spent = (double)(end - begin)/ CLOCKS_PER_SEC;
	printf("\nJob done in %2.4lf s !\n", time_spent);
	free(pic);

	//======================= POST-PROCESSING ===========================//
	//Takes the filtred image and saves it as a .ppm
	
	char oily_filename[20] = "oily_";
	strcat(oily_filename, filename);

	printf("Offset: %d\n", offset);

	FILE *pNewFile = fopen(oily_filename, "wb");
	//Writing the header
	char header[offset], temp[6];
	for (int i =0; i<offset ; ++i)header[i]='\0';
	sprintf(temp, "P6\n");
	strcat(header, temp);
	sprintf(temp, "%d", width);
	strcat(header, temp);
	strcat(header, " ");
	sprintf(temp, "%d", height);
	strcat(header, temp);
	strcat(header, "\n");
	sprintf(temp, "%d", depth);
	strcat(header, temp);
	strcat(header, "\n");
	strcat(header, "\0");
	fwrite(header, 1, offset, pNewFile);

	//Writing the data contained in pic
	unsigned char newBuffer[3*width]; //Creates another buffer
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			newBuffer[3*j]   =   newPic[3*(i*width + j)];//Red
			newBuffer[3*j + 1] = newPic[3*(i*width + j)+1];//Green
			newBuffer[3*j + 2] = newPic[3*(i*width + j)+2];// Blue
		}
		fwrite(newBuffer, 1, sizeof(newBuffer), pNewFile);
	}
	fclose(pNewFile);
	free(newPic);
	//======================= END OF PROGRAM ===========================//

	return(0);
}