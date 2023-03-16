#include <math.h>
#include "r2c-const.h"
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Utils.h>
#include "loop-interrupt.h"

static void add(double ** data, R_xlen_t * lens, int * di) {
  int di0 = di[0];
  int di1 = di[1];
  int dires = di[2];
  double * e1 = data[di0];
  double * e2 = data[di1];
  R_xlen_t len1 = lens[di0];
  R_xlen_t len2 = lens[di1];
  double * res = data[dires];

  if(len1 == 0 || len2 == 0) { // empty recycle is zero
    lens[dires] = 0;
    return;
  }
  // Not all "bin" operators are commutative
  // so we cannot play tricks with switching parameter order
  R_xlen_t i, j;
  if(len1 == len2) {
    LOOP_W_INTERRUPT1(len1, {res[i] = (e1[i] + e2[i]);});
    lens[dires] = len1;
  } else if (len2 == 1) {
    LOOP_W_INTERRUPT1(len1, {res[i] = (e1[i] + *e2);});
    lens[dires] = len1;
  } else if (len1 == 1) {
    LOOP_W_INTERRUPT1(len2, {res[i] = (*e1 + e2[i]);});
    lens[dires] = len2;
  } else if (len1 > len2) {
    LOOP_W_INTERRUPT2(len1, len2, res[i] = (e1[i] + e2[j]););
    if(j != len2) data[0][0] = 1.;   // bad recycle
    lens[dires] = len1;
  } else if (len2 > len1) {
    LOOP_W_INTERRUPT2(len2, len1, res[i] = (e1[j] + e2[i]););
    if(j != len1) data[0][0] = 1.;   // bad recycle
    lens[dires] = len2;
  }
}

static void sum(double ** data, R_xlen_t * lens, int * di, int flag) {
  int di0 = di[0];
  int di1 = di[1];

  R_xlen_t len_n = lens[di0];
  double * dat = data[di0];
  int narm = flag;  // only one possible flag parameter

  long double tmp = 0;
  R_xlen_t i;
  if(!narm) LOOP_W_INTERRUPT1(len_n, tmp += dat[i];);
  else LOOP_W_INTERRUPT1(len_n, {if(!ISNAN(dat[i])) tmp += dat[i];});

  *data[di1] = (double) tmp;
  lens[di1] = 1;
}

void run(
  double ** data, R_xlen_t * lens, int ** di, int * narg, int * flag, SEXP ctrl
) {
  (void) narg; // unused
  (void) ctrl; // unused
  
  // a + b
  add(data, lens, *di++);
  ++flag;
  
  // sum(... = a + b, na.rm = FALSE)
  sum(data, lens, *di++, *flag);
  ++flag;
}