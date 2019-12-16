/******************************************************************************

                            Online C Compiler.
                Code, Compile, Run and Debug C program online.
Write your code in this editor and press "Run" button to compile and execute it.

*******************************************************************************/

#include <stdio.h>
#include <string.h>

#define TCP_PACKET_COUNT    127
#define BUF_SIZE            1000


char g_cBsdSendBuf[BUF_SIZE];

int main()
{
    int             iCounter;
    size_t          lenfdp;
    
    for (iCounter=0 ; iCounter<TCP_PACKET_COUNT ; iCounter++)
    {
        g_cBsdSendBuf[iCounter] = (char)(iCounter);
        printf("char = %d\n", g_cBsdSendBuf[iCounter]);
    }

    lenfdp = strlen(g_cBsdSendBuf);
    printf("len start = %lu\n\r",lenfdp);
    printf("3rd char = %d\n", g_cBsdSendBuf[2]);
    
     char name[]="siva";
     printf("name = %p\n", name);
     printf("&name[0] = %p\n", &name[0]);
     printf("name printed as %%s is %s\n",name);
     printf("*name = %c\n",*name);
     printf("name[0] = %c\n", name[0]);

    return 0;
}
