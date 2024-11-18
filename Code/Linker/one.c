#include "a.h"
#include "b.h"

int c = 2;            
int main(void)       
{ 
	int temp = 10;  
	sb = sb + temp;
	printf("Invoke from %d: %d\n", a, sb); 
	b(temp+sb);  
	sb = sb + temp;
	printf("Invoke from %d: %d\n", a, sb); 
	b(temp+sb);  
	return 0;
} 
