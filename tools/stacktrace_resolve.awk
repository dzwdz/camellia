/k\/[0-9A-Fa-f]{8}/ {
	print;
	while (match($0, /k\/[0-9A-Fa-f]{8}/)) {
		addr = substr($0, RSTART + 2, RLENGTH - 2);
		if (addr != "00000000") {
			printf "    ";
			system("addr2line -psfe out/fs/boot/kernel.bin 0x" addr);
		}
		$0 = substr($0, RSTART + RLENGTH);
	}
	next;
}

{ print; }
