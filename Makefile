CC=$(CROSS)gcc
CFLAGS = -Wall -O -g -I .
C_FLAGS=
arm-elf-C_FLAGS= -Wl,-elf2flt
CFLAGS += $($(CROSS)C_FLAGS)

TESTPATH=/mnt/temp1

all: $(CROSS)testfat

$(CROSS)testfat: testfat.c fatfs.c fatfs.h bdfile.c bdfile.h part.c part.h rwporta.c rwporta.h inall.h
	$(CC) $(CFLAGS) -o $(CROSS)testfat testfat.c fatfs.c bdfile.c part.c rwporta.c

clean:
	rm $(CROSS)testfat || /bin/true
	rm $(CROSS)testfat.gdb || /bin/true

16M: disk16M s16M f16M

disk16M:
	rm -f /tmp/16M.bd
	dd if=/dev/zero of=/tmp/16M.bd bs=4096 count=4096
	/sbin/mkdosfs -v /tmp/16M.bd > /tmp/16M.meta
s16M:
	rm -f bd.bd
	ln -s /tmp/16M.bd bd.bd
f16M:
	mount -t vfat -o loop bd.bd $(TESTPATH)
	echo "abcdefgh" > $(TESTPATH)/abc1.txt
	dd if=/dev/zero of=$(TESTPATH)/zero1.log bs=4096 count=1024	
	echo "abcdefgh" > $(TESTPATH)/abc3.txt
	echo "abcdefgh" > $(TESTPATH)/abc2.txt
	dd if=/dev/urandom of=$(TESTPATH)/rand1.ran bs=4096 count=4	
	echo "abcdefgh" > $(TESTPATH)/abc4.txt
	echo "xyz" > $(TESTPATH)/aBc5.txt
	dd if=/dev/zero of=$(TESTPATH)/zero2.log bs=4096 count=512
	echo "1234567890" > $(TESTPATH)/longfilename.txt
	echo "abcdefgh" > $(TESTPATH)/abc6.txt
	dd if=/dev/zero of=$(TESTPATH)/zero3.log bs=4096 count=512
	echo "0987654321" > $(TESTPATH)/reallylongfilenamereallyrealylongfilenamereallyreallyreallylongfilename.log
	dd if=/dev/zero of=$(TESTPATH)/zero4.log bs=4096 count=4096 || /bin/true
	rm $(TESTPATH)/abc2.txt
	rm $(TESTPATH)/abc3.txt
	rm $(TESTPATH)/abc4.txt
	rm $(TESTPATH)/abc6.txt
	dd if=/dev/zero of=$(TESTPATH)/spread.fil bs=512 count=16
	umount $(TESTPATH)

t16M: testfat
	./testfat spread.fil > check_spreadfil.log

disk2G:
	rm -f /tmp/2G.bd
	dd if=/dev/zero of=/tmp/2G.bd bs=4096 seek=500000 count=10
	/sbin/mkdosfs -v -F 32 /tmp/2G.bd > /tmp/2G.meta
s2G:
	rm -f bd.bd
	ln -s /tmp/2G.bd bd.bd

