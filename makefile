all: BootLoader Kernel32 Kernel64 Disk.img Application

BootLoader:
	@echo BootLoader Build Start
	make -C 00.BootLoader
	@echo BootLoader Build End

Kernel32:
	@echo Kernel Build Start
	make -C 01.Kernel32
	@echo Kernel Build End

Kernel64:
	@echo Kernel Build Start
	make -C 02.Kernel64
	@echo Kernel Build End

Disk.img: 00.BootLoader/BootLoader.bin 01.Kernel32/Kernel32.bin 02.Kernel64/Kernel64.bin
	@echo Disk Image Build Start
	python3 04.Utility/00.ImageMaker/ImageMaker.py 
	@echo Disk Image Build End

Application:
	@echo Application Build Start
	make -C 03.Application
	@echo Application Build End

clean:
	make -C 00.BootLoader clean
	make -C 01.Kernel32 clean
	make -C 02.Kernel64 clean
	make -C 03.Application clean
	rm -f Disk.img