#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define ALPHA	256
#define PRIME	1610612741

uint64_t poly_hash(char *string)
{
	// We can use Horner's method to efficiently calculate the polynomail
	// based hash using O(n) multiplications, as opposed to O(n^2) when using
	// `pos()` (n^y = n*n*n*...*n).
	//
	// Horner's rule takes advantage of the nested format of a polynomial:
	// a_0(*x^0) + a_1*x^1 + a_2*x^2 + ... + a_n*x^n
	//	= a_0 + x(a_1 + x(a_2 + ... + x(a_n-1 + x*a_n) ... ))
	//
	// This can be written as a loop that uses pre-computation to build
	// up a polynomial of base 'x' (ALPHA), using the character code as a
	// coefficient. The loop expands the nested polynomial from the inner most
	// terms to the outer-most. We start with the highest order coefficient equal
	// to 1 (a_n), and then use the character codes of the string as we work our way
	// outward: 'a_n-1' = string[0], 'a_n-2 = string[1], etc...;
	// We then multiple by 'x' (= ALPHA) as we expand out the terms of the nested
	// ploynomial.
	uint64_t hash = 1;
	size_t len = strlen(string);
	for (size_t i = 0; i < len; ++i) {
		hash = (hash * ALPHA + (unsigned char)string[i]) % PRIME;
	}
	return hash;
}

