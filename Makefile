compile: manager.c
	@echo Compiling...
	gcc manager.c -o manager -lmenu -lncurses
	@echo Done.
	@echo Creating files...
	touch log.txt
	@echo Done.

clean:
	@echo Deleting files...
	rm manager
	rm log.txt
