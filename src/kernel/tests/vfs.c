#include <kernel/mem/alloc.h>
#include <kernel/tests/base.h>
#include <kernel/tests/tests.h>
#include <kernel/vfs/mount.h>
#include <shared/mem.h>

TEST(vfs_mount_resolve) {
	struct vfs_mount *mount = NULL;

#define ADD_MOUNT(path) do { \
		struct vfs_mount *mount2 = kmalloc(sizeof *mount2); \
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

	while (mount != NULL) {
		struct vfs_mount *tmp = mount;
		mount = mount->prev;
		kfree(tmp);
	}
}

void tests_vfs(void) {
	TEST_RUN(vfs_mount_resolve);
}
