#include <cassert>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Battleship board print/solve.
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

int add_water = false;
int debug = false;
int pb = false;
int solve = false;

const constexpr int MIN_C = 2;
const constexpr int MAX_C = 11;
const constexpr int MIN_R = 1;
const constexpr int MAX_R = 10;

char **board;
// https://www.conceptispuzzles.com/index.aspx?uri=puzzle/euid/020000008ea2a68b6f7f77641092ad10a6661488c2778ec728f86f07893a9a51df11bdad1639c03b75ebf219ff3ae5778d0312ef54863885d3a886592f17ad4ac2bb93f8/play
const char *iboard[] = {
  " |6050060003|",
  "1|..........|",
  "3|.....s....|",
  "1|..........|",
  "4|u.x..u...x|",
  "3|m....m....|",
  "2|m....d....|",
  "1|d.........|",
  "1|..........|",
  "2|.....u....|",
  "2|.....d....|",
  0,
  // " |1251501032|",
  // "3|..........|",
  // "3|..u.......|",
  // "2|..........|",
  // "1|..........|",
  // "1|.w........|",
  // "3|.........u|",
  // "1|..........|",
  // "5|..........|",
  // "1|..........|",
  // "0|..........|",
  // 0,
  // " |3322121051|",
  // "2|..........|",
  // "4|..........|",
  // "0|..........|",
  // "2|w.........|",
  // "2|..........|",
  // "2|..........|",
  // "3|.....m....|",
  // "2|.u........|",
  // "1|..........|",
  // "2|..........|",
  // 0,
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

// for debugging
void putboard() {
  for (int r = 0; board[r]; ++r) {
    fprintf(stderr, "%s\n", board[r]);
  }
}

void makewater(int r, int c) {
  if (r < MIN_R || c < MIN_C || !board[r] || board[r][c] == '|')
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
  assert(w == MAX_C + 2);
  int colcount[w];
  for (int c = MIN_C;c < w;++c) {
    colcount[c] = 0;
    for (int r = MIN_R;r <= MAX_R;++r) {
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
const constexpr int BOARDSIZE = MAX_R + 1 - MIN_R;
struct boat {
  int length;
  int row, col; // the r and c of lower left
  enum ORIENT { VERT, HORIZ, SUB } orient;
  char orient_name() {
    switch (orient) {
    case VERT:
      return 'v';
      break;
    case HORIZ:
      return 'h';
      break;
    case SUB:
      return 's';
      break;
    default:
      return 'x';
    }
  }
  enum SOLVED { YES, NO };
  enum PLACEMENT { PLACE, UNPLACE };
  
  boat(int len) {
    length = len;
    init_pos();
  }

  void init_pos() {
    col = MIN_C;
    row = MIN_R;
    if (length > 1) {
      orient = VERT;
    } else {
      orient = SUB;
    }
  }

  boat* next_boat() {
    boat* rv = this + 1;
    return rv;
  }
  
  void putboat(FILE* f) {
    fprintf(f, "boat l=%d r=%d c=%d %c\n",
            length, row, col, orient_name());
  }

  bool incr() {
    if (orient == VERT) {
      orient = HORIZ;
      return true;
    }
    if (orient != SUB) {
      orient = VERT;
    }
    if (col < MAX_C) {
      ++col;
      return true;
    }
    col = MIN_C;
    if (row < MAX_R) {
      ++row;
      return true;
    }
    return false;
  }

  SOLVED solve(int start_x, int start_y);
  bool it_fits();
  void place(PLACEMENT placement);
};

bool is_watery(int row, int col) {
  if (row < MIN_R || row > MAX_R ||
      col < MIN_C || col > MAX_C) {
    return true;
  }
  switch(board[row][col]) {
  case '.':
  case 'w':
    return true;
  default:
    return false;
  }
}

bool could_match(char b, char board) {
  if (board == b) {
    return true;
  }
  switch(board) {
  case '.':
    return true;
    break;
  case 'w':
    return false;
    break;
  case 'x':
    return true;
    break;
  }
  return false;
}

int row_residue(int row) {
  int sum = board[row][0] - '0';
  int inrow = 0;
  for (int i = MIN_C; i <= MAX_C; ++i) {
    if (!is_watery(row,i))
      ++inrow;
  }
  // fprintf(stderr, "row %d residue %d\n", row, sum - inrow);
  return sum - inrow;
}

int col_residue(int col) {
  int sum = board[0][col] - '0';
  int incol = 0;
  for (int i = MIN_R; i <= MAX_R; ++i) {
    if (!is_watery(i,col))
      ++incol;
  }
  // fprintf(stderr, "col %d residue %d\n", col, sum - incol);
  return sum - incol;
}

// this does not account for col and row counts
bool boat::it_fits() {
#define RETURN(x) { if (debug) fprintf(stderr, "boat(%d) at %d,%d(%c) %s\n", length, row, col, orient_name(), #x); return x; }
  // #define RETURN(x) { return x; }
  // what should either end of the boat look like
  char zeromatch='s';
  char maxmatch='s';
  char b=0;
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
  // Make sure the boat fits on the board
  switch (orient) {
  case SUB:
    if (col < MIN_C || col > MAX_C
        || row < MIN_R || row > MAX_R)
      RETURN(false);
    b = board[row][col];
    if (b == 's') {
      RETURN(true);
    }
    if (!could_match('s', b))
      RETURN(false);
    for (int ov = -1; ov <= 1; ++ov) {
      if (row + ov > MAX_R || row + ov < MIN_R)
        continue;
      for (int oh = -1; oh <= 1; ++oh) {
        if (col + oh > MAX_C || col + oh < MIN_C)
          continue;
        if (oh == 0 && ov == 0)
          continue;
        if (!is_watery(row+ov,col+oh))
          RETURN(false);
      }
    }
    break;
  case HORIZ:
    if (col < MIN_C || col+length-1 > MAX_C
        || row < MIN_R || row > MAX_R) {
      RETURN(false);
    }
    if (!could_match(zeromatch, board[row][col])) {
      RETURN(false);
    }
    for (int i = 1; i < length; ++i) {
      b = board[row][col+i];
      if (i+1 == length) {
        if (!could_match(maxmatch, b))
          RETURN(false);
      } else if (!could_match('m', b))
        RETURN(false);
    }
    for (int ov = -1; ov <= 1; ++ov) {
      if (row + ov > MAX_R || row + ov < MIN_R)
        continue;
      for (int oh = -1; oh <= length; ++oh) {
        if (ov == 0 && oh >= 0 && oh < length)
          continue;
        if (!is_watery(row+ov,col+oh))
          RETURN(false);
      }
    }
    break;

  case VERT:
    if (col < MIN_C || col > MAX_C
        || row-length+1 < MIN_R || row > MAX_R)
      RETURN(false);
    for (int i = 0; i < length; ++i) {
      b = board[row-i][col];
      if (i == 0) {
        if (!could_match(zeromatch, b))
          RETURN(false);
      } else if (i+1 == length) {
        if (!could_match(maxmatch, b))
          RETURN(false);
      } else {
        if (!could_match('m', b))
          RETURN(false);
      }
    }
    for (int oh = -1; oh <= 1; ++oh) {
      if (col + oh > MAX_C || col + oh < MIN_C)
        continue;
      for (int ov = -1; ov <= length; ++ov) {
        if (oh == 0 && ov >= 0 && ov < length)
          continue;
        if (!is_watery(row-ov,col+oh))
          RETURN(false);
      }
    }
  }
  RETURN(true);
}

void boat::place(PLACEMENT placement) {
  switch (orient) {
  case SUB:
    board[row][col] = 's';
    if (placement == UNPLACE) {
      board[row][col] = iboard[row][col];
    }
    break;
  case VERT:
    board[row][col] = 'd';
    for (int off = 1;off < length-1; ++off) {
      board[row-off][col] = 'm';
    }
    board[row-length+1][col] = 'u';
    if (placement == UNPLACE) {
      for (int off = 0;off < length; ++off) {
        board[row-off][col] = iboard[row-off][col];
      }
    }
    break;
  case HORIZ:
    board[row][col] = 'l';
    for (int off = 1;off < length-1; ++off) {
      board[row][col+off] = 'm';
    }
    board[row][col+length-1] = 'r';
    if (placement == UNPLACE) {
      for (int off = 0;off < length; ++off) {
        board[row][col+off] = iboard[row][col+off];
      }
    }
    break;
  }
}

boat::SOLVED  boat::solve(int start_col, int start_row) {
  init_pos();
  row = start_row;
  col = start_col;
  if (col > MAX_C) {
    col = MIN_C;
    ++row;
    if (row > MAX_R) {
      return NO;
    }
  }
  while (true) {
    if (it_fits()) {
      if (debug) {
        fprintf(stderr, "place boat(%d) r=%d c=%d o=%c\n",
                length, row, col, orient==VERT ? 'v' : 'h');
      }
      place(PLACE);
      if (pb) {
        putboard();
      }
      bool counts_ok = true;
      if (row_residue(row) < 0) {
        if (debug)
          fprintf(stderr, "row count failed for origin\n");
        counts_ok = false;
      }
      if (counts_ok && col_residue(col) < 0) {
        if (debug)
        fprintf(stderr, "col counts failed for origin\n");
        counts_ok = false;
      }
      switch (orient) {
      case VERT:
        for (int i = 1; counts_ok && i < length; ++i) {
          if (row_residue(row - i) < 0) {
            if (debug)
              fprintf(stderr, "counts failed for vert\n");
            counts_ok = false;
          }
        }
        break;
      case HORIZ:
        for (int i = 1; counts_ok && i < length; ++i) {
          if (col_residue(col + i) < 0) {
            if (debug)
              fprintf(stderr, "counts failed for horiz\n");
            counts_ok = false;
          }
        }
        break;
      }
      if (counts_ok) {
        // fprintf(stderr, "counts OK\n");
        auto nb = next_boat();
        if (!nb->length) {
          return YES;
        }
        SOLVED rv;
        if (nb->length == length) {
          rv = nb->solve(col+1, row);
        } else {
          rv = nb->solve(MIN_C, MIN_R);
        }
        if (rv == YES) {
          return rv;
        }
      }
      if (debug) {
        fprintf(stderr, "unplace boat(%d) r=%d c=%d o=%c\n",
                length, row, col, orient_name());
      }
      place(UNPLACE);
    }
    if (!incr()) {
      return NO;
    }
  }
  return NO;
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


static option long_options[] = {
    {"debug",     no_argument, &debug,              1 },
    {"putboard",  no_argument, &pb,                 1 },
    {"solve",     no_argument, &solve,              1 },
    {"water",     no_argument, &add_water,          1 },
    {0,           0,           0,                   0 },
};

int
main(int nargs, char* const* args) {
  int longind;
  while (int rv = getopt_long_only(nargs, args,
                                   "",
                                   long_options,
                                   &longind) >= 0) {
  }
  fputs(ps_preamble, stdout);
  board = (char**)malloc(sizeof(iboard));
  for (int r = 0; r < sizeof(iboard)/sizeof(iboard[0]); ++r) {
    board[r] = iboard[r]?strdup(iboard[r]):0; // make the board writable
  }
  if (add_water)
    fill_in_counts_and_water();
  boat* b = &boats[0];
  if (solve)
    b->solve(MIN_C, MIN_R);
  for (int i = MIN_C; board[0][i] && board[0][i] != '|'; ++i) {
    printf("%d 0 (%c) bigstring\n", i-1, board[0][i]);
  }
  for (int r = MIN_R; board[r]; ++r) {
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
  // for (boat* b = boats; b->length; b = b->next_boat()) {
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
