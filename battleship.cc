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
 x a small mark to indicate something's there but we don't know what
 . we don't know. 
*/

char **board;
const char *iboard[] = {
  /* " |6050060003|", */
  /* "1|s.........|", */
  /* "3|..s..s...u|", */
  /* "1|.........m|", */
  /* "4|u.u..u...d|", */
  /* "3|m.d..m....|", */
  /* "2|m....d....|", */
  /* "1|d.........|", */
  /* "1|..u.......|", */
  /* "2|..d..u....|", */
  /* "2|s....d....|", */
  /* 0, */
  /* " |6050060003|", */
  /* "1|..........|", */
  /* "3|.....s....|", */
  /* "1|..........|", */
  /* "4|u.x..u...x|", */
  /* "3|m....m....|", */
  /* "2|m....d....|", */
  /* "1|d.........|", */
  /* "1|..u.......|", */
  /* "2|..m..u....|", */
  /* "2|..d..d....|", */
  /* 0, */
  " |1251501032|",
  "3|..........|",
  "3|..u.......|",
  "2|..........|",
  "1|..........|",
  "1|.w........|",
  "3|.........u|",
  "1|..........|",
  "5|..........|",
  "1|..........|",
  "0|..........|",
  0,
};

const char ps_preamble[] =
R"(%!PS-Adobe
/Times-Roman findfont 8 scalefont setfont
62 230 translate
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
/big_x { % c r
 newpath
 to_coord % x y
 linespace 2 div add % x+off y
 exch % y x+off
 linespace 2 div add % y+off x+off
 exch % x+off y+off
 linespace 6 div 0 360 arc closepath
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

void putpsstr(int r, const char *s) {
  for (int c = 1; s[c]; ++c) {
    switch(s[c]) {
    case 's':
      printf("%d %d submarine\n", c-1, r);
      break;
    case 'm':
      printf("%d %d midship\n", c-1, r);
      break;
    case 'x':
      printf("%d %d big_x\n", c-1, r);
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

void makewater(int r, int c) {
  if (r < 1 || c < 2 || !board[r] || board[r][c] == '|')
    return;
  board[r][c] = 'w';
}

// The eight cells around r and c except for one
void makewater_ring(int r, int c, int dr, int dc) {
  int wr, wc;
  for (wr = -1; wr <= 1; ++wr) {
    for (wc = -1; wc <= 1; ++wc) {
      if (wr == 0 && wc == 0) continue;
      if (wr == dr && wc == dc) continue;
      makewater(r+wr, c+wc);
    }
  }
}

void fill_in_counts_and_water() {
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
    int rowcount = 0;
    for (int c = 2;board[0][c] && board[0][c] != '|'; ++c) {
      if (board[r][c] != '.' && board[r][c] != 'w')
	++rowcount;
    }
    // turn unknowns into water if rowcount or colcount is already
    // satisfied.
    for (int c = 2;board[0][c]; ++c) {
      if ((board[r][0] - '0' <= rowcount ||
	   board[0][c] - '0' <= colcount[c]) &&
	  board[r][c] == '.') {
	board[r][c] = 'w';
      }
      // put water around known boat parts
      switch (board[r][c]) {
        // l end of boat pointing left
      case 'l':
        makewater_ring(r, c, 0, 1);
        break;
      // r end of boat pointing right
      case 'r':
        makewater_ring(r, c, 0, -1);
        break;
      // u end of boat pointing up
      case 'u':
        makewater_ring(r, c, 1, 0);
        break;
      // d end of boat pointing down
      case 'd':
        makewater_ring(r, c, -1, 0);
        break;
      // s submarine!
      case 's':
        makewater_ring(r, c, 0, 0);
        break;
      }
    }
  }
}
const constexpr int BOARDSIZE = 10;
struct boat {
  int length;
  int x, y; // the x and y of lower left
  enum ORIENT { VERT, HORIZ, SUB } orient;
  boat(int len) {
    length = len;
    initpos();
  }
  void initpos() {
    init_x();
    init_y();
    y = 1;
    if (length > 1) {
      orient = VERT;
    } else {
      orient = SUB;
    }
  }
  void init_x() {
    x = 2;
  }
  void init_y() {
    y = 1;
  }
  boat* nextboat() {
    boat* rv = this + 1;
    return rv;
  }

  bool solve();
  
  void putboat(FILE* f) {
    fprintf(f, "boat l=%d x=%d y=%d %c\n",
            length, x, y,
            orient == SUB ? 's' : orient == VERT ? 'v' : 'h');
  }
  bool incr() {
    if (orient == VERT) {
      orient = HORIZ;
      return true;
    }
    if (orient != SUB) {
      orient = VERT;
    }
    if (x < BOARDSIZE+1) {
      ++x;
      return true;
    }
    init_x();
    if (y < BOARDSIZE) {
      ++y;
      return true;
    }
    return false;
  }
  bool it_fits();
};

bool boat::it_fits() {
  // what should either end of the boat look like
  char zeromatch='s';
  char maxmatch='s';
  if (length > 1) {
    switch (orient) {
    case VERT:
      zeromatch='d';
      maxmatch='u';
      break;
    case HORIZ:
      zeromatch='l';
      maxmatch='r';
      break;
    }
  }
  for (int i = 0; i < length; ++i) {
    for (int ov = -1; ov <= 1; ++ov) {
      for (int oh = -1; oh <= 1; ++oh) {
        if (ov == 0 && oh == 0) {
          // Looking at the boat
          char boat_match = zeromatch;
          if (i > 0) {
            boat_match = 'm';
          }
          if (i >= length - 1) {
            boat_match = maxmatch;
          }
          //          if (board[foofoo
        }
      }
    }
  }
  return true;
}

bool boat::solve() {
  if (length == 0)
    return true;
  return false;
}

boat boats[] = {
  boat(4),
  boat(3),
  boat(3),
  boat(2),
  boat(2),
  boat(2),
  boat(1),
  boat(1),
  boat(1),
  boat(1),
  boat(0),
};

int main() {
  fputs(ps_preamble, stdout);
  board = (char**)malloc(sizeof(iboard));
  for (int r = 0; r < sizeof(iboard)/sizeof(iboard[0]); ++r) {
    board[r] = iboard[r]?strdup(iboard[r]):0; // make the board writable
  }
  fill_in_counts_and_water();
  for (int i = 2; board[0][i] && board[0][i] != '|'; ++i) {
    printf("%d 0 (%c) bigstring\n", i-1, board[0][i]);
  }
  for (int r = 1; board[r]; ++r) {
    printf("0 %d (%c) bigstring\n", r, board[r][0]);
    putpsstr(r, board[r]);
  }
  printf("%% end of board\n");
  printf("gsave 1 10.5 to_coord translate\n");
  printf(".35 .35 scale\n");
  putpsstr(11, "|lmmr lmr lmr lr lr lr s s s s|");
  printf("grestore\n");

  putpsstr(13, "|lmmrlmrlmr|");
  putpsstr(14, "|lrlrlrssss|");

  printf("showpage\n");

  // TESTS
  // fprintf(stderr, "ALL BOATS\n");
  // for (boat* b = boats; b->length; b = b->nextboat()) {
  //   b->putboat(stderr);
  // }
  // const constexpr int SHOWBOAT = 6;
  // fprintf(stderr, "INCR BOATS[SHOWBOAT]\n");
  // while (true) {
  //   boats[SHOWBOAT].putboat(stderr);
  //   if (!boats[SHOWBOAT].incr())
  //     break;
  // }

  exit(0);
}

