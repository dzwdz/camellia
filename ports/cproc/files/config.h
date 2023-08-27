static const char target[]               = "x86_64-camellia";
static const char *const startfiles[]    = {"-l", ":crt0.o"};
static const char *const endfiles[]      = {"-l", "c"};
static const char *const preprocesscmd[] = {
	"cpp",

	/* clear preprocessor GNU C version */
	"-U", "__GNUC__",
	"-U", "__GNUC_MINOR__",

	/* we don't yet support these optional features */
	"-D", "__STDC_NO_ATOMICS__",
	"-D", "__STDC_NO_COMPLEX__",
	"-D", "__STDC_NO_VLA__",
	"-U", "__SIZEOF_INT128__",

	/* we don't generate position-independent code */
	"-U", "__PIC__",

	/* ignore attributes and extension markers */
	"-D", "__attribute__(x)=",
	"-D", "__extension__=",
};
static const char *const codegencmd[]    = {"qbe"};
static const char *const assemblecmd[]   = {"as"};
static const char *const linkcmd[]       = {"ld", "-no-dynamic-linker", "-pie"};
