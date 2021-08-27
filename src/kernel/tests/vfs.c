#include <kernel/mem.h>
#include <kernel/tests/base.h>
#include <kernel/util.h>
#include <kernel/vfs/mount.h>
#include <kernel/vfs/path.h>

TEST(path_simplify) {
#define TEST_WRAPPER(argument, result) do { \
		int len = path_simplify(argument, buf, sizeof(argument) - 1); \
		if (result == 0) { \
			TEST_COND(len < 0); \
		} else { \
			if (len == sizeof(result) - 1) { \
				TEST_COND(0 == memcmp(result, buf, len)); \
			} else { \
				TEST_COND(false); \
			} \
		} \
	} while (0)

	char buf[256];

	// some easy cases first
	TEST_WRAPPER("/",         "/");
	TEST_WRAPPER("/.",        "/");
	TEST_WRAPPER("//",        "/");
	TEST_WRAPPER("/asdf",     "/asdf");
	TEST_WRAPPER("/asdf/",    "/asdf/");
	TEST_WRAPPER("/asdf//",   "/asdf/");
	TEST_WRAPPER("/asdf/./",  "/asdf/");
	TEST_WRAPPER("/a/./b",    "/a/b");
	TEST_WRAPPER("/a/./b/",   "/a/b/");

	// some slightly less easy cases
	TEST_WRAPPER("/asdf/..",  "/");
	TEST_WRAPPER("/asdf/../", "/");
	TEST_WRAPPER("/asdf/.",   "/asdf/");
	TEST_WRAPPER("/asdf//.",  "/asdf/");

	// going under the root or close to it
	TEST_WRAPPER("/..",        0);
	TEST_WRAPPER("/../asdf",   0);
	TEST_WRAPPER("/../asdf/",  0);
	TEST_WRAPPER("/./a/../..", 0);
	TEST_WRAPPER("/a/a/../..", "/");
	TEST_WRAPPER("/a/../a/..", "/");
	TEST_WRAPPER("/a/../../a", 0);
	TEST_WRAPPER("/../a/../a", 0);
	TEST_WRAPPER("/../../a/a", 0);
	TEST_WRAPPER("/////../..", 0);
	TEST_WRAPPER("//a//../..", 0);

	// relative paths aren't allowed
	TEST_WRAPPER("relative",   0);
	TEST_WRAPPER("some/stuff", 0);
	TEST_WRAPPER("./stuff",    0);
	TEST_WRAPPER("../stuff",   0);
	TEST_WRAPPER("",           0);
#undef TEST_WRAPPER
}

TEST(vfs_mount_resolve) {
	struct vfs_mount *mount = NULL;

#define ADD_MOUNT(path) do { \
		struct vfs_mount *mount2 = kmalloc(sizeof(struct vfs_mount)); \
		mount2->prefix = path; \
		mount2->prefix_len = sizeof(path) - 1; \
		mount2->prev = mount; \
		mount = mount2; \
	} while (0)

	ADD_MOUNT(""); // root mount
	ADD_MOUNT("/dir/shadowed");
	ADD_MOUNT("/dir");

#undef ADD_MOUNT

#define TEST_WRAPPER(path, expected) do { \
		struct vfs_mount *mount2 = vfs_mount_resolve(mount, path, sizeof(path) - 1); \
		TEST_COND((mount2->prefix_len == sizeof(expected) - 1) \
		       && (0 == memcmp(mount2->prefix, expected, mount2->prefix_len))); \
	} while (0)

	TEST_WRAPPER("/",              "");
	TEST_WRAPPER("/test",          "");
	TEST_WRAPPER("/dir",           "/dir");
	TEST_WRAPPER("/dirry",         "");
	TEST_WRAPPER("/dir/",          "/dir");
	TEST_WRAPPER("/dir/shadowed",  "/dir");
	TEST_WRAPPER("/dir/shadowed/", "/dir");

#undef TEST_WRAPPER
}

void tests_vfs(void) {
	TEST_RUN(path_simplify);
	TEST_RUN(vfs_mount_resolve);
}
