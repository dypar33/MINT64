all: BootLoader Disk.img

BootLoader:
	@echo BootLoader Build Start
	make -C 00.BootLoader
	@echo BootLoader Build End

Disk.img: 00.BootLoader/BootLoader.bin
	@echo Disk Image Build Start
	cp 00.BootLoader/BootLoader.bin Disk.img
	@echo Disk Image Build End

clean:
	make -C 00.BootLoader clean
	rm -f Disk.img