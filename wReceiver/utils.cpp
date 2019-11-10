# include <iostream>
# include <cstdio>
# include <stdlib.h>

using namespace std;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}