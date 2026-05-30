
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
	for (;;) {
		printf("Hello from Eris Linux container!\n");
		sleep(5);
	}
	return 0;
}

