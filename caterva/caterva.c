/*
 * Copyright (C) 2018  Francesc Alted
 * Copyright (C) 2018  Aleix Alcacer
 */

#include <caterva.h>
#include "caterva.h"
#include "assert.h"

caterva_ctx_t *caterva_new_ctx(void *(*c_alloc)(size_t), void (*c_free)(void *), blosc2_cparams cparams, blosc2_dparams dparams) {
    caterva_ctx_t *ctx;
    if (c_alloc == NULL) {
        ctx = (caterva_ctx_t *) malloc(sizeof(caterva_ctx_t));
        ctx->alloc = malloc;
    } else {
        ctx = (caterva_ctx_t *) c_alloc(sizeof(caterva_ctx_t));
        ctx->alloc = c_alloc;
    }
    if (c_free == NULL) {
        ctx->free = free;
    } else {
        ctx->free = c_free;
    }
    ctx->cparams = cparams;
    ctx->dparams = dparams;
    return ctx;
}


caterva_dims_t caterva_new_dims(uint64_t *dims, int8_t ndim) {
    caterva_dims_t dims_s = CATERVA_DIMS_DEFAULTS;
    for (int i = 0; i < ndim; ++i) {
        dims_s.dims[i] = dims[i];
    }
    dims_s.ndim = ndim;
    return dims_s;
}

// Serialize the partition params
// TODO: use big-endian to encode ints
int32_t serialize_dims(int8_t ndim, uint64_t* shape, const uint64_t* pshape, uint8_t **sdims) {
    int32_t max_sdims_len = 116;  // 4 + MAX_DIM * (1 + sizeof(uint64_t)) + MAX_DIM * (1 + sizeof(int32_t))
    *sdims = malloc((size_t)max_sdims_len);
    uint8_t *pdims = *sdims;

    // Build an array with 3 entries (ndim, shape, pshape)
    *pdims++ = 0x90 + 3;

    // ndim entry
    *pdims++ = (uint8_t)ndim;  // positive fixnum (7-bit positive integer)
    assert(pdims - *sdims < max_sdims_len);

    // shape entry
    *pdims++ = (uint8_t)(0x90) + (uint8_t)ndim;  // fix array with ndim elements
    for (int8_t i = 0; i < ndim; i++) {
        *pdims++ = 0xcf;  // uint64
        memcpy(pdims, &(shape[i]), sizeof(uint64_t));
        pdims += sizeof(uint64_t);
    }
    assert(pdims - *sdims < max_sdims_len);

    // pshape entry
    *pdims++ = (uint8_t)(0x90) + (uint8_t)ndim;  // fix array with ndim elements
    for (int8_t i = 0; i < ndim; i++) {
        *pdims++ = 0xd2;  // int32
        int32_t pshape_i = (int32_t)pshape[i];
        memcpy(pdims, &pshape_i, sizeof(int32_t));
        pdims += sizeof(int32_t);
    }
    assert(pdims - *sdims <= max_sdims_len);

    int32_t slen = (int32_t)(pdims - *sdims);
    *sdims = realloc(*sdims, (size_t)slen);  // get rid of the excess of bytes allocated

    return slen;
}

// Serialize the partition params
// TODO: decode big-endian ints to native endian
int32_t deserialize_dims(uint8_t *sdims, uint32_t sdims_len, caterva_dims_t *shape, caterva_dims_t *pshape) {
    uint8_t *pdims = sdims;

    // Check that we have an array with 3 entries (ndim, shape, pshape)
    assert(*pdims == 0x90 + 3);
    pdims += 1;
    assert(pdims - sdims < sdims_len);

    // ndim entry
    int8_t ndim = pdims[0];  // positive fixnum (7-bit positive integer)
    assert (ndim < CATERVA_MAXDIM);
    pdims += 1;
    assert(pdims - sdims < sdims_len);
    shape->ndim = ndim;
    pshape->ndim = ndim;

    // shape entry
    // Initialize to ones, as required by Caterva
    for (int i = 0; i < CATERVA_MAXDIM; i++) shape->dims[i] = 1;
    assert(*pdims == (uint8_t)(0x90) + (uint8_t)ndim);  // fix array with ndim elements
    pdims += 1;
    for (int8_t i = 0; i < ndim; i++) {
        assert(*pdims == 0xcf);   // uint64
        pdims += 1;
        memcpy(&(shape->dims[i]), pdims, sizeof(uint64_t));
        pdims += sizeof(uint64_t);
    }
    assert(pdims - sdims < sdims_len);

    // pshape entry
    // Initialize to ones, as required by Caterva
    for (int i = 0; i < CATERVA_MAXDIM; i++) pshape->dims[i] = 1;
    assert(*pdims == (uint8_t)(0x90) + (uint8_t)ndim);  // fix array with ndim elements
    pdims += 1;
    for (int8_t i = 0; i < ndim; i++) {
        assert(*pdims == 0xd2);  // int32
        pdims += 1;
        int32_t pshape_i = (int32_t)pshape->dims[i];
        memcpy(&pshape_i, pdims, sizeof(int32_t));
        pshape->dims[i] = (uint64_t)pshape_i;
        pdims += sizeof(int32_t);
    }
    assert(pdims - sdims <= sdims_len);

    uint32_t slen = (uint32_t)(pdims - sdims);
    assert(slen == sdims_len);

    return 0;
}

caterva_array_t *caterva_empty_array(caterva_ctx_t *ctx, blosc2_frame *fp, caterva_dims_t pshape) {
    /* Create a caterva_array_t buffer */
    caterva_array_t *carr = (caterva_array_t *) ctx->alloc(sizeof(caterva_array_t));

    /* Copy context to caterva_array_t */
    carr->ctx = (caterva_ctx_t *) ctx->alloc(sizeof(caterva_ctx_t));
    memcpy(&carr->ctx[0], &ctx[0], sizeof(caterva_ctx_t));

    /* Create a schunk */
    blosc2_schunk *sc = blosc2_new_schunk(ctx->cparams, ctx->dparams, fp);
    carr->sc = sc;
    carr->size = 1;
    carr->csize = 1;
    carr->esize = 1;
    carr->ndim = pshape.ndim;

    for (unsigned int i = 0; i < CATERVA_MAXDIM; i++) {
        carr->pshape[i] = pshape.dims[i];
        carr->shape[i] = 1;
        carr->eshape[i] = 1;
        carr->csize *= carr->pshape[i];
    }

    blosc2_frame *frame = sc->frame;  // the frame is copied with a new_schunk operation, so use it
    if (frame != NULL) {
        // Serialize the dimension info in the associated frame
        if (frame->nnspaces >= BLOSC2_MAX_NAMESPACES) {
            fprintf(stderr, "the number of namespaces for this frame has been exceeded\n");
            return NULL;
        }
        uint8_t *sdims = NULL;
        int32_t sdims_len = serialize_dims(carr->ndim, carr->shape, carr->pshape, &sdims);
        if (sdims_len < 0) {
            fprintf(stderr, "error during serializing dims info for Caterva");
            return NULL;
        }
        // And store it in caterva namespace
        int retcode = blosc2_frame_add_namespace(frame, "caterva", sdims, (uint32_t)sdims_len);
        if (retcode < 0) {
            return NULL;
        }
    }

    return carr;
}

caterva_array_t *caterva_array_fromfile(caterva_ctx_t *ctx, char* filename) {
    /* Create a caterva_array_t buffer */
    caterva_array_t *carr = (caterva_array_t *) ctx->alloc(sizeof(caterva_array_t));

    /* Copy context to caterva_array_t */
    carr->ctx = (caterva_ctx_t *) ctx->alloc(sizeof(caterva_ctx_t));
    memcpy(&carr->ctx[0], &ctx[0], sizeof(caterva_ctx_t));

    // Open the frame on-disk
    blosc2_frame* frame = blosc2_frame_from_file(filename);

    /* Create a schunk out of the frame */
    blosc2_schunk *sc = blosc2_new_schunk(ctx->cparams, ctx->dparams, frame);
    carr->sc = sc;

    // Deserialize the caterva namespace
    int8_t ndim;
    caterva_dims_t shape;
    caterva_dims_t pshape;
    uint8_t *sdims;
    uint32_t sdims_len;
    blosc2_frame_get_namespace(frame, "caterva", &sdims, &sdims_len);
    deserialize_dims(sdims, sdims_len, &shape, &pshape);
    carr->size = 1;
    carr->csize = 1;
    carr->esize = 1;
    carr->ndim = pshape.ndim;

    for (int i = 0; i < CATERVA_MAXDIM; i++) {
        carr->shape[i] = shape.dims[i];
        carr->size *= shape.dims[i];
        carr->pshape[i] = pshape.dims[i];
        carr->csize *= pshape.dims[i];
        if (shape.dims[i] % pshape.dims[i] == 0) {
            // The case for shape.dims[i] == 1 and pshape.dims[i] == 1 is handled here
            carr->eshape[i] = shape.dims[i];
        } else {
            carr->eshape[i] = shape.dims[i] + pshape.dims[i] - shape.dims[i] % pshape.dims[i];
        }
        carr->size *= carr->shape[i];
        carr->esize *= carr->eshape[i];
    }
    return carr;
}

int caterva_free_ctx(caterva_ctx_t *ctx) {
    ctx->free(ctx);
    return 0;
}

int caterva_free_array(caterva_array_t *carr) {
    blosc2_free_schunk(carr->sc);
    void (*aux_free)(void *) = carr->ctx->free;
    caterva_free_ctx(carr->ctx);
    aux_free(carr);
    return 0;
}

int caterva_update_shape(caterva_array_t *carr, caterva_dims_t shape) {

    if (carr->ndim != shape.ndim) {
        printf("caterva array ndim and shape ndim are not equal\n");
        return -1;
    }
    carr->size = 1;
    carr->esize = 1;
    for (int i = 0; i < CATERVA_MAXDIM; ++i) {
        carr->shape[i] = shape.dims[i];

        if (i < shape.ndim) {
            if (shape.dims[i] % carr->pshape[i] == 0) {
                carr->eshape[i] = shape.dims[i];
            } else {
                carr->eshape[i] = shape.dims[i] + carr->pshape[i] - shape.dims[i] % carr->pshape[i];
            }
        } else {
            carr->eshape[i] = 1;
        }
        carr->size *= carr->shape[i];
        carr->esize *= carr->eshape[i];
    }

    blosc2_frame* fp = carr->sc->frame;
    if (fp != NULL) {
        uint8_t *sdims = NULL;
        // Serialize the dimension info ...
        int32_t sdims_len = serialize_dims(carr->ndim, carr->shape, carr->pshape, &sdims);
        if (sdims_len < 0) {
            fprintf(stderr, "error during serializing dims info for Caterva");
            return -1;
        }
        // ... and update it in its namespace
        int retcode = blosc2_frame_update_namespace(fp, "caterva", sdims, (uint32_t)sdims_len);
        if (retcode < 0) {
            return -1;
        }
    }

    return 0;
}

// Fill a caterva array from a C buffer
// The caterva array must be empty at the begining
int caterva_from_buffer(caterva_array_t *dest, caterva_dims_t shape, void *src) {
    int8_t *s_b = (int8_t *) src;

    if (dest->sc->nbytes > 0) {
        printf("Caterva container must be empty!");
        return -1;
    }

    caterva_update_shape(dest, shape);

    uint64_t d_shape[CATERVA_MAXDIM];
    uint64_t d_pshape[CATERVA_MAXDIM];
    uint64_t d_eshape[CATERVA_MAXDIM];
    int8_t d_ndim = dest->ndim;

    for (int i = 0; i < CATERVA_MAXDIM; ++i) {
        d_shape[(CATERVA_MAXDIM - d_ndim + i) % CATERVA_MAXDIM] = dest->shape[i];
        d_eshape[(CATERVA_MAXDIM - d_ndim + i) % CATERVA_MAXDIM] = dest->eshape[i];
        d_pshape[(CATERVA_MAXDIM - d_ndim + i) % CATERVA_MAXDIM] = dest->pshape[i];
    }

    caterva_ctx_t *ctx = dest->ctx;
    int typesize = dest->sc->typesize;
    int8_t *chunk = malloc(dest->csize * typesize);

    /* Calculate the constants out of the for  */
    uint64_t aux[CATERVA_MAXDIM];
    aux[7] = d_eshape[7] / d_pshape[7];
    for (int i = CATERVA_MAXDIM - 2; i >= 0; i--) {
        aux[i] = d_eshape[i] / d_pshape[i] * aux[i + 1];
    }

    /* Fill each chunk buffer */
    uint64_t desp[CATERVA_MAXDIM], r[CATERVA_MAXDIM];
    for (uint64_t ci = 0; ci < dest->esize / dest->csize; ci++) {
        memset(chunk, 0, dest->csize * typesize);

        /* Calculate the coord. of the chunk first element */
        desp[7] = ci % (d_eshape[7] / d_pshape[7]) * d_pshape[7];
        for (int i = CATERVA_MAXDIM - 2; i >= 0; i--) {
            desp[i] = ci % (aux[i]) / (aux[i + 1]) * d_pshape[i];
        }

        /* Calculate if pad with 0 are needed in this chunk */
        for (int i = CATERVA_MAXDIM - 1; i >= 0; i--) {
            if (desp[i] + d_pshape[i] > d_shape[i]) {
                r[i] = d_shape[i] - desp[i];
            } else {
                r[i] = d_pshape[i];
            }
        }

        /* Copy each line of data from chunk to arr */
        uint64_t s_coord_f, d_coord_f, s_a, d_a;
        uint64_t ii[CATERVA_MAXDIM];
        for (ii[6] = 0; ii[6] < r[6]; ii[6]++) {
            for (ii[5] = 0; ii[5] < r[5]; ii[5]++) {
                for (ii[4] = 0; ii[4] < r[4]; ii[4]++) {
                    for (ii[3] = 0; ii[3] < r[3]; ii[3]++) {
                        for (ii[2] = 0; ii[2] < r[2]; ii[2]++) {
                            for (ii[1] = 0; ii[1] < r[1]; ii[1]++) {
                                for (ii[0] = 0; ii[0] < r[0]; ii[0]++) {
                                    d_coord_f = 0;
                                    d_a = d_pshape[7];

                                    for (int i = CATERVA_MAXDIM - 2; i >= 0; i--) {
                                        d_coord_f += ii[i] * d_a;
                                        d_a *= d_pshape[i];
                                    }

                                    s_coord_f = desp[7];
                                    s_a = d_shape[7];

                                    for (int i = CATERVA_MAXDIM - 2; i >= 0; i--) {
                                        s_coord_f += (desp[i] + ii[i]) * s_a;
                                        s_a *= d_shape[i];
                                    }
                                    memcpy(&chunk[d_coord_f * typesize], &s_b[s_coord_f * typesize],
                                           r[7] * typesize);
                                }
                            }
                        }
                    }
                }
            }
        }
        blosc2_schunk_append_buffer(dest->sc, chunk, dest->csize * typesize);
    }

    ctx->free(chunk);
    return 0;
}

int caterva_fill(caterva_array_t *dest, caterva_dims_t shape, void *value) {

    caterva_update_shape(dest, shape);

    uint8_t *chunk = malloc(dest->csize * dest->sc->typesize);

    for (uint64_t i = 0; i < dest->csize; ++i) {
        if (dest->sc->typesize == 1) {
            chunk[i] = *(uint8_t *) value;
        } else if (dest->sc->typesize == 2) {
            ((uint16_t *) chunk)[i] = *(uint16_t *) value;
        } else if (dest->sc->typesize == 4) {
            ((uint32_t *) chunk)[i] = *(uint32_t *) value;
        } else {
            ((uint64_t *) chunk)[i] = *(uint64_t *) value;
        }
    }

    uint64_t nchunk = dest->esize / dest->csize;

    for (int i = 0; i < nchunk; ++i) {
        blosc2_schunk_append_buffer(dest->sc, chunk, dest->csize * dest->sc->typesize);
    }

    free(chunk);

    return 0;
}

int caterva_to_buffer(caterva_array_t *src, void *dest) {
    int8_t *d_b = (int8_t *) dest;

    uint64_t s_shape[CATERVA_MAXDIM];
    uint64_t s_pshape[CATERVA_MAXDIM];
    uint64_t s_eshape[CATERVA_MAXDIM];
    int8_t s_ndim = src->ndim;

    for (int i = 0; i < CATERVA_MAXDIM; ++i) {
        s_shape[(CATERVA_MAXDIM - s_ndim + i) % CATERVA_MAXDIM] = src->shape[i];
        s_eshape[(CATERVA_MAXDIM - s_ndim + i) % CATERVA_MAXDIM] = src->eshape[i];
        s_pshape[(CATERVA_MAXDIM - s_ndim + i) % CATERVA_MAXDIM] = src->pshape[i];
    }

    /* Initialise a chunk buffer */
    caterva_ctx_t *ctx = src->ctx;
    int typesize = src->sc->typesize;
    int8_t *chunk = (int8_t *) ctx->alloc(src->csize * typesize);

    /* Calculate the constants out of the for  */
    uint64_t aux[CATERVA_MAXDIM];
    aux[7] = s_eshape[7] / s_pshape[7];
    for (int i = CATERVA_MAXDIM - 2; i >= 0; i--) {
        aux[i] = s_eshape[i] / s_pshape[i] * aux[i + 1];
    }

    /* Fill array from schunk (chunk by chunk) */
    uint64_t desp[CATERVA_MAXDIM], r[CATERVA_MAXDIM];
    for (uint64_t ci = 0; ci < src->esize / src->csize; ci++) {
        blosc2_schunk_decompress_chunk(src->sc, (int) ci, chunk, src->csize * typesize);

        /* Calculate the coord. of the chunk first element in arr buffer */
        desp[7] = ci % aux[7] * s_pshape[7];
        for (int i = CATERVA_MAXDIM - 2; i >= 0; i--) {
            desp[i] = ci % (aux[i]) / (aux[i + 1]) * s_pshape[i];
        }

        /* Calculate if pad with 0 are needed in this chunk */
        for (int i = CATERVA_MAXDIM - 1; i >= 0; i--) {
            if (desp[i] + s_pshape[i] > s_shape[i]) {
                r[i] = s_shape[i] - desp[i];
            } else {
                r[i] = s_pshape[i];
            }
        }

        /* Copy each line of data from chunk to arr */
        uint64_t s_coord_f, d_coord_f, s_a, d_a;
        uint64_t ii[CATERVA_MAXDIM];
        for (ii[6] = 0; ii[6] < r[6]; ii[6]++) {
            for (ii[5] = 0; ii[5] < r[5]; ii[5]++) {
                for (ii[4] = 0; ii[4] < r[4]; ii[4]++) {
                    for (ii[3] = 0; ii[3] < r[3]; ii[3]++) {
                        for (ii[2] = 0; ii[2] < r[2]; ii[2]++) {
                            for (ii[1] = 0; ii[1] < r[1]; ii[1]++) {
                                for (ii[0] = 0; ii[0] < r[0]; ii[0]++) {
                                    s_coord_f = 0;
                                    s_a = s_pshape[7];

                                    for (int i = CATERVA_MAXDIM - 2; i >= 0; i--) {
                                        s_coord_f += ii[i] * s_a;
                                        s_a *= s_pshape[i];
                                    }

                                    d_coord_f = desp[7];
                                    d_a = s_shape[7];

                                    for (int i = CATERVA_MAXDIM - 2; i >= 0; i--) {
                                        d_coord_f += (desp[i] + ii[i]) * d_a;
                                        d_a *= s_shape[i];
                                    }
                                    memcpy(&d_b[d_coord_f * typesize], &chunk[s_coord_f * typesize],
                                           r[7] * typesize);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    ctx->free(chunk);
    return 0;
}

int _caterva_get_slice(caterva_array_t *src, void *dest, const uint64_t *start, const uint64_t *stop, const uint64_t *d_pshape) {
	uint8_t *bdest = dest;   // for allowing pointer arithmetic
    uint64_t s_shape[CATERVA_MAXDIM];
    uint64_t s_pshape[CATERVA_MAXDIM];
    uint64_t s_eshape[CATERVA_MAXDIM];
    int8_t s_ndim = src->ndim;

    for (int i = 0; i < CATERVA_MAXDIM; ++i) {
        s_shape[(CATERVA_MAXDIM - s_ndim + i) % CATERVA_MAXDIM] = src->shape[i];
        s_eshape[(CATERVA_MAXDIM - s_ndim + i) % CATERVA_MAXDIM] = src->eshape[i];
        s_pshape[(CATERVA_MAXDIM - s_ndim + i) % CATERVA_MAXDIM] = src->pshape[i];
    }

    /* Create chunk buffers */
    caterva_ctx_t *ctx = src->ctx;
    int typesize = src->sc->typesize;
    uint8_t *chunk = (uint8_t *) ctx->alloc(src->csize * typesize);

    uint64_t i_start[8], i_stop[8];
    for (int i = 0; i < CATERVA_MAXDIM; ++i) {
        i_start[i] = start[i] / s_pshape[i];
        i_stop[i] = (stop[i] - 1) / s_pshape[i];
    }

    /* Calculate the used chunks */
    uint64_t ii[CATERVA_MAXDIM], jj[CATERVA_MAXDIM];
    uint64_t c_start[CATERVA_MAXDIM], c_stop[CATERVA_MAXDIM];
    for (ii[0] = i_start[0]; ii[0] <= i_stop[0]; ++ii[0]) {
        for (ii[1] = i_start[1]; ii[1] <= i_stop[1]; ++ii[1]) {
            for (ii[2] = i_start[2]; ii[2] <= i_stop[2]; ++ii[2]) {
                for (ii[3] = i_start[3]; ii[3] <= i_stop[3]; ++ii[3]) {
                    for (ii[4] = i_start[4]; ii[4] <= i_stop[4]; ++ii[4]) {
                        for (ii[5] = i_start[5]; ii[5] <= i_stop[5]; ++ii[5]) {
                            for (ii[6] = i_start[6]; ii[6] <= i_stop[6]; ++ii[6]) {
                                for (ii[7] = i_start[7]; ii[7] <= i_stop[7]; ++ii[7]) {

                                    int nchunk = 0;
                                    int inc = 1;
                                    for (int i = CATERVA_MAXDIM - 1; i >= 0; --i) {
                                        nchunk += ii[i] * inc;
                                        inc *= s_eshape[i] / s_pshape[i];
                                    }
                                    blosc2_schunk_decompress_chunk(src->sc, nchunk, chunk, src->csize * typesize);
                                    for (int i = 0; i < CATERVA_MAXDIM; ++i) {
                                        if (ii[i] == (start[i] / s_pshape[i])) {
                                            c_start[i] = start[i] % s_pshape[i];
                                        } else {
                                            c_start[i] = 0;
                                        }
                                        if (ii[i] == stop[i] / s_pshape[i]) {
                                            c_stop[i] = stop[i] % s_pshape[i];
                                        } else {
                                            c_stop[i] = s_pshape[i];
                                        }
                                    }
                                    jj[7] = c_start[7];
                                    for (jj[0] = c_start[0]; jj[0] < c_stop[0]; ++jj[0]) {
                                        for (jj[1] = c_start[1]; jj[1] < c_stop[1]; ++jj[1]) {
                                            for (jj[2] = c_start[2]; jj[2] < c_stop[2]; ++jj[2]) {
                                                for (jj[3] = c_start[3]; jj[3] < c_stop[3]; ++jj[3]) {
                                                    for (jj[4] = c_start[4]; jj[4] < c_stop[4]; ++jj[4]) {
                                                        for (jj[5] = c_start[5]; jj[5] < c_stop[5]; ++jj[5]) {
                                                            for (jj[6] = c_start[6]; jj[6] < c_stop[6]; ++jj[6]) {
                                                                uint64_t chunk_pointer = 0;
                                                                uint64_t chunk_pointer_inc = 1;
                                                                for (int i = CATERVA_MAXDIM - 1; i >= 0; --i) {
                                                                    chunk_pointer += jj[i] * chunk_pointer_inc;
                                                                    chunk_pointer_inc *= s_pshape[i];
                                                                }
                                                                uint64_t buf_pointer = 0;
                                                                uint64_t buf_pointer_inc = 1;
                                                                for (int i = CATERVA_MAXDIM - 1; i >= 0; --i) {
                                                                    buf_pointer += (jj[i] + s_pshape[i] * ii[i] -
                                                                        start[i]) * buf_pointer_inc;
                                                                    buf_pointer_inc *= d_pshape[i];
                                                                }
                                                                memcpy(&bdest[buf_pointer * typesize],
                                                                       &chunk[chunk_pointer * typesize],
                                                                       (c_stop[7] - c_start[7]) * typesize);
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    ctx->free(chunk);

    return 0;
}


int caterva_get_slice(caterva_array_t *dest, caterva_array_t *src, caterva_dims_t start, caterva_dims_t stop){

    if (start.ndim != stop.ndim) {
        return -1;
    }
    if (start.ndim != src->ndim) {
        return -1;
    }


    int typesize = src->sc->typesize;
    caterva_ctx_t *ctx = src->ctx;
    uint8_t *chunk = ctx->alloc(dest->csize * typesize);

    uint64_t shape_[CATERVA_MAXDIM];
    for (int i = 0; i < start.ndim; ++i) {
        shape_[i] = stop.dims[i] - start.dims[i];
    }
    for (int i = (int) start.ndim; i < CATERVA_MAXDIM; ++i) {
        shape_[i] = 1;
        start.dims[i] = 0;
    }

    caterva_dims_t shape = caterva_new_dims(shape_, start.ndim);
    caterva_update_shape(dest, shape);

    uint64_t d_shape[CATERVA_MAXDIM];
    uint64_t d_pshape[CATERVA_MAXDIM];
    uint64_t d_eshape[CATERVA_MAXDIM];
    uint64_t d_start[CATERVA_MAXDIM];
    uint64_t d_stop[CATERVA_MAXDIM];
    int8_t d_ndim = dest->ndim;

    for (int i = 0; i < CATERVA_MAXDIM; ++i) {
        d_shape[(CATERVA_MAXDIM - d_ndim + i) % CATERVA_MAXDIM] = dest->shape[i];
        d_eshape[(CATERVA_MAXDIM - d_ndim + i) % CATERVA_MAXDIM] = dest->eshape[i];
        d_pshape[(CATERVA_MAXDIM - d_ndim + i) % CATERVA_MAXDIM] = dest->pshape[i];
        d_start[(CATERVA_MAXDIM - d_ndim + i) % CATERVA_MAXDIM] = start.dims[i];
        d_stop[(CATERVA_MAXDIM - d_ndim + i) % CATERVA_MAXDIM] = stop.dims[i];
    }


    uint64_t ii[CATERVA_MAXDIM];
    for (ii[0] = d_start[0]; ii[0] < d_stop[0]; ii[0] += d_pshape[0]) {
        for (ii[1] = d_start[1]; ii[1] < d_stop[1]; ii[1] += d_pshape[1]) {
            for (ii[2] = d_start[2]; ii[2] < d_stop[2]; ii[2] += d_pshape[2]) {
                for (ii[3] = d_start[3]; ii[3] < d_stop[3]; ii[3] += d_pshape[3]) {
                    for (ii[4] = d_start[4]; ii[4] < d_stop[4]; ii[4] += d_pshape[4]) {
                        for (ii[5] = d_start[5]; ii[5] < d_stop[5]; ii[5] += d_pshape[5]) {
                            for (ii[6] = d_start[6]; ii[6] < d_stop[6]; ii[6] += d_pshape[6]) {
                                for (ii[7] = d_start[7]; ii[7] < d_stop[7]; ii[7] += d_pshape[7]) {
                                    memset(chunk, 0, dest->csize * typesize);
                                    uint64_t jj[CATERVA_MAXDIM];
                                    for (int i = 0; i < CATERVA_MAXDIM; ++i) {
                                        if (ii[i] + d_pshape[i] > d_stop[i]) {
                                            jj[i] = d_stop[i];
                                        }
                                        else {
                                            jj[i] = ii[i] + d_pshape[i];
                                        }
                                    }

                                    _caterva_get_slice(src, chunk, ii, jj, d_pshape);

                                    blosc2_schunk_append_buffer(dest->sc, chunk, dest->csize * typesize);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    caterva_squeeze(dest);
    return 0;
}

int caterva_repart(caterva_array_t *dest, caterva_array_t *src) {
    uint64_t start_[CATERVA_MAXDIM] = {0, 0, 0, 0, 0, 0, 0, 0};
    caterva_dims_t start = caterva_new_dims(start_, dest->ndim);
    uint64_t stop_[CATERVA_MAXDIM];
    for (int i = 0; i < dest->ndim; ++i) {
        stop_[i] = src->shape[i];
    }
    caterva_dims_t stop = caterva_new_dims(stop_, dest->ndim);
    caterva_get_slice(dest, src, start, stop);
    return 0;

}

int caterva_squeeze(caterva_array_t *src) {
    int8_t nones = 0;
    uint64_t newshape_[CATERVA_MAXDIM];
    uint64_t newpshape_[CATERVA_MAXDIM];

    for (int i = 0; i < src->ndim; ++i) {
        if (src->shape[i] != 1) {
            newshape_[nones] = src->shape[i];
            newpshape_[nones] = src->pshape[i];
            nones += 1;
        }
    }

    for (int i = 0; i < CATERVA_MAXDIM; ++i) {
        if (i < nones){
            src->pshape[i] = newpshape_[i];
        } else {
            src->pshape[i] = 1;
        }
    }
    src->ndim = nones;

    caterva_dims_t newshape = caterva_new_dims(newshape_, nones);

    caterva_update_shape(src, newshape);

    return 0;
}


caterva_dims_t caterva_get_shape(caterva_array_t *src){
    caterva_dims_t shape;
    for (int i = 0; i < src->ndim; ++i) {
        shape.dims[i] = src->shape[i];
    }
    shape.ndim = src->ndim;
    return shape;
}

caterva_dims_t caterva_get_pshape(caterva_array_t *src) {
    caterva_dims_t pshape;
    for (int i = 0; i < src->ndim; ++i) {
        pshape.dims[i] = src->pshape[i];
    }
    pshape.ndim = src->ndim;
    return pshape;
}
