CC=$(CROSS)gcc
AR=$(CROSS)ar
STRIP=$(CROSS)strip
CFLAGS = -Wall -O2 -I .
CFLAGS = -Wall -g -I .
C_FLAGS=
arm-elf-C_FLAGS= -D PRG_MODE_DM270
CFLAGS += $($(CROSS)C_FLAGS)
L_FLAGS= -L .
arm-elf-L_FLAGS= -Wl,-elf2flt="-s 262144" -L .
LFLAGS += $($(CROSS)L_FLAGS)

PORTACFILES=rwporta.c utilsporta.c rand.c
PORTAOFILES=rwporta.o utilsporta.o rand.o
PORTAHFILES=rwporta.h utilsporta.h errorporta.h rand.h
TESTPATH=/mnt/temp1
INSTALLPATH=/hanishkvc/samples/fatfs
RELEASEPATH=/hanishkvc/samples/fatfs/fatfs-release
arm-elf-INSTALLPATH=/experiments/src/forBoard24-27Oct2004/uClinux-dist-2003/prop

FATFSCFILES=fatfs.c fsutils.c partk.c bdfile.c bdhdd.c linuxutils.c
FATFSOFILES=fatfs.o fsutils.o partk.o bdfile.o bdhdd.o linuxutils.o
FATFSHFILES=fatfs.h partk.h bdfile.h inall.h bdhdd.h linuxutils.h bdk.h errs.h
arm-elf-FATFS_CFILES=bdh8b16.c dm270utils.c
arm-elf-FATFS_OFILES=bdh8b16.o dm270utils.o
arm-elf-FATFS_HFILES=bdh8b16.h dm270utils.h
FATFS_CFILES=bdh8b16.c
FATFS_OFILES=bdh8b16.o
FATFS_HFILES=bdh8b16.h
FATFSCFILES+=$($(CROSS)FATFS_CFILES)
FATFSOFILES+=$($(CROSS)FATFS_OFILES)
FATFSHFILES+=$($(CROSS)FATFS_HFILES)

TESTFATS=$(CROSS)testfat $(CROSS)testfat-d $(CROSS)testfat_pm $(CROSS)testfat_pm-d
TESTFATS=$(CROSS)testfat $(CROSS)testfat_pm $(CROSS)testfat_pm-d
TESTFATS=$(CROSS)testfat_pm $(CROSS)testfat_pm-d
TESTFATS_FLTMISC=$(CROSS)testfat_pm.gdb $(CROSS)testfat_pm-d.gdb
LIBFATFSS=lib$(CROSS)fatfs_pm.a lib$(CROSS)fatfs_pm-d.a
LIBFATFSCFILES= $(FATFSCFILES) $(PORTACFILES)
LIBFATFSOFILES= $(FATFSOFILES) $(PORTAOFILES)
LIBFATFSHFILES= $(FATFSHFILES) $(PORTAHFILES)

all: libfatfs $(CROSS)testfat 

libfatfs: $(FATFSCFILES) $(FATFSHFILES) $(PORTAHFILES) $(PORTACFILES)
#	$(CC) $(CFLAGS) -c $(LIBFATFSCFILES)
#	$(AR) -cru lib$(CROSS)fatfs.a $(LIBFATFSOFILES)
#	$(CC) $(CFLAGS) -c $(LIBFATFSCFILES) -D MAKE_DEBUG
#	$(AR) -cru lib$(CROSS)fatfs-d.a $(LIBFATFSOFILES)
	$(CC) $(CFLAGS) -c $(LIBFATFSCFILES) -D FATFS_FAT_PARTLYMAPPED
	$(AR) -cru lib$(CROSS)fatfs_pm.a $(LIBFATFSOFILES)
	$(CC) $(CFLAGS) -c $(LIBFATFSCFILES) -D FATFS_FAT_PARTLYMAPPED -D MAKE_DEBUG
	$(AR) -cru lib$(CROSS)fatfs_pm-d.a $(LIBFATFSOFILES)

$(CROSS)testfat: testfat.c $(FATFSCFILES) $(FATFSHFILES) $(PORTAHFILES) $(PORTACFILES)
#	$(CC) $(CFLAGS) $(LFLAGS) -o $(CROSS)testfat testfat.c -l$(CROSS)fatfs
#	$(CC) $(CFLAGS) $(LFLAGS) -o $(CROSS)testfat-d testfat.c -l$(CROSS)fatfs-d -D MAKE_DEBUG
	$(CC) $(CFLAGS) $(LFLAGS) -o $(CROSS)testfat_pm testfat.c -l$(CROSS)fatfs_pm -D FATFS_FAT_PARTLYMAPPED
	$(CC) $(CFLAGS) $(LFLAGS) -o $(CROSS)testfat_pm-d testfat.c -l$(CROSS)fatfs_pm-d -D FATFS_FAT_PARTLYMAPPED -D MAKE_DEBUG

benchmark: bdhdd.c bdhdd.h
	cpp -DBDHDD_BENCHMARK -DBDHDD_OPTIGETSECTOR_LOOP16 bdhdd.c > bdhdd_benchmark.c

porta:
	./hkvc-porta_setup.sh $(PORTACFILES) $(PORTAHFILES)

release: libfatfs
	rm -rf $(RELEASEPATH) || /bin/true
	mkdir $(RELEASEPATH) $(RELEASEPATH)/include $(RELEASEPATH)/src $(RELEASEPATH)/lib
	cp $(LIBFATFSHFILES) $(RELEASEPATH)/include
	cp testfat*.* $(RELEASEPATH)/src
	cp MakefileUser $(RELEASEPATH)/src/Makefile
	cp $(LIBFATFSS) $(RELEASEPATH)/lib
#	$(STRIP) $(RELEASEPATH)/lib/*

install:
	mv $(TESTFATS) $($(CROSS)INSTALLPATH)/
	cp Makefile $($(CROSS)INSTALLPATH)/Disk16Make
	cp hkvc-fatfs_verifyfileseektest.sh /tmp/

clean:
	rm $(TESTFATS) || /bin/true
	rm $(TESTFATS_FLTMISC) || /bin/true
	rm *.o *.a core || /bin/true

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
	echo "helo" >> $(TESTPATH)/dir2/rand4.log
	dd if=/dev/zero of=$(TESTPATH)/dir2/zero4.log bs=1024 count=4096 || /bin/true
	cp $(TESTPATH)/dir2/rand4.log /tmp/rand4.log
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

