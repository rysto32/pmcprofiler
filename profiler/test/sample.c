
#include <stdio.h>

static inline void rstone_test_1(void)
{
	printf("hello\n");
}

static inline void rstone_test_2(void)
{
	rstone_test_1();
}

int test(void)
{
	rstone_test_2();
	return (4);
}


