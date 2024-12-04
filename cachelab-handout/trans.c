/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    for (int row = 0; row < N; row += 8)
    {
        for (int col = 0; col < M; col += 8)
        {
            int i, j, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;
            if (M == 64 && N == 64)
            { // handle 64*64 matrix separately because of cache collision every 4 rows
                for (i = 0; i < 8; i++)
                {
                    tmp1 = A[row + i][col];
                    tmp2 = A[row + i][col + 1];
                    tmp3 = A[row + i][col + 2];
                    tmp4 = A[row + i][col + 3];
                    tmp5 = A[row + i][col + 4];
                    tmp6 = A[row + i][col + 5];
                    tmp7 = A[row + i][col + 6];
                    tmp8 = A[row + i][col + 7];
                    B[col][row + i] = tmp1;
                    B[col + 1][row + i] = tmp2;
                    B[col + 2][row + i] = tmp3;
                    B[col + 3][row + i] = tmp4;
                    if (col - row != 8)
                    {
                        B[64][row + i + 8] = tmp5;
                        B[65][row + i + 8] = tmp6;
                        B[66][row + i + 8] = tmp7;
                        B[67][row + i + 8] = tmp8;
                    }
                    else
                    {
                        B[64][row + i + 16] = tmp5;
                        B[65][row + i + 16] = tmp6;
                        B[66][row + i + 16] = tmp7;
                        B[67][row + i + 16] = tmp8;
                    }
                }
                for (i = 0; i < 8; i++)
                {
                    if (col - row != 8)
                    {
                        B[col + 4][row + i] = B[64][row + i + 8];
                        B[col + 5][row + i] = B[65][row + i + 8];
                        B[col + 6][row + i] = B[66][row + i + 8];
                        B[col + 7][row + i] = B[67][row + i + 8];
                    }
                    else
                    {
                        B[col + 4][row + i] = B[64][row + i + 16];
                        B[col + 5][row + i] = B[65][row + i + 16];
                        B[col + 6][row + i] = B[66][row + i + 16];
                        B[col + 7][row + i] = B[67][row + i + 16];
                    }
                }
            }
            else if (row + 8 > N || col + 8 > M)
            { // handler residual blocks
                if (col + 8 <= M)
                {
                    for (i = 0; i < N - row; i++)
                    {
                        tmp1 = A[row + i][col];
                        tmp2 = A[row + i][col + 1];
                        tmp3 = A[row + i][col + 2];
                        tmp4 = A[row + i][col + 3];
                        tmp5 = A[row + i][col + 4];
                        tmp6 = A[row + i][col + 5];
                        tmp7 = A[row + i][col + 6];
                        tmp8 = A[row + i][col + 7];
                        B[col][row + i] = tmp1;
                        B[col + 1][row + i] = tmp2;
                        B[col + 2][row + i] = tmp3;
                        B[col + 3][row + i] = tmp4;
                        B[col + 4][row + i] = tmp5;
                        B[col + 5][row + i] = tmp6;
                        B[col + 6][row + i] = tmp7;
                        B[col + 7][row + i] = tmp8;
                    }
                }
                else if (row + 8 <= N)
                {
                    for (j = 0; j < M - col; j++)
                    {
                        tmp1 = A[row][col + j];
                        tmp2 = A[row + 1][col + j];
                        tmp3 = A[row + 2][col + j];
                        tmp4 = A[row + 3][col + j];
                        tmp5 = A[row + 4][col + j];
                        tmp6 = A[row + 5][col + j];
                        tmp7 = A[row + 6][col + j];
                        tmp8 = A[row + 7][col + j];
                        B[col + j][row] = tmp1;
                        B[col + j][row + 1] = tmp2;
                        B[col + j][row + 2] = tmp3;
                        B[col + j][row + 3] = tmp4;
                        B[col + j][row + 4] = tmp5;
                        B[col + j][row + 5] = tmp6;
                        B[col + j][row + 6] = tmp7;
                        B[col + j][row + 7] = tmp8;
                    }
                }
                else
                {
                    for (i = 0; i < N - row; i++)
                    {
                        for (j = 0; j < M - col; j++)
                        {
                            B[col + j][row + i] = A[row + i][col + j];
                        }
                    }
                }
            }
            else
            {
                for (i = 0; i < 8; i++)
                { // handle normal 8*8 blocks
                    tmp1 = A[row + i][col];
                    tmp2 = A[row + i][col + 1];
                    tmp3 = A[row + i][col + 2];
                    tmp4 = A[row + i][col + 3];
                    tmp5 = A[row + i][col + 4];
                    tmp6 = A[row + i][col + 5];
                    tmp7 = A[row + i][col + 6];
                    tmp8 = A[row + i][col + 7];
                    B[col][row + i] = tmp1;
                    B[col + 1][row + i] = tmp2;
                    B[col + 2][row + i] = tmp3;
                    B[col + 3][row + i] = tmp4;
                    B[col + 4][row + i] = tmp5;
                    B[col + 5][row + i] = tmp6;
                    B[col + 6][row + i] = tmp7;
                    B[col + 7][row + i] = tmp8;
                }
            }
        }
    }
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; j++)
        {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    // registerTransFunction(trans, trans_desc);
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; ++j)
        {
            if (A[i][j] != B[j][i])
            {
                return 0;
            }
        }
    }
    return 1;
}
