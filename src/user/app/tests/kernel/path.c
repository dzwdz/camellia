#include "../tests.h"
#include <camellia/path.h>
#include <string.h>

static void test_path_simplify(void) {
	const char *testcases[][2] = {
		{"/",         "/"},
		{"/.",        "/"},
		{"//",        "/"},
		{"/asdf",     "/asdf"},
		{"/asdf/",    "/asdf/"},
		{"/asdf//",   "/asdf/"},
		{"/asdf/./",  "/asdf/"},
		{"/a/./b",    "/a/b"},
		{"/a/./b/",   "/a/b/"},

		// some slightly less easy cases
		{"/asdf/..",  "/"},
		{"/asdf/../", "/"},
		{"/asdf/.",   "/asdf/"},
		{"/asdf//.",  "/asdf/"},

		{"/foo/bar/..", "/foo/"},
		{"/foo/bar/../baz",  "/foo/baz"},
		{"/foo/bar/../baz/", "/foo/baz/"},
		{"/foo/bar/xyz/..",  "/foo/bar/"},
		{"/foo/bar/xyz/../", "/foo/bar/"},

		// going under the root or close to it
		{"/..",        NULL},
		{"/../asdf",   NULL},
		{"/../asdf/",  NULL},
		{"/./a/../..", NULL},
		{"/a/a/../..", "/"},
		{"/a/../a/..", "/"},
		{"/a/../../a", NULL},
		{"/../a/../a", NULL},
		{"/../../a/a", NULL},
		{"/////../..", NULL},
		{"//a//../..", NULL},

		// relative paths aren't allowed
		{"relative",   NULL},
		{"some/stuff", NULL},
		{"./stuff",    NULL},
		{"../stuff",   NULL},
		{"",           NULL},
	};

	char buf[256];
	for (size_t i = 0; i < sizeof(testcases) / sizeof(testcases[0]); i++) {
		const char *input    = testcases[i][0];
		const char *expected = testcases[i][1];
		int len = path_simplify(input, buf, strlen(input));
		if (expected == NULL) {
			test(len < 0);
		} else {
			// TODO an argument for printing info on test failure
			test(len == (int)strlen(expected) && !memcmp(expected, buf, len));
		}
	}
}

void r_k_path(void) {
	run_test(test_path_simplify);
}
