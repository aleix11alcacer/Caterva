/*
 * Copyright (C) 2018  Francesc Alted
 * Copyright (C) 2018  Aleix Alcacer
 */

#ifndef CATERVA_HEADER_FILE
#define CATERVA_HEADER_FILE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <blosc.h>

#define CATERVA_MAXDIM 8

typedef struct {
    size_t shape[CATERVA_MAXDIM];  /* the shape of original data */
    size_t cshape[CATERVA_MAXDIM];  /* the shape of each chunk */
    size_t ndim;  /* data dimensions */
} caterva_pparams;

// The next is useful for initializing pparams structs
static const caterva_pparams CATERVA_PPARAMS_ONES = {
    .shape = {1, 1, 1, 1, 1, 1, 1, 1},
    .cshape = {1, 1, 1, 1, 1, 1, 1, 1},
    .ndim = 1
};

typedef struct {
    blosc2_schunk *sc;
    size_t shape[CATERVA_MAXDIM];  /* shape of original data */
    size_t cshape[CATERVA_MAXDIM];  /* shape of each chunk */
    size_t eshape[CATERVA_MAXDIM];  /* shape of schunk */
    size_t size;  /* size of original data */
    size_t csize;  /* size of each chunnk */
    size_t esize;  /* shape of schunk */
    size_t ndim;  /* data dimensions */
} caterva_array;

caterva_pparams caterva_new_pparams(size_t *shape, size_t *cshape, size_t ndim);

caterva_array *caterva_new_array(blosc2_cparams cp, blosc2_dparams dp, blosc2_frame *fp, caterva_pparams pp);

int caterva_free_array(caterva_array *carr);

int caterva_from_buffer(caterva_array *dest, void *src);

int caterva_to_buffer(caterva_array *src, void *dest);

int caterva_get_slice(caterva_array *src, void *dest, size_t *start, size_t *stop);

int caterva_equal_data(caterva_array *a, caterva_array *b);


#endif
