#include <stdio.h>
#include "sys/types.h"
#include "fcntl.h"

int main(int argc, char** argv)
{
    int fd = open("dev/relay",O_WRONLY,0777);

    if (fd == -1)
    {
        printf("open  erorr\n");
    }
    
    int num = 0;

    while(1)
    {
        scanf("%d",&num);
        if (num == 9)
        {
            break;
        }
        ioctl(fd, num);
    }
    close(fd);

    printf("bye bye!!\n");
    
    return 0;
}
