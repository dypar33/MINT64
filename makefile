all: BootLoader Kernel32 Disk.img 

BootLoader:
	@echo BootLoader Build Start
	make -C 00.BootLoader
	@echo BootLoader Build End

Kernel32:
	@echo Kernel Build Start
	make -C 01.Kernel32
	@echo Kernel Build End

Disk.img: 00.BootLoader/BootLoader.bin 01.Kernel32/Kernel32.bin
	@echo Disk Image Build Start
	python3 00.ImageMaker/ImageMaker.py
	@echo Disk Image Build End

clean:
	make -C 00.BootLoader clean
	make -C 01.Kernel32 clean
	rm -f Disk.img