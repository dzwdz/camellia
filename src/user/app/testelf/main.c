#include <stdio.h>

const char *str = "Hello!", *str2 = "World.";

int main(int argc, char **argv, char **envp) {
	printf("elftest's &main == 0x%x\n", &main);
	printf("%s %s\n", str, str2);
	printf("argc == 0x%x\n", argc);
	for (int i = 0; i < argc; i++)
		printf("argv[0x%x] == 0x%x == \"%s\"\n", i, argv[i], argv[i]);
	return 0;
}
