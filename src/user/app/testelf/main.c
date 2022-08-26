#include <stdio.h>
#include <string.h>
#include <unistd.h>

const char *str = "Hello!", *str2 = "World.";

int main(int argc, char **argv) {
	printf("elftest's &main == 0x%x\n", &main);
	printf("%s %s\n", str, str2);
	printf("argc == %u\n", argc);
	for (int i = 0; i < argc; i++)
		printf("argv[%u] == 0x%x == \"%s\"\n", i, argv[i], argv[i]);
	if (strcmp(argv[1], "stackexec") == 0) {
		/* exec something with arguments on the stack */
		const char s_d[] = "I am a pretty long string on the stack. Oh my. " \
			"I hope I won't get corrupted.\0";
		char s[sizeof(s_d)];
		memcpy(s, s_d, sizeof(s_d));
		const char *argv2[] = {argv[0], s, s, "hello", s, s, s, "lol", NULL};
		printf("argv2 == 0x%x, s == 0x%x\n== exec ==\n", argv2, s);
		execv(argv[0], (void*)argv2);
		puts("stackexec failed");
	}
	return 0;
}
