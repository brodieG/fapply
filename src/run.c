/*
 * Copyright (C) 2022  Brodie Gaslam
 *
 * This file is part of "r2c - Fast Iterated Statistic Computation in R"
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 or 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Go to <https://www.r-project.org/Licenses> for a copies of the licenses.
 */

// System headers if any go above ^^
#include "r2c.h"
#include <R_ext/Rdynload.h>

extern struct const_dat consts[];
extern int CONST_N;

// Taken from Rdynpriv.h; we'll need to work around this
// Dummy struct, per C99 all pointers to struct are the same size, and since
// we're just going to pass a NULL pointer which R_FindSymbol checks for this
// will work.  Of course not part of the APIS so this can't be how we do the
// final implementation.

struct Rf_RegisteredNativeSymbol {
    int dummy;
};
/*
 * Structure containing the varying data in a format for faster access
 */
struct R2C_dat {
  double ** data;  // Full data (see next for details)
  // data contains some meta data columns first, result vector, followed by the
  // iterations varying data, followed by "static" data (same for every call)
  int dat_start;   // First "iteration varying" data column
  int dat_end;     // Last "iteration varying" data column
  int dat_count;   // Number "iteration varying" data columns
  int ** datai;    // For each sub-fun, which indices in data are relevant
  int * narg;      // For each sub-fun, how many arguments it takes
  int * flags;     // Flag (T/F) control parameters, one for each sub-fun
  SEXP ctrl;       // Non data, non-flag parameters
  R_xlen_t * lens; // Length of each of the data vectors
  DL_FUNC fun;     // function to apply
};
/*
 * Common Data Restructure Steps
 *
 * Shared by group and window functions.  Uses a small amount of R_alloc memory.
 */
static struct R2C_dat prep_data(
  SEXP dat, SEXP dat_cols, SEXP ids, SEXP flag, SEXP ctrl, SEXP so
) {
  if(TYPEOF(so) != STRSXP || XLENGTH(so) != 1)
    Rf_error("Argument `so` should be a scalar string.");
  if(TYPEOF(dat_cols) != INTSXP && XLENGTH(dat_cols) != 1)
    Rf_error("Argument `dat_cols` should be scalar integer.");
  if(TYPEOF(dat) != VECSXP)
    Rf_error("Argument `data` should be a list.");
  if(TYPEOF(ids) != VECSXP)
    Rf_error("Argument `ids` should be a list.");
  if(TYPEOF(ctrl) != VECSXP)
    Rf_error("Argument `ctrl` should be a list.");
  if(TYPEOF(flag) != INTSXP)
    Rf_error("Argument `flag` should be an integer vector.");
  if(XLENGTH(ids) != XLENGTH(ctrl))
    Rf_error("Argument `ids` and `ctrl` should be the same length.");
  if(XLENGTH(flag) != XLENGTH(ctrl))
    Rf_error("Argument `flag` and `ctrl` should be the same length.");

  const char * fun_char = "run";
  const char * dll_char = CHAR(STRING_ELT(so, 0));
  struct Rf_RegisteredNativeSymbol * symbol = NULL;
  DL_FUNC fun = R_FindSymbol(fun_char, dll_char, symbol);
  int dat_count = Rf_asInteger(dat_cols);

  // Not a foolproof check, but we need at least group varying cols + I_GRP data
  if(dat_count < 0 || dat_count > XLENGTH(dat) - I_GRP)
    Rf_error("Internal Error: bad data col count.");

  // Retructure data to be seakable without overhead of VECTOR_ELT
  // R_alloc not guaranteed to align to pointers, blergh. FIXME.
  double ** data = (double **) R_alloc(XLENGTH(dat), sizeof(double*));
  R_xlen_t * lens = (R_xlen_t *) R_alloc(XLENGTH(dat), sizeof(R_xlen_t));
  for(R_xlen_t i = 0; i < XLENGTH(dat); ++i) {
    SEXP elt = VECTOR_ELT(dat, i);
    if(TYPEOF(elt) != REALSXP)
      Rf_error(
        "Internal Error: non-real data at %jd (%s).\n",
        (intmax_t) i, Rf_type2char(TYPEOF(elt))
      );
    *(data + i) = REAL(elt);
    *(lens + i) = XLENGTH(elt);
  }
  // Indices into data, should be as many as there are calls in the code
  int ** datai = (int **) R_alloc(XLENGTH(ids), sizeof(int*));
  int * narg = (int *) R_alloc(XLENGTH(ids), sizeof(int));
  R_xlen_t call_count = XLENGTH(ids);
  for(R_xlen_t i = 0; i < call_count; ++i) {
    SEXP elt = VECTOR_ELT(ids, i);
    if(TYPEOF(elt) != INTSXP)
      Rf_error(
        "Internal Error: non-integer data at %jd (%s).\n",
        (intmax_t) i, Rf_type2char(TYPEOF(elt))
      );
    // Each call accesses some numbers of elements from data, where the last
    // accessed element receives the result of the call (by convention)
    *(datai + i) = INTEGER(elt);
    *(narg + i) = (int) XLENGTH(elt) - 1;
  }
  struct R2C_dat res = {
    .data = data,
    .datai = datai,
    .dat_start = I_GRP,
    .dat_end = I_GRP + dat_count - 1,
    .dat_count = dat_count,
    .narg = narg,
    .lens = lens,
    .flags = INTEGER(flag),
    .ctrl = ctrl,
    .fun = fun
  };
  return res;
}
