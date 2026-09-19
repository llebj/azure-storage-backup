#include <stdbool.h>

#include "vendor/unity.h"
#include "vendor/unity_internals.h"

#include "../src/lib.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_it_correctly_hashes_a_basic_string(void)
{
	char *input = "hello";
	uint64_t output = poly_hash(input);

	TEST_ASSERT_EQUAL_UINT64(90986922, output);
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_it_correctly_hashes_a_basic_string);

	return UNITY_END();
}
