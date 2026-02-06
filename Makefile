sudokuboard.ps: sudoku Makefile
	./sudoku # -s "thonkyveryhard"

sudoku:	sudoku.cc
	g++ -g $< -o $@

battleship: battleship.c
	gcc -g $< -o $@

foo.ps: battleship
	./battleship > $@

clean:
	bye;rm -f sudoku battleship foo.ps
