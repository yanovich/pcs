#include <stdio.h>
int main()
{
	printf("Hello world!\n");
#ifdef BFD64
	printf("Hello files!\n");
#endif
	return 0;
}
