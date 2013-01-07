#include <stdio.h>

typedef union
{
	int i;
	char c[4];
}u;

int main()
{
	u temp;
	temp.i = 0x12345678;
	printf("%x\n", temp.i);
	printf("%x %x %x %x\n", temp.c[0], temp.c[1], temp.c[2], temp.c[3]);
}
