#include <kernel/tests/base.h>
#include <kernel/util.h>
#include <kernel/vfs/path.h>

TEST(path_simplify) {
	char buf[256];

	// some easy valid cases
	TEST_COND( path_simplify("/asdf", buf, 5));
	TEST_COND( path_simplify("/asd/", buf, 5));
	TEST_COND( path_simplify("/a/./", buf, 5));
	TEST_COND( path_simplify("/a/..", buf, 5));
	TEST_COND( path_simplify("/a//.", buf, 5));

	// .. going under the root or close to it
	TEST_COND(!path_simplify("/../123456", buf, 10));
	TEST_COND(!path_simplify("/./a/../..", buf, 10));
	TEST_COND( path_simplify("/a/a/../..", buf, 10));
	TEST_COND(!path_simplify("/////../..", buf, 10));
	TEST_COND(!path_simplify("//a//../..", buf, 10));

	// relative paths aren't allowed
	TEST_COND(!path_simplify("apath", buf, 5));
	TEST_COND(!path_simplify("a/pth", buf, 5));
	TEST_COND(!path_simplify("../th", buf, 5));

	// this includes empty paths
	TEST_COND(!path_simplify("", buf, 1));

	// TODO test if the paths are simplified correctly
}

void tests_vfs() {
	TEST_RUN(path_simplify);
}
