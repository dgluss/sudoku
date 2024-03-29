#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <ostream>
#include <regex.h>
#include <set>
#include <sstream>
#include <stdarg.h>
#include <string.h>
#include <string>
#include <vector>

// TODO: forcingchain (which seems pretty hard)
//  rectangle elimination, simple pairs (uses conjugate pairs)
// https://www.thonky.com/sudoku

//#include <cassert>
using namespace std;
const constexpr int SIZE = 9;

enum { FULL, CELLONLY } hintmode = FULL;
FILE* logfile = fopen("foo", "w"); // nullptr;
void maybe_log(const char* format, const char* cellname, ...) {
  va_list argptr;
  va_start(argptr, cellname);
  if (logfile) {
    if (hintmode == CELLONLY && format[0] != '#') {
      if (*cellname) {
        fprintf(logfile, "%s\n", cellname);
      }
    } else {
      vfprintf(logfile, format, argptr);
      va_end(argptr);
      va_start(argptr, cellname);
    }
  }
  vprintf(format, argptr);
  va_end(argptr);
}

int mask_from_set(const set<int>& the_set) {
  int m = 0;
  for (auto i : the_set) {
    m |= 1<<(i-1);
  }
  return m;
}

// A cell may have a value. The value_set is the set of currently
// known possible values.
struct cell_t {
  int value;
  set<int> value_set;
  int mask;

  void set_mask() {
    mask = mask_from_set(value_set);
  }

  bool can_have(int val) const {
    return value_set.find(val) != value_set.end();
  }
};

struct iboard {
  iboard(const char* n, int d[SIZE][SIZE]) {
    name = n;
    for (int i = 0; i < SIZE; ++i) {
      for (int j = 0; j < SIZE; ++j) {
        data[i][j] = d[i][j];
      }
    }
  }
  explicit iboard(const char* n) {
    name = n;
    for (int i = 0; i < SIZE; ++i) {
      for (int j = 0; j < SIZE; ++j) {
        data[i][j] = 0;
      }
    }
  }
  const char* name;
  int data[SIZE][SIZE];
};

typedef cell_t board_t[SIZE][SIZE];

const char* searchstr = "";
iboard boards[] = {
  {"337",
   (int[SIZE][SIZE])
   {{7,0,8, 2,0,0, 1,0,0},
    {1,0,2, 0,0,7, 6,8,0},
    {0,0,0, 0,0,0, 0,0,9},

    {0,0,0, 4,0,8, 0,6,0},
    {8,0,0, 0,0,0, 0,0,3},
    {0,5,0, 9,0,6, 0,0,0},
               
    {4,0,0, 0,0,0, 0,0,0},
    {0,8,9, 7,0,0, 3,0,6},
    {0,0,7, 0,0,5, 2,0,1}}
  },{"524",
   (int[SIZE][SIZE])
   {{4,1,0, 9,3,5, 0,6,8},
    {3,5,0, 7,0,0, 0,0,0},
    {8,0,0, 0,0,1, 0,0,0},

    {0,0,0, 5,0,2, 0,4,0},
    {5,0,0, 0,0,0, 0,0,9},
    {0,9,0, 6,0,3, 0,0,0},

    {0,0,0, 1,0,0, 0,0,6},
    {0,0,0, 0,0,9, 0,8,7},
    {7,8,0, 2,5,6, 0,9,1}}
  },{"339",
     (int[SIZE][SIZE])
     {{8,0,7, 0,5,0, 0,0,6},
      {0,1,2, 0,0,8, 0,5,0},
      {0,6,0, 2,0,9, 0,0,0},

      {0,0,0, 0,0,3, 0,0,0},
      {4,0,1, 0,0,0, 3,0,8},
      {0,0,0, 5,0,0, 0,0,0},

      {0,0,0, 9,0,7, 0,8,0},
      {0,7,0, 1,0,0, 2,4,0},
      {6,0,0, 0,3,0, 1,0,9}}
  },{"340",
     (int[SIZE][SIZE])
     {{3,0,0, 0,0,7, 0,6,0},
      {7,5,0, 0,4,0, 0,0,0},
      {0,4,0, 0,0,0, 3,0,0},
               
      {0,0,5, 0,2,8, 0,7,0},
      {0,0,9, 0,0,0, 5,0,0},
      {0,3,0, 7,9,0, 1,0,0},

      {0,0,1, 0,0,0, 0,9,0},
      {0,0,0, 0,1,0, 0,2,5},
      {0,9,0, 6,0,0, 0,0,1}}
  },{"341",
     (int[SIZE][SIZE])
     {{6,5,2, 9,0,0, 0,0,0},
      {4,0,9, 0,0,0, 0,2,5},
      {7,0,3, 5,2,0, 0,9,0},

      {9,0,0, 0,6,0, 1,0,2},
      {2,6,0, 1,3,5, 9,8,0},
      {0,0,1, 0,9,2, 0,0,6},

      {3,9,6, 2,4,7, 5,1,8},
      {1,2,0, 6,0,9, 4,7,3},
      {0,0,0, 3,0,1, 2,6,9}}
  },{"342",
     (int[SIZE][SIZE])
     {{5,0,7, 0,4,1, 0,8,0},
      {1,0,9, 8,7,0, 4,0,0},
      {4,0,8, 0,0,3, 0,7,1},

      {0,7,2, 1,0,4, 0,0,5},
      {0,4,5, 3,2,6, 0,0,0},
      {6,1,3, 7,0,0, 2,4,0},

      {7,5,1, 4,0,0, 0,0,3},
      {3,9,6, 0,1,7, 8,0,4},
      {2,8,4, 0,3,0, 0,0,0}}
  },{"525 *****",
     (int[SIZE][SIZE])
     {{9,7,0, 8,0,0, 4,0,0},
      {0,0,6, 0,0,7, 0,2,0},
      {5,0,8, 0,0,0, 0,0,9},

      {0,0,4, 0,8,1, 0,0,0},
      {0,9,0, 0,6,0, 0,3,0},
      {0,0,0, 4,3,0, 5,0,0},

      {4,0,0, 0,0,0, 9,0,1},
      {0,6,0, 1,0,0, 2,0,0},
      {0,0,9, 0,0,8, 0,5,6}}
  },{"526 *****",
     (int[SIZE][SIZE])
     {{1,0,0, 0,0,2, 0,0,0},
      {0,3,6, 7,0,0, 0,0,0},
      {0,0,4, 0,1,0, 5,0,0},

      {4,1,5, 0,0,0, 2,9,7},
      {0,6,0, 5,2,7, 3,1,4},
      {0,2,3, 0,0,0, 6,8,5},

      {0,0,1, 2,6,0, 8,0,0},
      {0,0,0, 0,0,9, 4,2,0},
      {0,0,2, 8,0,0, 0,0,9}}
  },{"601 [*/*]",
     (int[SIZE][SIZE])
     {{0,8,4, 0,0,9, 0,6,5},
      {0,2,0, 3,0,0, 0,8,1},
      {0,5,1, 0,0,7, 0,4,2},

      {4,1,0, 0,0,0, 2,7,0},
      {0,3,0, 0,7,0, 0,1,0},
      {0,7,2, 0,0,0, 0,5,4},

      {2,4,3, 7,0,0, 1,9,6},
      {1,9,7, 0,0,3, 5,2,8},
      {5,6,8, 9,0,0, 4,3,7}}
  },{"343 ****",
     (int[SIZE][SIZE])
     {{0,8,9, 0,0,4, 0,0,0},
      {5,3,0, 7,0,8, 0,0,2},
      {0,4,7, 0,9,0, 0,6,8},

      {0,2,0, 6,7,9, 0,0,5},
      {0,6,0, 4,0,1, 2,3,0},
      {4,0,0, 0,2,3, 0,0,6},

      {0,1,0, 0,4,0, 8,2,0},
      {8,0,0, 1,0,0, 0,7,0},
      {0,0,0, 0,0,0, 6,0,0}}
  },{"527 *****",
     (int[SIZE][SIZE])
     {{0,0,1, 4,3,0, 8,2,0},
      {5,2,6, 9,7,8, 0,1,0},
      {3,0,0, 1,0,2, 0,0,0},

      {0,0,5, 0,8,9, 0,0,0},
      {1,0,0, 0,0,0, 0,0,7},
      {0,0,0, 3,1,0, 2,0,0},

      {0,0,0, 6,0,1, 0,0,9},
      {0,1,0, 0,4,0, 6,0,2},
      {0,0,7, 0,9,3, 0,0,0}}
  },{"528 *****",
     (int[SIZE][SIZE])
     {{0,4,5, 7,8,0, 3,0,2},
      {0,3,0, 0,0,0, 0,5,0},
      {0,2,8, 3,0,0, 0,0,6},

      {4,5,0, 0,0,1, 0,0,0},
      {0,7,0, 0,0,0, 0,3,0},
      {0,0,0, 5,0,0, 0,0,9},

      {5,0,0, 0,0,7, 2,4,0},
      {0,1,4, 0,0,0, 0,6,0},
      {2,0,0, 0,4,8, 0,0,0}}
  },{"345 ****",
     (int[SIZE][SIZE])
     {{5,0,7, 6,0,9, 3,0,2},
      {0,0,9, 0,0,0, 4,0,0},
      {0,8,0, 0,0,0, 0,0,9},

      {7,0,0, 9,5,6, 8,3,1},
      {0,0,5, 2,1,8, 9,7,4},
      {0,9,0, 3,7,4, 6,2,5},

      {2,0,0, 0,9,3, 0,4,0},
      {0,0,3, 0,6,0, 2,9,0},
      {9,0,0, 4,2,7, 1,0,3}}
  },{"155 ***",
     (int[SIZE][SIZE])
     {{4,0,2, 0,0,0, 0,0,0},
      {0,5,8, 4,0,1, 0,9,2},
      {6,1,0, 0,5,2, 0,0,0},

      {0,0,0, 0,0,5, 8,4,0},
      {9,8,5, 0,0,0, 0,0,3},
      {0,4,7, 3,8,0, 9,0,0},

      {0,0,4, 5,1,8, 3,6,9},
      {8,3,1, 0,0,9, 0,7,0},
      {5,0,0, 0,0,0, 1,0,0}}
  },{"347 ****",
     (int[SIZE][SIZE])
     {{2,3,8, 0,0,0, 9,0,1},
      {0,4,0, 8,0,1, 2,0,3},
      {5,0,0, 2,0,3, 8,7,4},

      {0,6,2, 0,3,0, 0,1,8},
      {8,5,0, 0,0,0, 0,3,6},
      {4,0,3, 0,8,6, 5,9,2},

      {0,8,4, 0,0,7, 1,2,9},
      {1,0,0, 4,0,0, 3,8,5},
      {0,0,5, 0,0,8, 6,4,7}}
  },{"529 ****",
     (int[SIZE][SIZE])
     {{3,0,0, 0,2,0, 0,0,0},
      {0,0,8, 0,7,0, 0,0,0},
      {1,7,0, 6,0,0, 4,0,0},

      {4,0,6, 0,0,5, 2,9,0},
      {0,0,0, 0,0,0, 0,0,0},
      {0,5,9, 2,0,0, 1,0,8},

      {0,0,3, 0,0,9, 0,5,4},
      {0,0,0, 0,4,0, 7,0,0},
      {0,0,0, 0,8,0, 0,0,1}}
  },{"184 hard",
     (int[SIZE][SIZE])
     {{3,0,0, 0,1,7, 0,9,0},
      {0,0,4, 3,9,0, 0,0,1},
      {1,0,9, 0,0,0, 0,8,0},

      {0,0,7, 9,0,1, 8,0,0},
      {0,9,0, 0,3,0, 1,2,7},
      {0,0,1, 6,7,0, 9,0,0},

      {7,8,6, 0,0,3, 4,1,9},
      {4,1,2, 7,6,9, 5,3,8},
      {9,5,3, 1,8,4, 0,0,2}}
  },{"185 hard",
     (int[SIZE][SIZE])
     {{0,0,5, 2,0,9, 0,6,0},
      {4,0,0, 0,0,1, 8,0,0},
      {1,0,7, 0,0,0, 0,0,0},

      {0,0,0, 1,0,0, 9,0,5},
      {0,0,0, 0,3,0, 0,0,0},
      {9,0,2, 0,0,5, 0,0,0},

      {0,0,0, 0,0,0, 3,0,1},
      {0,0,1, 6,0,0, 0,0,2},
      {0,3,0, 5,0,2, 7,0,0}}
  },{"191 hard",
     (int[SIZE][SIZE])
     {{8,5,0, 0,6,1, 0,0,4},
      {9,6,4, 2,8,5, 1,7,3},
      {1,3,0, 9,4,0, 5,6,8},

      {5,0,6, 0,1,2, 0,4,0},
      {7,1,0, 6,9,4, 0,0,5},
      {4,2,0, 0,5,0, 6,0,1},

      {3,0,0, 5,0,6, 4,1,0},
      {6,4,5, 1,0,8, 0,0,9},
      {2,7,1, 4,3,9, 8,5,6}}
  },{"191 hard",
     (int[SIZE][SIZE])
     {{8,5,0, 0,6,1, 0,0,4},
      {9,6,4, 2,8,5, 1,7,3},
      {1,3,0, 9,4,0, 5,6,8},

      {5,0,6, 0,1,2, 0,4,0},
      {7,1,0, 6,9,4, 0,0,5},
      {4,2,0, 0,5,0, 6,0,1},

      {3,0,0, 5,0,6, 4,1,0},
      {6,4,5, 1,0,8, 0,0,9},
      {2,7,1, 4,3,9, 8,5,6}}
  },{"185 hard",
     (int[SIZE][SIZE])
     {{0,0,5, 2,0,9, 0,6,0},
      {4,0,0, 0,0,1, 8,0,0},
      {1,0,7, 0,0,0, 0,0,0},

      {0,0,0, 1,0,0, 9,0,5},
      {0,0,0, 0,3,0, 0,0,0},
      {9,0,2, 0,0,5, 0,0,0},
      
      {0,0,0, 0,0,0, 3,0,1},
      {0,0,1, 6,0,0, 0,0,2},
      {0,3,0, 5,0,2, 7,0,0}}
  },{"348 ****",
    (int[SIZE][SIZE]) {
      {0,2,0, 4,1,6, 8,5,3},
      {0,3,0, 5,7,9, 2,0,4},
      {4,0,0, 8,2,3, 0,0,0},

      {0,4,0, 6,9,8, 7,2,1},
      {0,7,0, 2,4,5, 0,3,0},
      {2,9,0, 7,3,1, 0,4,0},

      {0,0,0, 9,8,2, 0,0,6},
      {0,8,2, 3,6,4, 0,9,0},
      {0,6,0, 1,5,7, 0,8,2},
    }
  },{"530 *****",
     (int[SIZE][SIZE]) {
       {9,0,6, 0,0,0, 5,0,0},
       {5,0,0, 3,0,9, 0,1,6},
       {0,3,0, 5,0,6, 0,9,0},
       
       {8,6,3, 9,0,2, 4,5,0},
       {1,7,4, 8,5,3, 9,6,2},
       {2,5,9, 4,6,0, 0,0,8},
       
       {6,9,0, 7,0,5, 0,4,0},
       {0,2,0, 6,9,4, 0,0,5},
       {0,0,5, 0,0,0, 6,0,9},
     }
  },{"93 fiendish",
     (int[SIZE][SIZE])
     {{0,0,0, 0,0,0, 0,0,0},
      {5,0,0, 0,4,2, 0,8,3},
      {0,0,4, 5,0,3, 2,7,0},

      {0,0,5, 3,2,0, 0,1,0},
      {9,0,0, 0,0,0, 0,2,8},
      {0,2,0, 0,0,7, 6,0,0},

      {0,7,9, 8,0,6, 4,0,0},
      {3,6,0, 0,5,0, 0,9,2},
      {4,5,0, 0,0,0, 0,0,0}}
  },{"199 fiendish",
     (int[SIZE][SIZE])
     {
       {0,0,0, 0,0,0, 7,9,0},
       {0,7,0, 0,0,4, 0,0,1},
       {0,0,0, 0,0,9, 6,4,5},

       {0,0,0, 0,4,0, 0,1,8},
       {0,0,4, 5,0,3, 9,0,0},
       {3,6,0, 0,8,0, 0,0,0},

       {7,9,3, 8,0,0, 0,0,0},
       {8,0,0, 3,0,0, 0,2,0},
       {0,2,1, 0,0,0, 0,0,0},
     }
  },{"110 fiendish",
     (int[SIZE][SIZE])
     {
       {6,1,0, 0,2,0, 0,0,0},
       {7,0,8, 3,0,6, 1,5,0},
       {0,0,0, 0,0,0, 0,0,0},

       {0,9,0, 1,0,2, 0,0,0},
       {2,8,0, 0,0,0, 0,1,6},
       {0,0,0, 4,0,7, 0,8,0},

       {0,0,0, 0,0,0, 0,0,0},
       {0,2,1, 5,0,8, 6,0,7},
       {0,0,0, 0,1,0, 0,2,8},
     }
  },{"78 demanding",
     (int[SIZE][SIZE])
     {
       {7,0,0, 1,5,0, 8,3,9},
       {9,0,0, 4,0,0, 5,1,7},
       {1,5,8, 0,9,0, 0,0,2},

       {3,1,0, 0,0,5, 7,2,4},
       {4,0,5, 0,0,0, 0,0,8},
       {0,0,0, 0,0,0, 0,5,1},

       {0,0,0, 2,4,0, 1,0,5},
       {5,0,0, 0,0,0, 0,0,6},
       {0,7,1, 0,0,9, 0,0,3},
     }
  },{"54a fiendish",
     (int[SIZE][SIZE])
     {
       {0,0,0, 2,6,5, 1,4,3},
       {5,1,4, 0,0,3, 7,2,6},
       {2,3,6, 7,4,1, 0,0,0},

       {4,0,0, 3,1,6, 2,7,0},
       {1,7,3, 5,0,2, 0,6,4},
       {0,6,2, 4,7,0, 5,3,1},

       {3,4,9, 1,5,7, 6,8,2},
       {0,0,0, 6,2,4, 3,0,0},
       {6,2,0, 0,3,0, 4,0,7},
     }
  },{"54b fiendish",
     (int[SIZE][SIZE])
     {
       {6,8,2, 0,4,0, 0,0,0},
       {3,0,0, 2,7,8, 0,0,0},
       {4,0,7, 0,0,0, 8,3,2},

       {1,6,4, 7,3,5, 9,2,8},
       {0,3,8, 0,0,2, 0,0,7},
       {7,2,0, 1,8,0, 0,0,0},

       {0,0,1, 0,2,7, 0,0,6},
       {8,7,0, 0,0,0, 2,5,1},
       {2,4,0, 0,0,0, 0,0,0},
     }
  },{"68 fiendish",
     (int[SIZE][SIZE])
     {
       {0,0,9, 0,0,7, 6,0,0},
       {0,0,0, 8,0,0, 2,5,0},
       {0,0,0, 1,5,0, 0,0,4},

       {0,9,8, 0,1,0, 7,0,0},
       {0,6,0, 7,0,8, 0,9,0},
       {0,0,5, 0,9,0, 1,8,0},

       {1,0,0, 0,4,2, 0,6,0},
       {0,3,4, 0,0,6, 0,0,0},
       {0,0,6, 5,0,0, 3,0,0},
     }
  },{"68p fiendish",
     (int[SIZE][SIZE])
     {
       {0,0,9, 0,0,7, 6,0,0},
       {0,0,3, 8,6,0, 2,5,0},
       {6,0,2, 1,5,0, 0,0,4},

       {0,9,8, 0,1,5, 7,0,0},
       {0,6,1, 7,0,8, 4,9,5},
       {0,0,5, 0,9,0, 1,8,0},

       {1,0,7, 3,4,2, 0,6,0},
       {0,3,4, 9,0,6, 0,0,0},
       {9,0,6, 5,0,1, 3,4,0},
     }
  },{"203 fiendish",
     (int[SIZE][SIZE])
     {
       {0,0,6, 0,0,0, 0,9,0},
       {4,0,1, 2,0,0, 5,3,7},
       {5,0,0, 0,0,1, 0,0,8},

       {7,0,0, 0,8,0, 0,1,0},
       {6,1,0, 0,0,0, 0,5,2},
       {0,5,0, 1,4,0, 0,7,6},

       {1,6,7, 3,0,0, 0,0,5},
       {0,4,3, 0,0,7, 0,0,0},
       {0,2,5, 0,1,0, 7,0,3},
     }
  },{"57a fiendish",
     (int[SIZE][SIZE])
     {
       {0,0,7, 0,0,3, 0,0,0},
       {0,4,6, 2,0,0, 0,1,3},
       {0,8,0, 0,5,0, 6,0,9},

       {0,3,0, 1,8,0, 4,0,0},
       {0,1,0, 7,0,6, 0,0,0},
       {8,0,0, 0,0,0, 9,0,0},

       {7,6,0, 0,0,0, 0,0,0},
       {0,0,3, 0,0,0, 0,0,0},
       {0,0,0, 0,0,5, 0,0,0},
     }
  },{"57b fiendish",
     (int[SIZE][SIZE])
     {
       {0,0,0, 7,0,0, 0,0,0},
       {0,0,0, 0,0,0, 3,0,0},
       {0,0,0, 0,0,0, 0,4,2},

       {0,0,4, 0,0,0, 0,0,5},
       {0,0,0, 8,0,9, 0,6,0},
       {0,0,3, 0,2,1, 0,9,0},

       {3,0,8, 0,4,0, 0,1,0},
       {5,2,0, 0,0,8, 6,3,0},
       {0,0,0, 1,0,0, 7,0,0},
     }
  },{"66 fiendish",
     (int[SIZE][SIZE])
     {
       {0,0,0, 0,6,0, 4,0,0},
       {0,0,0, 1,0,7, 0,3,0},
       {9,0,0, 0,0,0, 2,0,6},

       {0,7,0, 0,1,2, 0,0,0},
       {3,0,0, 0,0,0, 0,0,9},
       {0,0,0, 8,4,0, 0,6,0},

       {4,0,1, 0,0,0, 0,0,8},
       {0,5,0, 3,0,4, 0,0,0},
       {0,0,6, 0,5,0, 0,0,0},
     }
  }};
              
//iboard boards[] = {
  // This one (152) was particulary fiendish:
  // {
  //   "152",
  //   {0,0,9, 0,8,0, 0,0,0},
  //   {8,0,0, 1,7,6, 0,0,0},
  //   {1,0,0, 0,0,0, 0,2,0},

  //   {4,0,0, 0,9,0, 1,3,0},
  //   {0,6,0, 0,0,0, 0,5,0},
  //   {0,3,1, 0,4,0, 0,0,6},

  //   {0,4,0, 0,0,0, 0,0,3},
  //   {0,0,0, 8,3,7, 0,0,4},
  //   {0,0,0, 0,6,0, 5,0,0},
  // },
  // This one (521) required a swordfish!
  // {
  // "521",
  //   {0,1,0, 2,0,0, 4,9,0},
  //   {9,5,4, 0,6,0, 0,0,0},
  //   {2,0,0, 9,0,0, 0,0,0},

  //   {7,0,9, 0,1,0, 0,8,4},
  //   {0,0,0, 0,0,0, 0,0,0},
  //   {6,8,0, 0,2,0, 9,0,3},

  //   {0,0,0, 0,0,2, 0,0,8},
  //   {0,0,0, 0,4,0, 3,7,9},
  //   {0,7,1, 0,0,8, 0,5,0},
  // },
  // 334 yikes this is hard: Thonky says it's trivial
  // {
  //   {0,1,0, 0,2,0, 9,0,0},
  //   {0,0,8, 1,0,7, 4,0,5},
  //   {4,0,0, 0,0,0, 0,0,0},

  //   {0,0,0, 0,0,0, 2,0,1},
  //   {8,0,0, 4,6,2, 0,0,7},
  //   {3,0,5, 0,0,0, 0,0,0},

  //   {0,0,0, 0,0,0, 0,0,9},
  //   {1,0,2, 9,0,6, 3,0,0},
  //   {0,0,4, 0,8,0, 0,5,0},
  // },
  // 522
  // {
  //   {6,0,0, 0,8,0, 0,0,0},
  //   {0,9,2, 5,0,0, 0,7,8},
  //   {1,0,0, 0,0,0, 0,2,0},

  //   {7,0,3, 0,0,6, 1,0,0},
  //   {0,0,0, 0,1,0, 0,0,0},
  //   {0,0,6, 9,0,0, 8,0,7},

  //   {0,5,0, 0,0,0, 0,0,1},
  //   {9,6,0, 0,0,3, 4,5,0},
  //   {0,0,0, 0,5,0, 0,0,3},
  // },
  // 335
  // {
  //   {0,4,0, 0,5,6, 1,0,0},
  //   {0,2,0, 0,0,0, 0,9,4},
  //   {0,0,0, 4,0,0, 6,7,0},

  //   {0,0,0, 3,2,0, 0,0,0},
  //   {8,0,0, 0,0,0, 0,0,9},
  //   {0,0,0, 0,1,7, 0,0,0},

  //   {0,3,4, 0,0,2, 0,0,0},
  //   {2,9,0, 0,0,0, 0,4,0},
  //   {0,0,1, 9,6,0, 0,2,0},
  // }
  // 337
  // {
  //   "337",
  //   {7,0,8, 2,0,0, 1,0,0},
  //   {1,0,2, 0,0,7, 6,8,0},
  //   {0,0,0, 0,0,0, 0,0,9},

  //   {0,0,0, 4,0,8, 0,6,0},
  //   {8,0,0, 0,0,0, 0,0,3},
  //   {0,5,0, 9,0,6, 0,0,0},

  //   {4,0,0, 0,0,0, 0,0,0},
  //   {0,8,9, 7,0,0, 3,0,6},
  //   {0,0,7, 0,0,5, 2,0,1},
  // }
  // {
  //   {0,1,0, 0,2,0, 9,0,0},
  //   {0,0,8, 1,0,7, 4,0,5},
  //   {4,0,0, 0,0,0, 0,0,0},

  //   {0,0,0, 0,0,0, 2,0,1},
  //   {8,0,0, 4,6,2, 0,0,7},
  //   {3,0,5, 0,0,0, 0,0,0},

  //   {0,0,0, 0,0,0, 0,0,9},
  //   {1,0,2, 9,0,6, 3,0,0},
  //   {0,0,4, 0,8,0, 0,5,0},
  // },
//};  

struct Board {
  board_t board;
  set<int>& value_set(int r, int c) {
    return board[r][c].value_set;
  }
  iboard altcolor = iboard("color");
  const char* title;
  int maxmoves = -1;
  bool draw2links = false;
  
  void Setup(const iboard& from_board) {
    for (int i = 0 ; i < SIZE; ++i) {
      for (int j = 0 ; j < SIZE; ++j) {
        board[i][j].value = from_board.data[i][j];
      }
    }
    title = from_board.name;
  }    

  void Setup(const Board& from_board) {
    for (int i = 0 ; i < SIZE; ++i) {
      for (int j = 0 ; j < SIZE; ++j) {
        board[i][j].value = from_board.board[i][j].value;
      }
    }
    title = from_board.title;
  }    

  explicit Board(const iboard& from_board) {
    Setup(from_board);
  }

  explicit Board(const Board& from_board) {
    Setup(from_board);
  }
};

void printboard(Board& context, const char* filename);
bool putps = true;
bool interior_possibilities = false;

// max # possibilities to put in the postscript
const int NPOSS = 8;

// sets of r,c in a row, col, or block
struct Collection {
  enum Type {ROW, COL, BLOCK};
  struct ccell {
    int r, c;
    struct Compare {
      bool operator()(const ccell& a, const ccell& b) const {
        if (a.r < b.r) return true;
        if (a.r > b.r) return false;
        if (a.c < b.c) return true;
        return false;
      }
    };
    bool operator==(const ccell& other) const {
      return r == other.r && c == other.c;
    }
    set<int>& value_set(Board& context) const {
      return context.board[r][c].value_set;
    }
    void set_mask(Board& context) const {
      context.board[r][c].set_mask();
    }
    int mask(const Board& context) const {
      return context.board[r][c].mask;
    }
    bool can_have(Board& context, int i) const {
      return value_set(context).find(i) != value_set(context).end();
    }
    // The two ccells share a coordinate (same row or column) or are
    // in the same block.
    bool haslinkto(const ccell& other, bool use_cell_too) const {
      return (r == other.r || c == other.c ||
              (use_cell_too && (r/3 == other.r/3 && c/3 == other.c/3)));
    }
    std::string loc() const {
      std::ostringstream s;
      s << "(" << r+1 << "," << c+1 << ")";
      return s.str();
    }
  };
  std::string name;
  std::set<ccell, ccell::Compare> cells;
  void clear() {
    cells.clear();
  }
  // some ccells in my collection line up with this other ccell
  int link_count(const Collection::ccell other) const {
    int ct = 0;
    for (auto& my : cells) {
      if (my.haslinkto(other, false)) {
        ++ct;
      }
    }
    return ct;
  }
  // some ccells line up
  int link_count(const Collection& other) const {
    int ct = 0;
    for (auto& my : cells) {
      for (auto& they : other.cells) {
        if (my.haslinkto(they, false)) {
          ++ct;
        }
      }
    }
    return ct;
  }
  bool operator==(const Collection& other) {
    return cells == other.cells;
  }
  void Intersect(const Collection& other, Collection* output) const {
    output->clear();
    for (const auto& e : cells) {
      if (other.cells.find(e) != other.cells.end()) {
        output->cells.insert(e);
      }
    }
    output->name = string("(") + name + " intersect " + other.name + ")";
  }

  void Subtract(const Collection& other, Collection* a_b) const {
    a_b->cells.clear();
    for (const auto& e : cells) {
      if (other.cells.find(e) == other.cells.end()) {
        a_b->cells.insert(e);
      }
    }
    a_b->name = string("(") + name + "\\" + other.name + ")";
  }
  std::string locs() const {
    std::ostringstream s;
    for (auto c : cells) {
      s << c.loc();
    }
    return s.str();
  }
};

struct GetCollection {
  enum Type {ROW, COL, BLOCK} type;
  GetCollection(Type type) {
    this->type = type;
  }

  void New(int r, int c, Collection* col) {
    col->cells.clear();
    Add(r, c, col);
  }

  void New(int n, Collection* col) {
    col->cells.clear();
    switch(type) {
    case ROW:
      New(n, 0, col);
      break;
    case COL:
      New(0, n, col);
      break;
    case BLOCK:
      int a = 3*(n%3);
      int b = 3*(n/3);
      New(a, b, col);
      break;
    }
  }

  void GetName(std::string* out) {
    switch(type) {
    case ROW:
      *out = "row";
      return;
    case COL:
      *out = "col";
      return;
    case BLOCK:
      *out = "block";
      return;
    }
  }
  
  void Add(int r, int c, Collection* col) {
    Collection::ccell e;
    switch(type) {
    case ROW:
      col->name = "row ";
      col->name += std::to_string(r+1);
      e.r = r;
      for (int i=0;i<SIZE;++i) {
        e.c = i;
        col->cells.insert(e);
      }
      break;
    case COL:
      col->name = "col ";
      col->name += std::to_string(c+1);
      e.c = c;
      for (int i=0;i<SIZE;++i) {
        e.r = i;
        col->cells.insert(e);
      }
      break;
    case BLOCK:
      col->name = "block ";
      col->name += "UCB"[r/3];
      col->name += "LCR"[c/3];
      int n = 0;
      for (int i = 0; i < 3; ++i) {
        e.r = 3 * (r/3) + i;
        for (int j = 0; j < 3; ++j) {
          e.c = 3 * (c/3) + j;
          col->cells.insert(e);
          ++n;
        }
      }
      break;
    }
  }
};
    
typedef void Setfcn(int, int, Collection*);
typedef void SetSfcn(int, Collection*);

void print_collection(const Collection& c) {
  printf("collection %s: ", c.name.c_str());
  for (const Collection::ccell& cell : c.cells) {
    printf("%s", cell.loc().c_str());
  }
  printf("\n");
}

void print_set(const set<int>& s) {
  for (auto i : s) {
    printf("%d", i);
  }
  printf("\n");
}

// Add a row to a Collection
void rowcollection(int row, int, Collection* c) {
  GetCollection g(GetCollection::ROW);
  g.Add(row, 0, c);
}

void rowScollection(int row, Collection* c) {
  rowcollection(row, 0, c);
}

// Add a column to a Collection
void colcollection(int, int col, Collection* c) {
  c->name = "col ";
  c->name += std::to_string(col+1);
  Collection::ccell e;
  e.c = col;
  for (int i=0;i<SIZE;++i) {
    e.r = i;
    c->cells.insert(e);
  }
}

void colScollection(int col, Collection* c) {
  colcollection(0, col, c);
}

// a in {0-8}, b in {0-8}
void blockcollection(int a, int b, Collection* c) {
  a /= 3;
  b /= 3;
  c->name = "block ";
  c->name += "UCB"[a];
  c->name += "LCR"[b];
  int n = 0;
  Collection::ccell e;
  for (int i = 0; i < 3; ++i) {
    e.r = 3 * a + i;
    for (int j = 0; j < 3; ++j) {
      e.c = 3 * b + j;
      c->cells.insert(e);
      ++n;
    }
  }
}

// This collects all of the cells on the board that have exactly two choices.
void twochoicecollection(Board& context, Collection* c) {
  c->name = "twochoices";
  Collection::ccell e;
  for (int r = 0; r < SIZE; ++r) {
    for (int col = 0;col < SIZE; ++col) {
      if (context.board[r][col].value_set.size() == 2) {
        e.r = r;
        e.c = col;
        c->cells.insert(e);
      }
    }
  }
}

void blockScollection(int block, Collection* c) {
  blockcollection(3*(block%3), 3*(block/3), c);
}

void collection_to_set(const Board& context,
                       const Collection& c,
                       set<int>* s) {
  for (const auto& ccell : c.cells) {
    if (context.board[ccell.r][ccell.c].value > 0)
      s->insert(context.board[ccell.r][ccell.c].value);
  }
}

void collection_lists_to_set(const Board& context,
                             const Collection& c, set<int>* s) {
  for (const auto& ccell : c.cells) {
    const set<int>& value_set = context.board[ccell.r][ccell.c].value_set;
    s->insert(value_set.begin(), value_set.end());
  }
}

void allsubsets(const set<int>& value_set, vector<set<int>>* subsets) {
  subsets->clear();
  for (auto i : value_set) {
    vector<set<int>> newsubsets;
    for (const auto& p : *subsets) {
      set<int> newsub = p;
      newsub.insert(i);
      newsubsets.push_back(newsub);
    }
    for (const auto& s : newsubsets) {
      subsets->push_back(s);
    }
    set<int> s;
    s.insert(i);
    subsets->push_back(s);
  }
}

// apply a function to a row and column to produce a set.
void arbset(const Board& context,
            int row, int column,
            Setfcn fn, set<int>* s) {
  Collection c;
  fn(row, column, &c);
  collection_to_set(context, c, s);
}

void rowset(const Board& context, int n, set<int>* s) {
  arbset(context, n, 0, rowcollection, s);
}

void colset(const Board& context, int n, set<int>* s) {
  arbset(context, 0, n, colcollection, s);
}

// a in {0-8}, b in {0-8}
void blockset(const Board& context, int a, int b, set<int>* s) {
  arbset(context, a, b, blockcollection, s);
}

// How many entries would work in this cell?
int possibilities(const Board& context, int r, int c, set<int>* result) {
  set<int> s;
  auto v = context.board[r][c].value;
  if (v != 0) {
    result->insert(v);
    return 1;
  }
  arbset(context, r, c, rowcollection, &s);
  arbset(context, r, c, colcollection, &s);
  arbset(context, r, c, blockcollection, &s);
  int cand;
  int n = 0;
  for (cand=1;cand<10;++cand) {
    if (s.find(cand) == s.end()) {
      if (result)
	result->insert(cand);
      ++n;
    }
  }
  return n;
}

void set_lists(Board& context) {
  for (int r = 0;r < SIZE;++r) {
    for (int c = 0;c < SIZE; ++c) {
      context.board[r][c].value_set.clear();
      possibilities(context, r, c, &context.board[r][c].value_set);
      context.board[r][c].set_mask();
    }
  }
}

void find_sets(int r, int c, Setfcn fcn) {
  Collection col;
  fcn(r, c, &col);
}

// remove any cell in col that can't have val in it.
void remove_from_collection_not(const Board& context, int val, Collection* col) {
  set<Collection::ccell, Collection::ccell::Compare> badcells;
  for (auto& cell : col->cells) {
    if (!context.board[cell.r][cell.c].can_have(val)) {
      badcells.insert(cell);  // You can't do col->cells.erase(cell) because
      // then you crash the loop. So save the cells to delete for later.
    }
  }
  for (auto& cell : badcells) {
    col->cells.erase(cell);
  }
}

// There's only one possible candidate for this cell
// because all other candidates are in the same row, column,
// or block.
int eliminate(const Board& context, int r, int c) {
  set<int> s;
  if (possibilities(context, r, c, &s) == 1) {
    auto first = s.begin();
    return *first;
  }
  if (s.empty()) {
    return -1;
  }
  return 0;
}

// The candidate is in this row..
bool rowhas(const Board& context, int cand, int row) {
  for (int col = 0; col < SIZE; ++col) {
    if (context.board[row][col].value == cand) return true;
  }
  return false;
}

// The candidate is in this col.
bool colhas(const Board& context, int cand, int col) {
  for (int row = 0; row < SIZE; ++row) {
    if (context.board[row][col].value == cand) return true;
  }
  return false;
}

bool alldone(const Board& context) {
  for (int r = 0; r < SIZE; ++r) {
    for (int c = 0;c < SIZE; ++c) {
      if (!context.board[r][c].value)
        return false;
    }
  }
  return true;
}

// Side effect: sets the value for cells that have list size 1
// after removing val.
bool mayberemove(Board& context, const string& name,
                 const Collection::ccell& thecell, int val) {
  if (!thecell.can_have(context, val)) {
    return false;
  }
  if (context.maxmoves == 0) return false;
  else if (context.maxmoves > 0) --context.maxmoves;
  maybe_log("erase %d in %s by %s\n", thecell.loc().c_str(),
            val, thecell.loc().c_str(), name.c_str());
  thecell.value_set(context).erase(val);
  if (thecell.value_set(context).size() == 1) {
    context.board[thecell.r][thecell.c].value = *thecell.value_set(context).begin();
    maybe_log("found %d in %s because it's all that's left\n",
              thecell.loc().c_str(),
              context.board[thecell.r][thecell.c].value,
              thecell.loc().c_str());
  }
  thecell.set_mask(context);
  return true;
}

// If there are, e.g. 2 cells that have identical size 2 lists,
// then no other cells in the collection can have those values.
// This is actually true of subsets too, so 123, 12, 12 must work:
// the latter two are 1 and 2 so the first one must be 3, thus all other
// lists couldn't have 1 or 2 or 3.
// Side effect: sets the value for cells that now have list size 1.
bool trynakedcells(Board& context, GetCollection& g) {
  bool retval = false;
  Collection col;
  for (int r = 0; r < SIZE; ++r) {
    g.New(r, &col);
    // print_collection(col);
    for (const auto& firstcell : col.cells) {
      const std::set<int> firstlist = firstcell.value_set(context);
      std::set<Collection::ccell, Collection::ccell::Compare> mycells;
      for (auto& subcell : col.cells) {
        if (std::includes(firstlist.begin(), firstlist.end(),
                          subcell.value_set(context).begin(),
                          subcell.value_set(context).end())) {
          mycells.insert(subcell);
        }
      }
      if (mycells.size() == firstlist.size()) {
        if (firstlist.size() == 1)
          context.board[firstcell.r][firstcell.c].value = *firstlist.begin();
        for (auto& thecell : col.cells) {
          if (mycells.find(thecell) == mycells.end()) {
            for (int i : firstcell.value_set(context)) {
              std::ostringstream s;
              s << "nakedcells from " << firstcell.loc() << ":";
              for (auto c : mycells) {
                s << c.loc();
              }
              retval |=
                mayberemove(context, s.str().c_str(), thecell, i);
              if (retval) return retval;
            }
          }
        }
      }
    }
    col.clear();
  }
  return retval;
}

bool tryallnakedcells(Board& context) {
  GetCollection gr(GetCollection::ROW);
  bool r = trynakedcells(context, gr);
  GetCollection gc(GetCollection::COL);
  r |= trynakedcells(context, gc);
  GetCollection gb(GetCollection::BLOCK);
  r |= trynakedcells(context, gb);
  printf("tryallnakedcells returns %d\n", r);
  return r;
}

// If there are, e.g. 2 cells that are the only cells that can have
// 2 different values, then there can't be any other values in those cells.
bool tryhiddencells(Board& context, GetCollection& g) {
  bool retval = false;
  Collection col;
  // A set of values must be a subset of one of the 9 cells, so we
  // don't need to try all subsets of 9 values (512 things to try).
  // No, this was wrong: e.g. {1,2}, {1,3}, {2,3}. Instead we should look
  // at all subsets of the union of values from size>1 sets.
  for (int r = 0; r < SIZE; ++r) {
    g.New(r, &col);
    set<int> triedmasks;
    set<int> interesting_values;
    for (const auto& cell1 : col.cells) {
      vector<set<int>> subsets;
      allsubsets(cell1.value_set(context), &subsets);
      for (const auto& s : subsets) {
        if (s.size() < 2) {
          continue;
        }
        int mask = mask_from_set(s);
        if (triedmasks.find(mask) != triedmasks.end()) {
          continue;
        }
        triedmasks.insert(mask);
        int cell_ct = 0;
        for (const auto& cell : col.cells) {
          // If the mask of this cell includes some of our mask,
          // then count it.
          if ((cell.mask(context) & mask) != 0)
            ++cell_ct;
        }
        if (cell_ct == s.size()) {
          // Now, the question is if we can trim any of these cell's lists.
          for (const auto& cell : col.cells) {
            if ((cell.mask(context) & mask) == 0)
              continue;
            // If the mask of this cell extends beyond our mask,
            // then remove those parts.
            if ((cell.mask(context) & !mask) != 0) {
              set_lists(context);
              printboard(context, "foobar.ps");
              printf("AHA! Hiddencells!\n");
              exit(1);
            }
          }
        }
      }
    }
  }
  return retval;
}

bool tryallhiddencells(Board& context) {
  GetCollection gr(GetCollection::ROW);
  bool r = tryhiddencells(context, gr);
  GetCollection gc(GetCollection::COL);
  r |= tryhiddencells(context, gc);
  GetCollection gb(GetCollection::BLOCK);
  r |= tryhiddencells(context, gb);
  printf("tryallhiddencells returns %d\n", r);
  return r;
}

// The rowfn could be a col also.
bool trypointing(Board& context,
                 int iter,
                 const char* name,
                 GetCollection& blockfn,
                 GetCollection& rowfn) {
  bool retval = false;
  Collection block;
  Collection row;
  Collection rest_of_block;
  Collection rest_of_row;
  for (int b = 0; b < SIZE; ++b) {
    blockfn.New(b, &block);
    string blockname(name);
    blockname += " from " + block.name;
    for (int r = 0; r < SIZE; ++r) {
      row.clear();
      rowfn.New(r, &row);
      block.Subtract(row, &rest_of_block);
      if (rest_of_block == block) continue;
      row.Subtract(block, &rest_of_row);
      set<int> values;
      collection_lists_to_set(context, rest_of_block, &values);
      for (int candidate = 1; candidate < 10; ++candidate) {
        if (values.find(candidate) == values.end()) {
          for (const auto& c : rest_of_row.cells) {
            retval |= mayberemove(context, blockname.c_str(), c, candidate);
          }
        }
      }
    }
  }
  return retval;
}

bool tryallpointing(Board& context) {
  static int iter = 0;
  // printf("tryallpointing %d\n", ++iter);
  GetCollection row(GetCollection::ROW);
  GetCollection col(GetCollection::COL);
  GetCollection block(GetCollection::BLOCK);
  bool r = trypointing(context, iter, "row pointing", block, row);
  r |= trypointing(context, iter, "col pointing", block, col);
  r |= trypointing(context, iter, "row block pointing", row, block);
  r |= trypointing(context, iter, "col block pointing", col, block);
  printf("tryallpointing returns %d\n", r);
  return r;
}

// of course columns and rows can be interchanged.
bool tryxwing(Board& context,
              const char* name, SetSfcn rowfn, SetSfcn colfn) {
  Collection leftcol, rightcol, toprow, botrow;
  bool retval = false;
  for (int cl = 0; cl < SIZE; ++cl) {
    leftcol.clear();
    colfn(cl, &leftcol);
    for (int cr = cl+1; cr < SIZE; ++cr) {
      rightcol.clear();
      colfn(cr, &rightcol);
      for (int rt = 0; rt < SIZE; ++rt) {
        toprow.clear();
        rowfn(rt, &toprow);
        for (int rb = rt+1; rb < SIZE; ++rb) {
          botrow.clear();
          rowfn(rb, &botrow);
          ostringstream descr;
          descr << "xwing("
                << toprow.name << ","
                << botrow.name << ","
                << leftcol.name << ","
                << rightcol.name << ")";
          for (int candidate = 1; candidate < 10; ++candidate) {
	    for (auto cell : toprow.cells) {
	      if (cell.c != cl && cell.c != cr) {
		if (context.board[cell.r][cell.c].can_have(candidate))
		  goto next_candidate;
	      }
	    }
	    for (auto cell : botrow.cells) {
	      if (cell.c != cl && cell.c != cr) {
		if (context.board[cell.r][cell.c].can_have(candidate))
		  goto next_candidate;
	      }
	    }
	    for (auto cell : leftcol.cells) {
	      if (cell.r != rt && cell.r != rb) {
                retval |= mayberemove(context, descr.str(), cell, candidate);
	      }
	    }
	    for (auto cell : rightcol.cells) {
	      if (cell.r != rt && cell.r != rb) {
                retval |= mayberemove(context, descr.str(), cell, candidate);
	      }
	    }
	  next_candidate:;
          }
        }
      }
    }
  }
  return retval;
}

bool tryallxwing(Board& context) {
  bool r = tryxwing(context, "xwing cols", colScollection, rowScollection);
  r |= tryxwing(context, "xwing rows", rowScollection, colScollection);
  printf("tryallxwing returns %d\n", r);
  return r;
}

// All cells in "twocells" have length-2 lists and so are suitable for
// the pivot and pincer cells.
bool tryywing(Board& context, const Collection& twocells) {
  bool retval = false;
  for (auto& pivot : twocells.cells) {
    const auto& vpivot = pivot.value_set(context);
    for (auto& pincer1 : twocells.cells) {
      if (pincer1 == pivot) continue;
      if (!pivot.haslinkto(pincer1, true)) continue;
      const auto& vpincer1 = pincer1.value_set(context);
      set<int> intersect1;
      set_intersection(vpivot.begin(), vpivot.end(),
                       vpincer1.begin(), vpincer1.end(),
                       std::inserter(intersect1, intersect1.begin()));
      if (intersect1.size() != 1) continue;
      for (auto& pincer2 : twocells.cells) {
        if (pincer2 == pivot) continue;
        if (pincer2 == pincer1) continue;
        if (!pivot.haslinkto(pincer2, true)) continue;
        const auto& vpincer2 = pincer2.value_set(context);
        set<int> intersect2;
        set_intersection(vpivot.begin(), vpivot.end(),
                       vpincer2.begin(), vpincer2.end(),
                       std::inserter(intersect2, intersect2.begin()));
        if (intersect2.size() != 1) continue;
        if (intersect2 == intersect1) continue;
        set<int> allvals= vpincer1;
        allvals.insert(vpincer2.begin(),vpincer2.end());
        if (allvals.size() != 3) continue;
        // printf("pivot (%d,%d) pincer1 (%d,%d) pincer2 (%d,%d)\n",
        //        pivot.r+1, pivot.c+1, pincer1.r+1, pincer1.c+1,
        //        pincer2.r+1, pincer2.c+1);
        for (auto v : vpivot) {
          allvals.erase(v);
        }
        int bad = *allvals.begin();
        for (int r = 0; r < SIZE; ++r) {
          for (int c = 0; c < SIZE; ++c) {
            Collection::ccell ccell;
            ccell.r = r;
            ccell.c = c;
            if (ccell == pivot || ccell == pincer1 || ccell == pincer2)
              continue;
            if (ccell.haslinkto(pincer1, true)
                && ccell.haslinkto(pincer2, true)) {
              std::ostringstream s;
              s << "ywing(pivot=" << pivot.r+1 << "," << pivot.c+1
                << ", pincer1=" << pincer1.r+1 << "," << pincer1.c+1
                << ", pincer2=" << pincer2.r+1 << "," << pincer2.c+1
                << ")";
              if (mayberemove(context, s.str(), ccell, bad)) {
                retval = true;
              }
            }
          }
        }
      }
    }
  }
  printf("tryywing returns %s\n", (retval ? "true" : "false"));
  return retval;
}

bool tryallywing(Board& context) {
  Collection twocells;
  twochoicecollection(context, &twocells);
  bool rv = tryywing(context, twocells);
  printf("tryallywing returns %d\n", rv);
  return rv;
}

bool tryswordfish(Board& context, 
                  const char* name,
                  GetCollection& rowf,
                  GetCollection& colf,
                  int candidate) {
  map<int, Collection> rows;
  // get all the rows that can only have the candidate exactly twice.
  int current_row = 0;
  for (int i = 0; i < SIZE; ++i) {
    Collection row;
    rowf.New(i, &row);
    remove_from_collection_not(context, candidate, &row);
    if (row.cells.size() == 2) {
      rows.insert(pair<int, Collection>(current_row++, row));
    }
  }
  // Make sure left and right are connected, in this case by making sure
  // there are two connections for each row. Each row has two cells, and
  // we need a connection on both.
 prune_rows:
  for (const auto& maybe_remove_me : rows) {
    int connections[2] = {0, 0};
    int ct = 0;
    for (const auto& connecting_cell : maybe_remove_me.second.cells) {
      for (const auto& t : rows) {
        if (maybe_remove_me.first == t.first) continue;
        connections[ct] += t.second.link_count(connecting_cell);
      }
      ++ct;
    }
    if (connections[0] < 1 || connections[1] < 1) {
      rows.erase(maybe_remove_me.first);
      goto prune_rows;
    }
  }
  bool retval = false;
  if (rows.size() > 0) {
    std::ostringstream s;
    std::string rowname, colname;
    rowf.GetName(&rowname);
    colf.GetName(&colname);
    s << "swordfish(" << candidate << ","
      << rowname << "," << colname << ") from {";
    for (const auto& r : rows) {
      s << "(";
      for (const auto& cell : r.second.cells) {
        s << cell.loc();
      }
      s << ")";
    }
    s << "}";
    for (const auto& r : rows) {
      for (const auto& c : r.second.cells) {
        Collection column;
        colf.New(c.r, c.c, &column);
        // Don't erase the original swordfish cells.
        for (const auto& therow : rows) {
          for (const auto& cell : therow.second.cells) {
            column.cells.erase(cell);
          }
        }
        for (auto& e : column.cells) {
          retval |= mayberemove(context, s.str().c_str(), e, candidate);
        }
      }
    }
  }
  return retval;
}

bool tryallswordfish(Board& context) {
  bool r = false;
  GetCollection gr(GetCollection::ROW);
  GetCollection gc(GetCollection::COL);
  for (int candidate = 0; candidate < SIZE; ++candidate) {
    r |= tryswordfish(context, "swordfish by row",
                      gr, gc, candidate);
    r |= tryswordfish(context, "swordfish by col",
                      gc, gr, candidate);
  }
  return r;
}

bool tryallcoloring(int candidate) {
  bool r = false;
  GetCollection gr(GetCollection::ROW);
  GetCollection gc(GetCollection::COL);
  GetCollection gb(GetCollection::BLOCK);
  return false;
}

bool tryforcingchain() {return false;}

// The candidate is in this block. a and b are in {0-8}
bool blockhas(const Board& context, int cand, int a, int b) {
  a /= 3;
  b /= 3;
  for (int row = 0; row < 3; ++row) {
    for (int col = 0; col < 3; ++col) {
      if (context.board[3*a + row][3*b + col].value == cand) return true;
    }
  }
  return false;
}

const char postscript_preamble[] =
R"(%!PS-Adobe
/Times-Roman findfont 8 scalefont setfont
54 160 translate
/linespace 56 def
/numlines 9 def
/linelen { linespace numlines mul } def
% calculate lower left coords of x,y
/to_coord {
 numlines sub neg linespace mul
 exch
 1 sub linespace mul
 exch
} def
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
 /Times-Roman findfont 14 scalefont setfont
 3 1 roll % string x y
 to_coord
 moveto % string
 linespace .1 mul linespace .7 mul rmoveto
 show
} def
% digit is printed in a position in a 3x3 array in the cell
% dig is an int, not a string.
/digit { % col row dig
 3 1 roll to_coord
 3 -1 roll % cellcol cellrow dig
 dup 2 string cvs % cellcol cellrow dig (dig)
 4 1 roll 1 sub dup % (dig) cellcol cellrow dig* dig*
 3 mod linespace mul 4 div linespace 8 div add exch % (dig) cellcol cellrow xspace dig
 3 idiv 2 sub neg linespace mul 4 div linespace 8 div add % (dig) cellcol cellrow xspace yspace
 3 -1 roll add % (dig) cellcol xspace yloc
 3 1 roll add exch % (dig) xloc yloc
 gsave
 /Times-Roman findfont 16 scalefont setfont
 .5 setgray
 moveto show
 grestore
} def
/hline { 0 exch moveto linelen 0 rlineto stroke  } def
/vline { 0 moveto 0 linelen rlineto stroke } def
.5 setlinewidth
0 linespace linelen { hline } for
0 linespace linelen { vline } for
% heavier lines every N lines, or 0 for none:
/heavyspace 3 def
heavyspace 0 gt
{ 2 setlinewidth
  /sp heavyspace linespace mul def
  0 sp linelen { hline } for
  0 sp linelen { vline } for
} if
)";

void printboard(Board& context, const char* filename) {
  for (int r = 0;r < SIZE;++r) {
    if (!(r%3))
      fputs("-------------------------------\n", stdout);
    for (int c = 0;c < SIZE; ++c) {
      if (!(c%3)) fputs("|", stdout);
      if (context.board[r][c].value)
	printf(" %d ", context.board[r][c].value);
      else
	printf("   ");
    }
    printf("|\n");
  }
  fputs("-------------------------------\n", stdout);
  if (putps && filename) {
    printf("write file %s\n", filename);
    auto f = fopen(filename, "w");
    fputs(postscript_preamble, f);
    time_t rawtime;
    time(&rawtime);
    struct tm* now = localtime(&rawtime);
    fprintf(f, "1 -0.3 (%d/%d/%d) smallstring\n",
            1900 + now->tm_year,
            1 + now->tm_mon,
            now->tm_mday);
    fprintf(f, "1 0 (%s) smallstring\n", filename);
    if (context.title[0]) {
      fprintf(f, "8 -1 (%s) bigstring\n", context.title);
    }
    for (int r = 0;r < SIZE;++r) {
      for (int c = 0;c < SIZE; ++c) {
	if (context.board[r][c].value) {
	  fprintf(f, "gsave\n");
	  if (context.altcolor.data[r][c])
	    fprintf(f, "0 1 0 setrgbcolor\n");
	  fprintf(f, "%d %d (%d) bigstring\n", c+1, r+1,
                  context.board[r][c].value);
	  fprintf(f, "grestore\n");
	} else {
          if (interior_possibilities) {
            for (auto d : context.board[r][c].value_set) {
              fprintf(f, "%d %d %d digit\n",
                      c+1, r+1, d);
            }            
          } else {
            const auto &mylist = context.board[r][c].value_set;
            if (mylist.size() <= NPOSS) {
              char str[10];
              char* q = str;
              for (auto e : mylist) {
                *q++ = '0' + e;
              }
              *q = 0;
              fprintf(f, "%d %d (%s) smallstring\n",
                      c+1, r+1, str);
            }
          }
        }
      }
    }
    fprintf(f, "showpage\n");
    fclose(f);
  }
}

// This candidate can't be in any other position of this row, col, or block.
string exclude(Board& context, int cand, int r, int c) {
  // the candidate must be OK in this position.
  if (rowhas(context, cand, r)) {
    return "";
  }
  if (colhas(context, cand, c)) {
    return "";
  }
  if (blockhas(context, cand, r, c)){
    return "";
  }
  // the candidate is excluded from any other position of this col.
  bool hit = true;
  for (int row = 0;row < SIZE; ++row) {
    if (row == r) continue;
    if (context.board[row][c].value) continue;
    if (!(rowhas(context, cand, row) || blockhas(context, cand,row,c))) {
      hit = false;
      break;
    }
  }
  if (hit) return "col";
  // the candidate is excluded from any other position of this row.
  hit = true;
  for (int col = 0;col < SIZE; ++col) {
    if (col == c) continue;
    if (context.board[r][col].value) continue;
    if (!(colhas(context, cand, col) || blockhas(context, cand,r,col))) {
      hit = false;
      break;
    }
  }
  if (hit) return "row";
  // the candidate is excluded from any other position of this block.
  for (int blockrow = 0;blockrow < 3; ++blockrow) {
    for (int blockcol = 0;blockcol < 3; ++blockcol) {
      int row = blockrow + 3*(r/3);
      int col = blockcol + 3*(c/3);
      if (row == r && col == c) continue;
      if (context.board[row][col].value) continue;
      if (!(rowhas(context, cand, row) || colhas(context, cand, col))) {
	return "";
      }
    }
  }
  return "block";
}

void msg(int cand, int r, int c, const string& by, const string& what) {
  char loc[20];
  sprintf(loc, "(%d,%d)", r+1, c+1);
  maybe_log("found %d at (%d,%d) by %s(%s)\n", loc, cand, r+1, c+1,
	 by.c_str(), what.c_str());
  //  printboard(0);
}

int makebasicmoves(Board& context) {
  int mademoves = 0;
  bool found = true;
  while (found && (context.maxmoves < 0 || --context.maxmoves >= 0)) {
    found = false;
    for (int r = 0;r < SIZE;++r) {
      for (int c = 0;c < SIZE; ++c) {
	if (context.board[r][c].value > 0) continue;
	if (int e = eliminate(context, r, c)) {
	  if (e < 0) {
	    maybe_log("Problem impossible (%d,%d)\n", "", r+1, c+1);
            maybe_log("%d moves\n", "", mademoves);
            set_lists(context);
            printboard(context, "sudokuboardfailure.ps");
	    exit(1);
	  } else {
	    context.board[r][c].value = e;
	    msg(e, r, c, "elimination", "");
	  }
	  found = true;
          ++mademoves;
	  goto endloop;
	}
      }
    }
    if (found) continue;
    for (int r = 0;r < SIZE;++r) {
      for (int c = 0;c < SIZE; ++c) {
	if (context.board[r][c].value > 0) continue;
	for (int e = 1;e <= 9;++e) {
	  string what = exclude(context, e, r, c);
	  if (!what.empty()) {
	    context.board[r][c].value = e;
	    msg(e, r, c, "exclusion", what);
	    found = true;
            ++mademoves;
	    goto endloop;
	  }
	}
      }
    }
  endloop:;
  }
  printf("---------------------------------- Finished makebasicmoves.\n");
  printf("%d moves\n", mademoves);
  return mademoves;
}

struct Progress {
  const char* name;
  int level;
  bool done = false;
} prog[] = {
  {"basic", 0},
  {"pointing", 1},
  {"nakedcells", 2},
  {"hiddencells", 3},
  {"xwing", 4},
  {"ywing", 5},
  {"swordfish", 6},
  {"final", 7},
};

void print_progress(Board& context, const char* name) {
  Progress* p = nullptr;
  for (auto& q : prog) {
    if (!strcmp(name, q.name)) {
      p = &q;
    }
  }
  if (p == nullptr) {
    printf("print_progress called with illegal \"%s\".\n", name);
    return;
  }
  if (p->done) return;
  p->done = true;
  string oname = "sudokuboard";
  printboard(context, (oname + to_string(p->level) + name + ".ps").c_str());
}

void initboarddata(Board& context, const iboard& b) {
  for (int i = 0 ; i < SIZE; ++i) {
    for (int j = 0 ; j < SIZE; ++j) {
      context.board[i][j].value = b.data[i][j];
      context.altcolor.data[i][j] = 0;
    }
  }
  context.title = b.name;
}

bool solve_board(Board& context) {
  maybe_log("%s\n", "", context.title);
  set_lists(context);
  printboard(context, 0);
  makebasicmoves(context);
  set_lists(context);
  printboard(context, "sudokuboard0basic.ps");
  bool changed = true;
  while (changed && !alldone(context)) {
    changed = false;
    if (makebasicmoves(context) > 0) {
      changed = true;
      continue;
    }
    changed = tryallpointing(context);
    if (changed) {
      print_progress(context, "pointing");
      continue;
    }
    changed = tryallnakedcells(context);
    if (changed) {
      print_progress(context, "nakedcells");
      continue;
    }
    changed = tryallhiddencells(context);
    if (changed) {
      print_progress(context, "hiddencells");
      continue;
    }
    changed = tryallxwing(context);
    if (changed) {
      print_progress(context, "xwing");
      continue;
    }
    changed = tryallswordfish(context);
    if (changed) {
      print_progress(context, "swordfish");
      continue;
    }
    changed = tryallywing(context);
    if (changed) {
      print_progress(context, "ywing");
      continue;
    }
  }
  return alldone(context);
}

int
main(int nargs, const char* args[]) {
  auto& selected_iboard = boards[sizeof(boards)/sizeof(iboard) - 1];
  Board mycontext(selected_iboard);
  while(nargs > 1) {
    if (args[1][1] == 'b') {
      // blank board print
      for (int i = 0 ; i < SIZE; ++i) {
	for (int j = 0 ; j < SIZE; ++j) {
	  mycontext.board[i][j].value = 0;
	}
      }
      printboard(mycontext, "sudokuboard.ps");
      return 0;
    }
    if (args[1][1] == 'p') {
      // Just print the starting board.
      set_lists(mycontext);
      printboard(mycontext, "sudokuboard.ps");
      return 0;
    }
    if (args[1][1] == 'm') {
      // set the number of moves to make (to partially solve).
      if (nargs == 2) {
	printf("the -m flag needs an integer argument, e.g. -m 2\n");
	return(1);
      }
      nargs--;
      args++;
      mycontext.maxmoves = atoi(args[1]);
    }
    if (args[1][1] == 't') {
      if (nargs == 2) {
	printf("the -t (title) flag needs a string argument\n");
	return(1);
      }
      nargs--;
      args++;
      mycontext.title = args[1];
    }
    if (args[1][1] == 's') {
      if (nargs == 2) {
	printf("the -s (search) flag needs a string argument\n");
	return(1);
      }
      nargs--;
      args++;
      searchstr = args[1];
    }
    if (args[1][1] == 'h') {
      // hint mode: don't print the full description of the move.
      hintmode = CELLONLY;
    }
    if (args[1][1] == 'i') {
      // put possibilities inside the square instead of at the top.
      interior_possibilities = true;
    }
    nargs--;
    args++;
  }
  if (searchstr && searchstr[0]) {
    regex_t regbuf;
    regcomp(&regbuf, searchstr, 0);
    for (auto& b : boards) {
      if (regexec(&regbuf, b.name, 0, 0, 0) == 0) {
        mycontext.Setup(b);
        break;
      }
    }
  }
  // putps = false;
  // for (auto& myboard : boards) {
  //   mycontext.Setup(myboard);
  //   if (solve_board(mycontext))
  //     printf("%s worked.\n", mycontext.title);
  //   else
  //     printf("FAILED: %s\n", mycontext.title);
  //   //    printboard(context, board);
  // }
  // exit(0);
  solve_board(mycontext);
  printboard(mycontext, "sudokuboard.ps");
}
