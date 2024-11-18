//-----start of a.h--------
extern int a;
static int sa=5;
//-----end of a.h----------

//-----start of b.h--------
int b(int p);
static int sb=10;
//-----end of b.h----------

//-----start of c.h--------
extern int c;
static int sc=3;
//-----end of c.h----------

//-----start of one.c------
#include "a.h"
#include "b.h"
int c = 2;            
int main(void)       
{ 	int temp = 10;  
	sb = sb + temp;
	printf("Invoke from %d: %d\n", a, sb); 
	b(temp+sb);  
	sb = sb + temp;
	printf("Invoke from %d: %d\n", a, sb); 
	b(temp+sb);  
	return 0;
} 
//-----end of one.c----------

//-----start of two.c--------
#include "c.h"
#include "b.h"
int a = 1;    
int b(int p2)  
{ 	static int temp = 2;  
	sc++;
	sb = sb + 2;
	temp = p2+temp+sc+sb;
	if (temp < 50) goto done;
	temp = 100;
done:
	printf("Output from %d: %d\n", c, temp); 
	return temp;
} 
//-----end of two.c----------