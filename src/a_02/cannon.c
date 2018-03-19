/******************************************************************************/
/*  MPI-template for the parallel calculation of C = C + A*B based on         */
/*  Cannon's algorithm                                                        */
/******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mpi.h"

#define N 16 /* matrix dimension (global) */

#define TAG_A 100
#define TAG_B 101
#define TAG_C 102


/******************************************************************************/
/*  Function MatrixMultiply()                                                 */
/*   - calculates C = C + A*B                                                 */
/******************************************************************************/
void MatrixMultiply(int n, double *A, double *B, double *C){

  int i, j, k;

  for (i=0; i<n; i++){
    for(j=0; j<n; j++)
      for(k=0; k<n; k++)
	C[i*n+j] += A[i*n+k] * B[k*n+j];
  }
}


/******************************************************************************/
/*  Function CannonMatrixMultiply                                             */
/*   - multiplies two quadratic matrices based on Cannon's algorithm          */
/******************************************************************************/
void CannonMatrixMultiply(int n, double *A, double *B, double *C, 
                          int num_blocks, int mycoords[2], MPI_Comm comm_2d){

  int i;
  int right, left, up, down;
  int shiftsource, shiftdest;
  MPI_Status status;

  printf("Hello, I am ( %d | %d ), and this are my blocks:\n", mycoords[0], mycoords[1]);
  printf("\nA:\n");
  
    


  /* TODO: compute ranks of all four neighbors */

  /* TODO: perform the initial matrix alignment for A and B */

  /* get into the main computation loop */
  for (i=0; i<num_blocks; ++i){

    /* TODO: compute C = C + A*B */

    /* TODO: shift matrix A left by one */

    /* TODO: shift matrix B up by one */
  }

  /* TODO: restore the original distribution of A and B */

}


/******************************************************************************/
/*                              M A I N                                       */
/******************************************************************************/

int main(int argc, char *argv[]) 
{

  int    i, j, k;
  double matA[N * N], matB[N * N], matC[N * N];  /* total matrices */
  double *locA, *locB, *locC;  /* local matrices */
  double errmax, vergl;

  int num_test_prints;
  int num_procs, my_rank;
  int blocksize, num_blocks, blockstart;
  int proc, count;
  int dims[2], periods[2];
  int my_rank_2d, mycoords[2];

  MPI_Status status;
  MPI_Datatype blockmat;
  MPI_Comm comm_2d;
  
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
  MPI_Comm_size(MPI_COMM_WORLD,&num_procs);

  num_blocks = sqrt(num_procs);

  /* N divisible by sqrt(num_procs) ? */ 
  if ( (fabs(sqrt(num_procs) - num_blocks) > 0.01) || ((N % num_blocks) != 0) ){
    printf("N = %d must be divisible by sqrt(num_procs)!\n",N);
    exit(1);
  }

  blocksize = N / num_blocks;

  /* ALL PROCESSES: allocate memory for local part of the matrices */
  locA = malloc(blocksize * sizeof(double));
  locB = malloc(blocksize * sizeof(double));
  locC = malloc(blocksize * sizeof(double));

  /* ALL PROCESSES: create the Cartesian topology, without rank reordering */
  dims[0] = num_blocks;
  dims[1] = num_blocks;
  periods[0] = 1;
  periods[1] = 1;
  MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 0, &comm_2d);

 
  /* get rank and coordinates with respect to the new communicator */
  MPI_Comm_rank(comm_2d, &my_rank_2d);
  MPI_Cart_coords(comm_2d, my_rank_2d, 2, mycoords);


  /* ALL PROCESSES: data type for the blockwise distribution of the matrices */
  MPI_Type_vector(blocksize, blocksize, N, MPI_DOUBLE, &blockmat);
 

  /* MASTER: initialize matrices */
  if (my_rank == 0){
    k=1;
    for (i=0; i<N; i++) {
      for (j=0;j<N; j++) { 
	matA[i*N+j]= k++; 
	matB[i*N+j]=(i<=j)?1.0:0.0;
	matC[i*N+j]=0;
      }
    }
    /* test prints */
    num_test_prints = N < 10 ? N : 10;
    for (j=0;j<num_test_prints; j++) { /* j := Zeile */
      printf("A(2,%d)= %14.8f\n",j,matA[2*N+j]);
    }
    for (j=0;j<num_test_prints ; j++) { /* j := Zeile */
      printf("B(2,%d)= %14.8f\n",j,matB[2*N+j]);
    }

    /* initialize local matrix blocks */
    count = 0;
    for(i=0; i<blocksize; ++i){
      for (j=0; j<blocksize; ++j){
	locA[count] = matA[i*N + j];
	locB[count] = matB[i*N + j];
	count++;
      }
    }

    /* distribute matrices blockwise among processes*/
    int curRow;
    int curCol;
    for (curRow = 0; curRow < num_blocks; curRow++) {
      for (curCol = 0; curCol < num_blocks; curCol++) {
	// don't send from master to master
	if (curRow != mycoords[0] || curCol != mycoords[1]) {
	  // get the rank of the destination process
	  int destRank;
	  int curCoords [] = {curRow, curCol};
	  MPI_Cart_rank(comm_2d, curCoords, &destRank);

	  // send the A and B blocks to the destination process
	  MPI_Send(&(matA[curRow * blocksize * N + curCol * blocksize]), 1, blockmat, destRank, TAG_A, comm_2d);
	  MPI_Send(&(matB[curRow * blocksize * N + curCol * blocksize]), 1, blockmat, destRank, TAG_B, comm_2d);
	}
      }
    }
  }

  /* WORKER: receive matrix blocks */
  else{
    MPI_Recv(locA, blocksize*blocksize, MPI_DOUBLE, 0, TAG_A, comm_2d, &status);
    MPI_Recv(locB, blocksize*blocksize, MPI_DOUBLE, 0, TAG_B, comm_2d, &status);
  }

  /* ALL PROCESSES: initialize matric C */
  for (i=0; i<blocksize*blocksize; ++i)
    locC[i] = 0;


  /*********************************************************/
  /* ALL PROCESSES:   call funktion CannonMatrixMultiply() */
  /*********************************************************/
  CannonMatrixMultiply(blocksize, locA, locB, locC, num_blocks, mycoords, comm_2d);


  /* TODO: WORKER: send result to master */
  if(my_rank != 0){

  }

  /* MASTER: collect results and check correctness */
  else{

    /* TODO: collect results */

    /* copy own results */
    count = 0;
    for(i=0; i<blocksize; ++i){
      for (j=0; j<blocksize; ++j){
	matC[i*N + j] = locC[count];
	count++;
      }
    }      
     
    /* check results */
    errmax=0.0;
    for (i=0; i<N; i++) {
      for (j=0;j<N; j++) { 
	vergl=(double)(i*(j+1.0)*N+(j+2.0)*(j+1.0)/2.0);
	if( fabs(matC[i*N+j]-vergl) > errmax)
	  {
	    errmax=fabs(matC[i*N+j]-vergl);
	    if(errmax>1.0)
	      {
		printf("C(%d,%d)=%14.8f, vergl=%14.8f \n",i,j,matC[i*N+j],vergl);
	      }
	  }
      }
    }
    for (j=0;j<num_test_prints; j++) { 
      printf("C(2,%d)= %14.8f, Vergleichswert=%d \n",j,matC[2*N+j],2*(j+1)*N+(j+2)*(j+1)/2);
    }
    printf("maximum error: %14.8f\n",errmax);
  }

  /* TODO: free communicator */

  MPI_Finalize();

  return 0;
}