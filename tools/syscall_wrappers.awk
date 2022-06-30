BEGIN {
	print "\
/* generated by tools/syscall_wrappers.awk\n\
 * don't modify manually, instead run:\n\
 *     make src/init/syscalls.c\n\
 */\n\
#include <shared/syscalls.h>\n\
\n";
}

/_syscall\(/ { next; } # skipping _syscall(), it's implemented elsewhere

/\);/ {
	sub(/;/, " {");
	print $0;

	name = substr($0, match($0, /_syscall_[^(]+/), RLENGTH);
	rets = substr($0, 0, RSTART - 1);
	sub(/ *$/, "", rets)

	params = substr($0, match($0, /\(.+\)/) + 1, RLENGTH - 2);
	if (params == "void") params = ""

	split(params, p, /,/);
	for (i = 0; i <= 4; i += 1) {
		if (p[i]) {
			# p[i] is a parameter, convert it into an expression to pass to _syscall()
			sub(/^ */, "", p[i]); # strip
			split(p[i], words, / /);
			if (length(words) != 1) {
				var = words[length(words)];
				sub(/\*/, "", var);
				if (words[1] != "int") var = "(int)" var;
			}
			p[i] = var;
		} else {
			p[i] = 0;
		}
	}

	printf "\t";
	if (!index($0, "_Noreturn")) {
		printf "return ";
		if (rets != "int") printf "(%s)", rets;
	}
	printf "_syscall(%s, %s, %s, %s, %s);\n", toupper(name), p[1], p[2], p[3], p[4];
	if (index($0, "_Noreturn")) print "\t__builtin_unreachable();";

	print "}\n";
}
