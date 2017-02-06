#include <stdio.h>
#include "sys/types.h"
#include "fcntl.h"

int main(int argc, char** argv)
{
    int fp;
    char val[6];
    int i = 0;

    fp = open("dev/tmp_out",O_RDONLY,0777);
    if(fp < 0)
    {
        printf("Open error!\n");
        return -1;
    }
    else
    {
        printf("Open susses!\n");
    }
    while(1)
    {
	    i = read(fp, &val, sizeof(val));
	        if (i<0)
	        {
	            printf(">>>i = %d\n", &i);
	            return -2;
	        }
	   
	    printf(">>>val = %d.%d  %d  %d  %d\n", val[0],val[1],val[2],val[3],val[4]);
    }
    close(fp);

    return 0;
}
