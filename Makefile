CC=$(CROSS)gcc
CFLAGS = -Wall -O -g -I .
C_FLAGS=
arm-elf-C_FLAGS= -Wl,-elf2flt
CFLAGS += $($(CROSS)C_FLAGS)

PORTACFILES=rwporta.c utilsporta.c
PORTAHFILES=rwporta.h utilsporta.h errorporta.h
TESTPATH=/mnt/temp1
INSTALLPATH=/hanishkvc/samples/fatfs

FATFSCFILES=fatfs.c fsutils.c partk.c bdfile.c bdhdd.c
FATFSHFILES=fatfs.h partk.h bdfile.h inall.h bdhdd.h

all: $(CROSS)testfat

$(CROSS)testfat: testfat.c $(FATFSCFILES) $(FATFSHFILES) $(PORTAHFILES) $(PORTACFILES)
	$(CC) $(CFLAGS) -o $(CROSS)testfat testfat.c $(FATFSCFILES) $(PORTACFILES)

porta:
	./hkvc-porta_setup.sh $(PORTACFILES) $(PORTAHFILES)

install:
	mv testfat $(INSTALLPATH)/

clean:
	rm $(CROSS)testfat || /bin/true
	rm $(CROSS)testfat.gdb || /bin/true
	rm *.o core || /bin/true

allclean: clean
	rm bdf.bd || /bin/true
	rm tags || /bin/true
	rm -i *.log || /bin/true

16M: disk16M s16M f16M
16M32: disk16M32 s16M f16M

disk16M:
	rm -f /tmp/16M.bd
	dd if=/dev/zero of=/tmp/16M.bd bs=4096 count=4096
	/sbin/mkdosfs -F 16 -v /tmp/16M.bd > /tmp/16M.meta

disk16M32:
	rm -f /tmp/16M.bd
	dd if=/dev/zero of=/tmp/16M.bd bs=4096 count=4096
	/sbin/mkdosfs -F 32 -v /tmp/16M.bd > /tmp/16M.meta
s16M:
	rm -f bdf.bd
	ln -s /tmp/16M.bd bdf.bd
f16M:
	mount -t vfat -o loop bdf.bd $(TESTPATH)
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
	dd if=/dev/zero of=$(TESTPATH)/zero3.log bs=4096 count=128
	echo "0987654321" > $(TESTPATH)/reallylongfilenamereallyrealylongfilenamereallyreallyreallylongfilename.log
	mkdir $(TESTPATH)/dir1
	echo "0987654321" > $(TESTPATH)/dir1/reallyDIR1longfilename.log
	mkdir $(TESTPATH)/dir1/dir11
	echo "0987654321" > $(TESTPATH)/dir1/dir11/reallyDIR11longfilename.log
	mkdir $(TESTPATH)/dir2
	echo "0987654321" > $(TESTPATH)/dir2/reallyDIR2longfilename.log
	mkdir $(TESTPATH)/dir2/dir21
	echo "0987654321" > $(TESTPATH)/dir2/dir21/reallyDIR21longfilename.log
	dd if=/dev/urandom of=$(TESTPATH)/dir2/rand4.log bs=2048 count=4096 || /bin/true
	dd if=/dev/zero of=$(TESTPATH)/dir2/zero4.log bs=1024 count=4096 || /bin/true
	rm $(TESTPATH)/abc2.txt
	rm $(TESTPATH)/abc3.txt
	rm $(TESTPATH)/abc4.txt
	rm $(TESTPATH)/abc6.txt
	rm $(TESTPATH)/zero3.log
	dd if=/dev/urandom of=$(TESTPATH)/spread.fil bs=512 count=16
	umount $(TESTPATH)

test: testfat
	./testfat n spread.fil > check_spreadfil.log

disk2G:
	rm -f /tmp/2G.bd
	dd if=/dev/zero of=/tmp/2G.bd bs=4096 seek=500000 count=10
	/sbin/mkdosfs -v -F 32 /tmp/2G.bd > /tmp/2G.meta
s2G:
	rm -f bdf.bd
	ln -s /tmp/2G.bd bdf.bd

rwporta:
	diff rwporta.c ../../porta/rwporta.c
	rm rwporta.c
	ln -s ../../porta/rwporta.c rwporta.c
	diff rwporta.h ../../porta/rwporta.h
	rm rwporta.h
	ln -s ../../porta/rwporta.h rwporta.h

