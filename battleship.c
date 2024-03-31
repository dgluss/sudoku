#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Battleship board print.
 w water
 m mid part of boat
 l end of boat pointing left
 r end of boat pointing right
 u end of boat pointing up
 d end of boat pointing down
 s submarine!
 . we don't know. 
*/

char *board[] = {
  " |0231150431|",
  "1|..........|",
  "3|..........|",
  "2|..........|",
  "4|........lr|",
  "0|..........|",
  "3|...l......|",
  "2|..........|",
  "2|..........|",
  "2|..........|",
  "1|.....s....|",
  0,
};

const char ps_preamble[] =
R"(%!PS-Adobe
/Times-Roman findfont 8 scalefont setfont
54 160 translate
/linespace 50 def
/numlines 10 def
/linelen { linespace numlines mul } def
% bigstring centers the string in the box
/bigstring { % i j string
 /fsize 32 def
 /Times-Roman findfont fsize scalefont setfont
 3 1 roll % string i j
 to_coord 3 -1 roll % x y string
 dup % x y string string
 stringwidth % x y string h v
 pop % x y string h
 2 div neg % x y string add_to_x
 4 -1 roll % y string add_to_x x
 add % y string left_x
 3 -1 roll % string left_x y
 moveto % string
 linespace 2 div linespace fsize .8 mul sub 2 div rmoveto
 show
} def
% smallstring goes at the top left
/smallstring { % i j string
 /fsize 14 def
 /Times-Roman findfont fsize scalefont setfont
 3 1 roll % string x y
 to_coord
 moveto % string
 linespace .1 mul linespace .7 mul rmoveto
 show
} def
/submarine { % c r
 newpath to_coord
 linespace 2 div add
 exch
 linespace 2 div add
 exch
 linespace 2.5 div 0 360 arc closepath
 0 setgray fill
} def
/midship { % c r
 newpath to_coord
 linespace .1 mul add
 exch
 linespace .1 mul add
 exch
 moveto
 0 linespace .8 mul rlineto
 linespace .8 mul 0 rlineto
 0 linespace -.8 mul rlineto
 linespace -.8 mul 0 rlineto
 closepath 0 setgray fill
} def
/endpart { % rot col row
 gsave
 to_coord
 linespace .5 mul add
 exch
 linespace .5 mul add
 exch
 translate
 rotate
 newpath
 linespace -.4 mul linespace .6 mul
 linespace 270 323.13 arc
 linespace -.4 mul linespace -.6 mul
 linespace 36.87 90 arc
 closepath
 0 setgray fill
 grestore
} def
/water { % col row
 gsave
 to_coord
 translate % I'm at the lower left corner
 0 1 2 {
  linespace .3 mul mul linespace .45 mul add /v exch def
  newpath
  0 1 2 {
   linespace .3 mul mul linespace .2 mul add /h exch def
   h v linespace .3 mul 240 300 arc
   stroke
  } for
 } for
 grestore
} def
% calculate lower left coords of x,y
/to_coord {
 numlines sub neg linespace mul
 exch
 1 sub linespace mul
 exch
} def
/hline { 0 exch moveto linelen 0 rlineto stroke  } def
/vline { 0 moveto 0 linelen rlineto stroke } def
.5 setlinewidth
0 linespace linelen { hline } for
0 linespace linelen { vline } for
% heavier lines every N lines, or 0 for none:
/heavyspace 0 def

heavyspace 0 gt
{ 2 setlinewidth
  /sp heavyspace linespace mul def
  0 sp linelen { hline } for
  0 sp linelen { vline } for
} if
)";

void putpsstr(int r, char *s) {
  for (int c = 1; s[c]; ++c) {
    switch(s[c]) {
    case 's':
      printf("%d %d submarine\n", c-1, r);
      break;
    case 'm':
      printf("%d %d midship\n", c-1, r);
      break;
    case 'r':
      printf("0 %d %d endpart\n", c-1, r);
      break;
    case 'u':
      printf("90 %d %d endpart\n", c-1, r);
      break;
    case 'l':
      printf("180 %d %d endpart\n", c-1, r);
      break;
    case 'd':
      printf("270 %d %d endpart\n", c-1, r);
      break;
    case 'w':
      printf("%d %d water\n", c-1, r);
      break;
    }
  }
}

void fill_in_water() {
  int w = strlen(board[0]);
  int colcount[w];
  for (int c = 2;c < w;++c) {
    colcount[c] = 0;
    for (int r = 1;board[r];++r) {
      if (board[r][c] != '.' && board[r][c] != 'w')
	++colcount[c];
    }
  }
  for (int r = 1; board[r]; ++r) {
    board[r] = strdup(board[r]); // make writable
    int rowcount = 0;
    for (int c = 2;board[0][c] && board[0][c] != '|'; ++c) {
      if (board[r][c] != '.' && board[r][c] != 'w')
	++rowcount;
    }
    for (int c = 2;board[0][c]; ++c) {
      if ((board[r][0] - '0' <= rowcount ||
	   board[0][c] - '0' <= colcount[c]) &&
	  board[r][c] == '.')
	board[r][c] = 'w';
    }
  }
}

int main() {
  fputs(ps_preamble, stdout);
  fill_in_water();
  for (int i = 2; board[0][i] && board[0][i] != '|'; ++i) {
    printf("%d 0 (%c) bigstring\n", i-1, board[0][i]);
  }
  for (int r = 1; board[r]; ++r) {
    printf("0 %d (%c) bigstring\n", r, board[r][0]);
    putpsstr(r, board[r]);
  }
  printf("%% end of board\n");
  printf("1 11 to_coord translate\n");
  printf(".35 .35 scale\n");
  putpsstr(10, " lmmr lmr lmr lr lr lr s s s s");
  printf("showpage\n");
  exit(0);
}
