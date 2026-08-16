// Desktop self-test for core/str.h. Pure, no dependencies — compile and run:
//   g++ -std=c++17 -I../../ str_test.cpp -o str_test && ./str_test
// Exits non-zero on the first failed check.

#include "core/str.h"
#include <cstring>
#include <cstdio>

static int failures = 0;
#define CHECK(cond) do { \
	if (!(cond)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); failures++; } \
} while (0)

int main() {
	// copy: normal
	{
		char b[16];
		CHECK(core::copy(b, "hello"));
		CHECK(strcmp(b, "hello") == 0);
	}
	// copy: exact fit (15 chars + NUL in 16)
	{
		char b[16];
		CHECK(core::copy(b, "123456789012345"));
		CHECK(strlen(b) == 15);
	}
	// copy: truncation returns false, stays NUL-terminated, no overflow
	{
		char b[8] = {};
		CHECK(!core::copy(b, "this is far too long"));
		CHECK(strcmp(b, "this is") == 0);
		CHECK(b[7] == '\0');
	}
	// copy: null source -> empty
	{
		char b[8]; b[0] = 'x';
		CHECK(core::copy(b, nullptr));
		CHECK(b[0] == '\0');
	}
	// format: normal + truncation
	{
		char b[16];
		CHECK(core::format(b, "%s-%d", "id", 42));
		CHECK(strcmp(b, "id-42") == 0);
		char s[6];
		CHECK(!core::format(s, "%s", "toolong"));
		CHECK(strcmp(s, "toolo") == 0);
	}
	// append: onto existing + truncation
	{
		char b[16];
		core::copy(b, "foo");
		CHECK(core::append(b, "bar"));
		CHECK(strcmp(b, "foobar") == 0);
		char s[8];
		core::copy(s, "abcd");
		CHECK(!core::append(s, "efghij"));
		CHECK(strcmp(s, "abcdefg") == 0);
		CHECK(s[7] == '\0');
	}

	if (failures == 0) printf("core/str.h: all checks passed\n");
	return failures == 0 ? 0 : 1;
}
