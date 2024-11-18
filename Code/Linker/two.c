#include "c.h"
#include "b.h"
int a = 1;    
int b(int p2)  
{ 
	static int temp = 2;  
	sc++;
	sb = sb + 2;
	temp = p2+temp+sc+sb;

	if (temp < 50) goto done;
	temp = 100;
done:
	printf("Output from %d: %d\n", c, temp); 
	return temp;
} 
