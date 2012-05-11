#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "board.hpp"
#include "gtp.h"


int main(int argc, char *argv[])
{
    initialize();
	int flag = 0;

	int i;
	for(i=1; i<argc; i++)
	{
		if(strcmp(argv[i], "--mode") == 0)
		{
			if(i+1<argc)
			{
				i ++;
				if(strcmp(argv[i], "gtp") == 0) flag = 1;
			}
		}
		else if(strcmp(argv[i], "--level") == 0)
		{
			if(i+1<argc)
			{
				int c = 0;
				i ++;
				sscanf(argv[i], "%d", &c);
				int TimeLimit = 0;
				switch(c)
				{
					case 1: TimeLimit = 1000; break;
					case 2: TimeLimit = 2500; break;
					case 3: TimeLimit = 5000; break;
					case 4: TimeLimit = 10000; break;
					case 5: TimeLimit = 15000; break;
					case 6: TimeLimit = 22500; break;
					case 7: TimeLimit = 30000; break;
					case 8: TimeLimit = 40000; break;
					case 9: TimeLimit = 60000; break;
					case 10: TimeLimit = 90000; break;
					default: TimeLimit = 120000; break;
				}
			}
		}
	}
	if(flag == 0)
	{
      //a text ui for debugging?
	}
	else
	{
		play_gtp();
	}
	return 0;
}
