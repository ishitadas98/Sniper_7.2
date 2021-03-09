#include "stdio.h"
#include "/home/ishita/Desktop/Sniper/sniper-7.2/include/sim_api.h"
#define ARRAY_SIZE 1000000

int a[ARRAY_SIZE];
int b[ARRAY_SIZE];
int c[ARRAY_SIZE];

int main()
{
    SimRoiStart();
    SimNamedMarker(4, "begin");
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        a[i] = 50*i;
        b[i] = 50*i;
        c[i] = 0;
    }
    
    int *p = (int *) 0x614a6c;
    printf("%x\n", p);
    printf("address of a[%d] is:0x%x\n", 99, &a[99]);
    
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        c[i] = a[i] + b[i];
    }
    SimNamedMarker(5, "end");
    SimRoiEnd();

    printf("done!\n");
}