#include <stdbool.h>

#include "vendor/unity.h"
#include "vendor/unity_internals.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test(void)
{
	TEST_ASSERT_TRUE(true);
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test);

	return UNITY_END();
}
