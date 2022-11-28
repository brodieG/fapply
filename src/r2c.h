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


#ifndef R2C_H
#define R2C_H
#define R_NO_REMAP

// System headers go above
#include <R.h>
#include <Rinternals.h>
#include <Rversion.h>
#include <stdint.h>

// Important indices in the alloc data (0-base)
#define I_STAT      0   // status flags
#define I_RES       1   // final call result
#define I_GRP       2   // index start of group varying data

// Indices in the I_STAT element of the alloc_data (only 1 so far)
#define STAT_N       1  // STATUS entry count (not an index)
#define STAT_RECYCLE 0  // bad recycling

struct const_dat {const char * name; const int value;};

typedef SEXP (*r2c_dl_fun) (
  double ** data, R_xlen_t * lens, int ** di, int * narg, int * flag, SEXP ctrl
);
/*
 * Structure containing the varying data in a format for faster access
 */
struct R2C_dat {
  double ** data;  // Full data (see next for details)
  // `data` contains some meta data columns first, result vector, followed by
  // the iterations varying data, followed by "static" data (same for every
  // call), see I_* above for precise indices.
  int dat_start;   // First "iteration varying" data column
  int dat_end;     // Last "iteration varying" data column
  int dat_count;   // dat_end - dat_start + 1 (convenience)
  int ** datai;    // For each sub-fun, which indices in data are relevant
  int * narg;      // For each sub-fun, how many arguments it takes
  int * flags;     // Flag (T/F) control parameters, one for each sub-fun
  SEXP ctrl;       // Non data, non-flag parameters
  R_xlen_t * lens; // Length of each of the data vectors
  r2c_dl_fun fun;  // function to apply
};

SEXP R2C_assumptions(void);
SEXP R2C_constants(void);
SEXP R2C_group_sizes(SEXP g);

struct R2C_dat prep_data(
  SEXP dat, SEXP dat_cols, SEXP ids, SEXP flag, SEXP ctrl, SEXP so
);

SEXP R2C_run_window(
  SEXP so, SEXP dat, SEXP dat_cols, SEXP ids, SEXP flag,
  SEXP ctrl, SEXP width, SEXP offset, SEXP by, SEXP partial
);
SEXP R2C_run_group(
  SEXP so, SEXP dat, SEXP dat_cols, SEXP ids, SEXP flag,
  SEXP ctrl, SEXP grp_lens, SEXP res_lens
);
SEXP R2C_run_window_i(
  SEXP so, SEXP dat, SEXP dat_cols, SEXP ids, SEXP flag,
  SEXP ctrl, SEXP width, SEXP offset,
  SEXP by_sxp, SEXP index_sxp, SEXP start_sxp, SEXP end_sxp,
  SEXP interval_sxp, SEXP partial
);

#endif  /* R2C_H */
