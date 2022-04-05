/*
 * Copyright (C) 2022  Brodie Gaslam
 *
 * This file is part of "fapply - Fast Apply"
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
#include "fapply.h"
#include <R_ext/Rdynload.h>

// Taken from Rdynpriv.h; we'll need to work around this

typedef struct {
    char       *name;
    DL_FUNC     fun;
    int         numArgs;

    R_NativePrimitiveArgType *types;   
} Rf_DotCSymbol;

typedef Rf_DotCSymbol Rf_DotFortranSymbol;


typedef struct {
    char       *name;
    DL_FUNC     fun;
    int         numArgs;
} Rf_DotCallSymbol;

typedef Rf_DotCallSymbol Rf_DotExternalSymbol;

struct Rf_RegisteredNativeSymbol {
    NativeSymbolType type;
    union {
	Rf_DotCSymbol        *c;
	Rf_DotCallSymbol     *call;
	Rf_DotFortranSymbol  *fortran;
	Rf_DotExternalSymbol *external;
    } symbol;
    DllInfo *dll;
};

/*
 * Given an so and function name, retrieve and run the function
 */

SEXP FAPPLY_run(SEXP so, SEXP fun_name, SEXP x) {
  if(TYPEOF(so) != STRSXP || XLENGTH(so) != 1)
    error("Argument `so` should be a scalar string.");
  if(TYPEOF(fun_name) != STRSXP || XLENGTH(fun_name) != 1)
    error("Argument `fun_name` should be a scalar string.");
  if(TYPEOF(x) != REALSXP)
    error("Argument `x` should be a numeric vector.");
  // if(TYPEOF(x) != VECSXP)
  //   error("Argument `x` should be a list.");

  // R_xlen_t xlen = XLENGTH(x);
  // if(xlen > INT_MAX)
  //   error("Arguent `x` may not contain more than INT_MAX items.");
  // double ** xvals = (double**) R_alloc((size_t)xlen, sizeof(double*));
  // for(R_xlen_t i = 0; i < xlen; ++i) {
  //   if(TYPEOF(VECTOR_ELT(x, i)) != REALSXP)
  //     error("Argument `x[%d]` must be a double.", i);
  //   *(xvals + i) = REAL(VECTOR_ELT(x, i));
  // }
  DL_FUNC fun = NULL;
  const char * fun_char = CHAR(STRING_ELT(fun_name, 0));
  const char * dll_char = CHAR(STRING_ELT(so, 0));
  struct Rf_RegisteredNativeSymbol symbol = {0, {NULL}, NULL};
  Rprintf("finding symbol for '%s' '%s'", fun_char, dll_char);
  fun = R_FindSymbol(fun_char, dll_char, &symbol);
  Rprintf("Calling fun");
  return (*fun)(x);
}
