#include <stdio.h>

const char *str = "Hello!\n", *str2 = "World.\n";

int main(void) {
	printf("elftest's &main == 0x%x\n", &main);
	printf("%s%s", str, str2);
	return 0;
}
