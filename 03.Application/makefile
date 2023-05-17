all: MINT64OSLibrary HelloWorld TextViewer

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

clean:
	make -C UserLibrary clean
	make -C 00.HelloWorld clean
	make -C 00.TextViewer clean