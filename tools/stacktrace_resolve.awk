/k\/[0-9A-Fa-f]{8}/ {
	print;
	addr = substr($0, match($0, /k\/[0-9A-Fa-f]{8}/) + 2, RLENGTH - 2);
	if (addr != "00000000") {
		printf "    ";
		system("addr2line -psfe out/fs/boot/kernel.bin 0x" addr);
	}
	next;
}

{ print; }
