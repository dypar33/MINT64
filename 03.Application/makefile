all: MINT64OSLibrary HelloWorld TextViewer Shooter Hexa HangulViewer OneLineMemo

MINT64OSLibrary:
	@echo Build UserLibrary
	make -C UserLibrary
	@echo Build Complete

HelloWorld:
	@echo Build HelloWorld
	make -C 00.HelloWorld
	@echo Build Complete

TextViewer:
	@echo Build TextViewer
	make -C 01.TextViewer
	@echo Build Complete

Shooter:
	@echo Build Shooter
	make -C 02.Shooter
	@echo Build Complete

Hexa:
	@echo Build Shooter
	make -C 03.Hexa
	@echo Build Complete

HangulViewer:
	@echo Build Shooter
	make -C 04.HangulViewer
	@echo Build Complete

OneLineMemo:
	@echo Build Shooter
	make -C 05.OneLineMemo
	@echo Build Complete

clean:
	make -C UserLibrary clean
	make -C 00.HelloWorld clean
	make -C 01.TextViewer clean
	make -C 02.Shooter clean
	make -C 03.Hexa clean
	make -C 04.HangulViewer clean
	make -C 05.OneLineMemo clean