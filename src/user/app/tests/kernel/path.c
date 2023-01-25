#include "../tests.h"
#include <camellia/path.h>
#include <string.h>
#include <camellia/compat.h>
#include <camellia/fs/misc.h>

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
			test(len == 0);
		} else {
			// TODO an argument for printing info on test failure
			test(len == (int)strlen(expected) && !memcmp(expected, buf, len));
		}
	}
}

static void mount_resolve_drv(const char *path) {
	if (fork2_n_mount(path)) return;

	struct ufs_request res;
	while (!c0_fs_wait(NULL, 0, &res)) {
		// TODO does the first argument of c0_fs_respond need to be non-const?
		c0_fs_respond((void*)path, strlen(path), 0);
	}
	exit(1);
}

static void test_mount_resolve(void) {
	const char *testcases[][2] = {
		{"/",              "/"},
		{"/test",          "/"},
		{"/dir",           "/dir"},
		{"/dir/..",        "/"},
		{"/dir/../dir",    "/dir"},
		{"/dirry",         "/"},
		{"/dir/",          "/dir"},
		{"/dir/shadowed",  "/dir"},
		{"/dir/shadowed/", "/dir"},
	};
	mount_resolve_drv("/");
	mount_resolve_drv("/dir/shadowed");
	mount_resolve_drv("/dir");

	char buf[16];
	for (size_t i = 0; i < sizeof(testcases) / sizeof(testcases[0]); i++) {
		const char *input    = testcases[i][0];
		const char *expected = testcases[i][1];
		hid_t h = _sys_open(input, strlen(input), 0);
		test(h >= 0);
		int len = _sys_read(h, buf, sizeof buf, 0);
		test(len == (int)strlen(expected) && !memcmp(expected, buf, len));
		_sys_close(h);
	}
}

void r_k_path(void) {
	run_test(test_path_simplify);
	run_test(test_mount_resolve);
}
