all: OSLibrary HelloWorld TextViewer Shooter Hexa

OSLibrary:
	@echo 
	@echo ============= Build UserLibrary ===============
	@echo 
	
	make -C UserLibrary

	@echo 
	@echo =============== Build Complete ===============
	@echo 

HelloWorld:
	@echo 
	@echo ============= Build HelloWorld ===============
	@echo 
	
	make -C 00.HelloWorld

	@echo 
	@echo =============== Build Complete ===============
	@echo 

TextViewer:
	@echo 
	@echo ============= Build TextViewer ===============
	@echo 
	
	make -C 01.TextViewer

	@echo 
	@echo =============== Build TextViewer ===============
	@echo 

Shooter:
	@echo 
	@echo ============= Build Shooter ===============
	@echo 
	
	make -C 02.Shooter

	@echo 
	@echo =============== Build Shooter ===============
	@echo 

Hexa:
	@echo 
	@echo ============= Build Hexa ===============
	@echo 
	
	make -C 03.Hexa

	@echo 
	@echo =============== Build Hexa ===============
	@echo 

clean:
	make -C UserLibrary clean
	make -C 00.HelloWorld clean
	make -C 01.TextViewer clean
	make -C 02.Shooter clean
	make -C 03.Hexa clean