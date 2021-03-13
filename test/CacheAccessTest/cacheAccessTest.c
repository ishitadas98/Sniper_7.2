#include "stdio.h"
#include "stdint.h"
#include "/home/ishita/Desktop/Sniper/sniper-7.2/include/sim_api.h"
#define ARRAY_SIZE 8
uint64_t a[ARRAY_SIZE];
uint64_t b[ARRAY_SIZE];
uint64_t c[ARRAY_SIZE];

int main()
{
    SimRoiStart();
    SimNamedMarker(4, "begin");
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        a[i] = 50;
        b[i] = 50;
        c[i] = 0;
    }
    
    // int *p = (int *) 0x614a6c;
    // printf("%x\n", p);
    // printf("address of a[%d] is:0x%x\n", 99, &a[99]);
    
    for (int i = 0; i < ARRAY_SIZE; i+=2)
    {
        c[i] = a[i] + b[i];
    }




    SimNamedMarker(5, "end");
    SimRoiEnd();

    printf("done!\n");
}