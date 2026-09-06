#include <stddef.h>

#include "vendor/unity.h"
#include "vendor/unity_internals.h"

#include "../profile-installer/parser.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_test()
{
	size_t count = 0;
	bool result = count_profiles(&count, NULL, 0);
	TEST_ASSERT_TRUE(result);
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_test);

	return UNITY_END();
}
