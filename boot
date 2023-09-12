#!/bin/sh
set -eu

disk() {
	echo "-drive file=$1,format=raw,media=disk"
}

if [ -n "${QFLAGS:-}" ]; then
	echo "QFLAGS in env"
fi
QFLAGS="-no-reboot -m 1g -gdb tcp::12366 $(disk out/boot.iso) $(disk out/fs.e2) ${QFLAGS:-}"
QEMU=qemu-system-x86_64

QDISPLAY="-display none"
QNET="-nic user,model=rtl8139,mac=52:54:00:ca:77:1a,net=192.168.0.0/24,hostfwd=tcp::12380-192.168.0.11:80,id=n1"
QKVM="-enable-kvm"
QTTY="-serial mon:stdio"

DRY_RUN=
TEST_RUN=

for opt; do
	case "$opt" in
	--display|-d)
		QDISPLAY=
		;;
	--dry-run|-n)
		DRY_RUN=1
		;;
	--help|-h)
		echo $0 [opts]
		sed -ne '/\t-.*)/ {s/)//; s/|/, /g; p}' $0
		exit
		;;
	--pcap=*)
		PCAP="${opt#*=}"
		echo outputting pcap to $PCAP
		QNET="$QNET -object filter-dump,id=f1,netdev=n1,file=$PCAP"
		;;
	--no-kvm)
		QKVM=
		;;
	--test|-t)
		rm -vf out/qemu.in out/qemu.out
		mkfifo out/qemu.in out/qemu.out
		TEST_RUN=1
		QTTY="-serial pipe:out/qemu"
		;;
	--no-serial)
		QTTY="-serial none"
		;;
	*)
		echo "unknown option $opt"
		exit 1
		;;
	esac
done

CMD="$QEMU $QDISPLAY $QNET $QKVM $QTTY $QFLAGS"
if [ -n "$DRY_RUN" ]; then
	echo "$CMD"
elif [ -n "$TEST_RUN" ]; then
	echo "$CMD &"
	$CMD &

	sleep 1 # dunno

	# something eats the first character sent, so let's send a sacrificial newline
	echo > out/qemu.in
	# carry on with the tests
	echo tests > out/qemu.in
	echo halt > out/qemu.in
	cat out/qemu.out
else
	echo "$CMD"
	$CMD
fi
