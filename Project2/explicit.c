/*=======================================================================================
*	This code was written by:                                                          *
*								Antonin Aumètre - antonin.aumetre@gmail.com            *
*								Céline Moureau -  cemoureau@gmail.com          		   *
*	For: High Performance Scientific course at ULiège, 2018-19                         *
*	Project 2                                                                          *
*                             														   *
*	Originally uploaded to: https://github.com/Cobalt1911                              *
*	Under GNU General Public License 11/2018                                           *
=======================================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

#include "COO_CSR_BSR.h"
#include "algorithms.h"
#include "fileIO.h"


#define initConcentration 1 //[g/m3]

int explicit_solver(int argc, char *argv[])
{
	// Initalizes MPI
	MPI_Init(NULL, NULL);
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	// Declare variables for the main loop
	size_t iteration = 0;
	bool onBoundary = false;
	bool onZBoundary = false;
	bool valueOnBoundary = false;
	int *stopFlags = calloc(world_size, sizeof(int));
	int *stopFlagsFromOthers = calloc(world_size, sizeof(int));
	bool stopFlag = false;
	bool isIdle = false;

	// Retrieving data from the .dat file
	Param parameters = readDat(argv[1]);
	size_t nodeX = (int)(parameters.L/parameters.h) + 1;
	size_t nodeY = nodeX, nodeZ =nodeX;
	size_t stopTime = parameters.Tmax/parameters.m;

	if (rank == 0) {
		printf("We have %d nodes\n", world_size);
		printf("Number of slices: %zu\n", nodeZ);
		printf("=====================================\n");
	}
	MPI_Barrier(MPI_COMM_WORLD);

	// If there are more nodes than possible slices, the surplus nodes are set to idle
	int *ranks = calloc(world_size, sizeof(int));
	if (world_size > nodeZ){ // Change the value of world_size to that further calculations are still valid
		world_size = nodeZ;
		for (int i = 0; i < nodeZ; ++i){
			ranks[i] = i;
		}
		if (rank >= world_size){ // Idle this node
			printf("Node %d was set to idle.\n", rank);
			isIdle = true;
			stopFlag = true;
		}
	}
	else{
		for (int i = 0; i < world_size; ++i){
			ranks[i] = i;
		}
	}

	// If there are some idling nodes, exclude them from the new communicator
	MPI_Group world_group;
	MPI_Comm_group(MPI_COMM_WORLD, &world_group);
	MPI_Group sub_world_group;
	MPI_Group_incl(world_group, world_size, ranks, &sub_world_group);
	// Define the new communiator associated with the sub-group
	MPI_Comm SUB_COMM;
	MPI_Comm_create_group(MPI_COMM_WORLD, sub_world_group, 0, &SUB_COMM);

	// Assign each node its nomber of slices (thicknessMPI)
	int nbAdditionalSlices = 0;
	size_t thicknessMPI = 0;
	int *share = shareWorkload(nodeZ, world_size);
	if (rank == world_size-1 && world_size != 1){
		thicknessMPI = share[0];
		nbAdditionalSlices = share[0]-share[1];
	}
	else thicknessMPI = share[1];

	// Some prints
	if (rank == 0) printf("Vx, Vy, Vz: %.2f %.2f %.2f\n", parameters.vx, parameters.vy, parameters.vz);
	MPI_Barrier(MPI_COMM_WORLD);
	if (!isIdle) printf("Thickness = %zu, for rank %d\n", thicknessMPI, rank);
	MPI_Barrier(MPI_COMM_WORLD);
	if (rank == 0) printf("=====================================\n");

	// Get memory for the concentration values
	double *concentration = calloc(nodeX*nodeY*thicknessMPI, sizeof(double));
	double *c_ = calloc(nodeX*nodeY*(thicknessMPI+2), sizeof(double)); // +2 to store the values coming from the previous and next slices
	if (concentration == NULL || c_ == NULL) {
		puts("Mem ERR0R !");
		exit(1);
	}

	// Give initial concentration value
	int middleSliceIndex = floor(nodeZ/2);
	int rankMiddle = floor(middleSliceIndex/(thicknessMPI-nbAdditionalSlices));
	if (rank == rankMiddle){
		int initValueIndex = nodeX*floor(nodeY/2) + floor(nodeX/2) + nodeX*nodeY*(middleSliceIndex-rank*(thicknessMPI-nbAdditionalSlices));
		c_[initValueIndex+nodeX*nodeY] = initConcentration;
		printf("Initial value set on node #%d, at index %d\n", rank, initValueIndex);
	}
	MPI_Barrier(MPI_COMM_WORLD);

	/*================================================================================================
	#	Main loop
	================================================================================================*/
	int numthreads = 1;
	#pragma omp parallel //Parallel region of the code
	numthreads = __builtin_omp_get_num_threads();
	if (rank == 0) printf("Iteration: ");
	while (iteration <= stopTime && !stopFlag)
	{
		if (rank == 0) printf("%ld ", iteration);
		// Search for boundaries
		//size_t isXbound = 0;
		//size_t index = 0;
		//if (rank == 0) index = nodeX*nodeY;
		//size_t stopIndex = nodeX*nodeY*thicknessMPI;
		//if (rank == world_size-1) stopIndex -= nodeY*nodeX;

		// ============================== Compute internal values
		#pragma omp for
		for(size_t index=0; index<nodeX*nodeY*thicknessMPI; index++)
		{
			bool isOnZBoundary = false;
			int k = floor(index/(nodeX*nodeY));
			int j = floor((index-k*nodeX*nodeY)/nodeX);
			int i = index - k * nodeX * nodeY - j * nodeX;

			if (rank == 0) isOnZBoundary = (index<nodeX*nodeY);
	    if (rank == world_size-1) isOnZBoundary = (index>=(thicknessMPI-1)*nodeX*nodeX);

			if (!(i==0 || i == nodeX-1 || j==0 || j == nodeY-1|| isOnZBoundary))
			{	//I am in the domain
				int kbis = k+1;

				concentration[index] =
					c_[i+j*nodeX+kbis*nodeX*nodeY] +
					parameters.m * parameters.D * (c_[i+1+j*nodeX+kbis*nodeX*nodeY]+c_[i+(j+1)*nodeX+kbis*nodeX*nodeY]+
					c_[i+j*nodeX+(kbis+1)*nodeX*nodeY]-6*c_[i+j*nodeX+kbis*nodeX*nodeY]+
					c_[i-1+j*nodeX+kbis*nodeX*nodeY]+c_[i+(j-1)*nodeX+kbis*nodeX*nodeY]+
					c_[i+j*nodeX+(kbis-1)*nodeX*nodeY])/pow(parameters.h,2) -
					parameters.m * parameters.vx * (c_[i+1+j*nodeX+kbis*nodeX*nodeY]-c_[i-1+j*nodeX+kbis*nodeX*nodeY])/(2*parameters.h) -
					parameters.m * parameters.vy * (c_[i+(j+1)*nodeX+kbis*nodeX*nodeY]-c_[i+(j-1)*nodeX+kbis*nodeX*nodeY])/(2*parameters.h) -
					parameters.m * parameters.vz * (c_[i+j*nodeX+(kbis+1)*nodeX*nodeY]-c_[i+j*nodeX+(kbis-1)*nodeX*nodeY])/(2*parameters.h);

				// Check the stopping conditions on the boundaries of the domain
				if (rank == 0) onZBoundary = (index<2*nodeX*nodeY);
				if (rank == world_size-1) onZBoundary = (index>=(thicknessMPI-2)*nodeX*nodeX);
				onBoundary = (i<=1 || i >= nodeX-2 || j<=1 || j >= nodeY-2);
				if ((onBoundary || onZBoundary)  && (concentration[index] > 5e-8)){
					valueOnBoundary=true;
					printf("concentration on boundary = %e at index %zu at iteration %zu\n", concentration[index], index, iteration);
         			printf("STOP\n");
				}
			}
		}

		// Send your status to all the other nodes
		for (int i = 0; i < world_size; ++i)
		{
			if(valueOnBoundary)stopFlags[i] = 1;
		}
		MPI_Allgather(stopFlags, 1, MPI_INT, stopFlagsFromOthers, 1, MPI_INT, SUB_COMM);
		for (int i = 0; i < world_size; ++i)
		{
			if(stopFlagsFromOthers[i] == 1)
			{
			stopFlag = true;
			break;
			}
		}

		for (size_t copyIndex = 0; copyIndex< nodeX*nodeY*thicknessMPI; copyIndex++)
		{
			c_[copyIndex+nodeX*nodeY] = concentration[copyIndex];
		}

		// ============================== Send and receive neighboring values
		int *commList = getCommListSlices(world_size);
		for (int commIndex=0 ; commIndex<4*(world_size-1) ; commIndex += 2)
		{
			// Get sender (commList[commIndex]) & receiver (commList[commIndex+1])
			//printf("S#%d R%d\n", commList[commIndex], commList[commIndex+1]);
			bool isSender = false;
			bool isReceiver = false;
			if (rank == commList[commIndex]) isSender = true;
			if (rank == commList[commIndex+1]) isReceiver = true;
			int klocal = 0;
			int i = 0;
			int j = 0;
			if (isSender)
			{
				if (commList[commIndex] > commList[commIndex+1])
					{ // If sender ID is greater than receiver ID
						// Send the upper boundary values
						klocal = 0;
						MPI_Send(&concentration[0], nodeX*nodeY, MPI_DOUBLE, commList[commIndex+1], 0, SUB_COMM);
					}
				else
				{
					// Send the lower boundary values
					klocal = thicknessMPI-1;
					MPI_Send(&concentration[klocal*nodeX*nodeY], nodeX*nodeY, MPI_DOUBLE, commList[commIndex+1], 0, SUB_COMM);
				}
			}
			else if (isReceiver){
				if (commList[commIndex] > commList[commIndex+1]){ // If sender ID is greater than receiver ID
					// Get the upper boundary values
					klocal = thicknessMPI+1;
					MPI_Recv(&c_[klocal*nodeX*nodeY], nodeX*nodeY, MPI_DOUBLE, commList[commIndex], 0, SUB_COMM, MPI_STATUS_IGNORE);
				}
				else
				{
					// Get the lower boundary values
					klocal = 0;
					MPI_Recv(&c_[klocal*nodeX*nodeY],  nodeX*nodeY, MPI_DOUBLE, commList[commIndex], 0, SUB_COMM, MPI_STATUS_IGNORE);
				}
			}
		}

		// Check if files should be saved
		if (iteration%parameters.S == 0){
			// ============================== Writing the output file
			// Use of the MPI file IO functions
			int data_size = thicknessMPI*nodeX*nodeY; // doubles, data every node has (this is a number, not bytes!)
			MPI_File output_file;
			char file_name[30];
			sprintf(file_name, "resultsExplicit/c_%ld.dat",iteration);
			unsigned int N[] = {nodeX};

			MPI_File_open(SUB_COMM, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &output_file);
			if (rank == 0) MPI_File_write(output_file, N, 1, MPI_UNSIGNED, MPI_STATUS_IGNORE);
			MPI_Offset displacement;
			displacement = rank*(thicknessMPI-nbAdditionalSlices)*nodeX*nodeY*sizeof(double) + sizeof(unsigned int); // Displacement in bytes
			// Set the view the current node has
			MPI_File_seek(output_file, displacement, MPI_SEEK_SET);
			MPI_File_write(output_file, concentration, nodeX*nodeY*thicknessMPI, MPI_DOUBLE, MPI_STATUS_IGNORE);
			MPI_File_close(&output_file);
		}
		++iteration;
	}

	// Cleaning
	free(stopFlagsFromOthers);
	free(stopFlags);
	free(share);
	free(ranks);
	free(concentration);
	free(c_);
	MPI_Group_free(&world_group);
	MPI_Group_free(&sub_world_group);
	if (!isIdle) MPI_Comm_free(&SUB_COMM);
	MPI_Finalize();

	printf("\nJob done using %d nodes and %d threads.\n", world_size, numthreads);
	return 0;
}
