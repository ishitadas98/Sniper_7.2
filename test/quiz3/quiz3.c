#include "stdio.h"
#include "sim_api.h"

#define ARRAY_SIZE 512
void main()
{
    double A[ARRAY_SIZE], B[ARRAY_SIZE], C[ARRAY_SIZE];
    int i = 0;

    printf("Number of bytes occupied by INT:%lu\n", sizeof(double));

    SimRoiStart();

    for (i = 0; i < 256; i++)
    {
         A[i] = B[i] + C[i];
    }

#if 0
    SimRoiEnd();
#endif
    printf("Hello Quiz3\n");
}
