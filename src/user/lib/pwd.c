#include <grp.h>
#include <pwd.h>
#include <string.h>

struct passwd *
getpwuid(uid_t uid)
{
	static struct passwd p;
	memset(&p, 0, sizeof p);
	p.pw_name = "unnamed";
	p.pw_uid = uid;
	return &p;
}

struct group *
getgrgid(gid_t gid)
{
	static struct group g;
	memset(&g, 0, sizeof g);
	g.gr_name = "unnamed";
	g.gr_gid = gid;
	return &g;
}
