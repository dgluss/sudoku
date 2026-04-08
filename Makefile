want:	sudokuboard.ps
want:	foo.ps

sudokuboard.ps: sudoku Makefile
	./sudoku # -s "thonkyveryhard"

sudoku:	sudoku.cc
	g++ -g $< -o $@

battleship: battleship.cc
	g++ -g $< -o $@

foo.ps: Makefile battleship
	./battleship -s  > $@

clean:
	bye;rm -f sudoku battleship foo.ps
