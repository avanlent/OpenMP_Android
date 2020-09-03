#include "Laplacian.h"
#include <omp.h>

void ComputeLaplacian(const float (&u)[XDIM][YDIM], float (&Lu)[XDIM][YDIM], int thread_count)
{

    omp_set_num_threads(thread_count);
#pragma omp parallel for
    for (int i = 1; i < XDIM-1; i++)
        for (int j = 1; j < YDIM-1; j++)
            Lu[i][j] =
                    -4 * u[i][j]
                    + u[i+1][j]
                    + u[i-1][j]
                    + u[i][j+1]
                    + u[i][j-1];

}

void ComputeLaplacianFlip(const float (&u)[XDIM][YDIM], float (&Lu)[XDIM][YDIM], int thread_count)
{

    omp_set_num_threads(thread_count);
#pragma omp parallel for
    for (int i = 1; i < YDIM-1; i++)
        for (int j = 1; j < XDIM-1; j++)
            Lu[i][j] =
                    -4 * u[i][j]
                    + u[i+1][j]
                    + u[i-1][j]
                    + u[i][j+1]
                    + u[i][j-1];

}

void ComputeLaplacianPtrArr(float **u, float **Lu, int thread_count)
{
    omp_set_num_threads(thread_count);
#pragma omp parallel for
    for (int i = 1; i < XDIM-1; i++)
        for (int j = 1; j < YDIM-1; j++)
            Lu[i][j] =
                    -4 * u[i][j]
                    + u[i+1][j]
                    + u[i-1][j]
                    + u[i][j+1]
                    + u[i][j-1];

}
