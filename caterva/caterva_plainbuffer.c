/*
 * Copyright (C) 2018 Francesc Alted, Aleix Alcacer.
 * Copyright (C) 2019-present Blosc Development team <blosc@blosc.org>
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */

#include <caterva.h>

int caterva_plainbuffer_array_free(caterva_context_t *ctx, caterva_array_t **array) {
    if ((*array)->buf != NULL) {
        ctx->cfg->free((*array)->buf);
    }
    return CATERVA_SUCCEED;
}

int caterva_plainbuffer_array_append(caterva_context_t *ctx, caterva_array_t *array, void *chunk,
                                     int64_t chunksize) {
    int64_t start[CATERVA_MAX_DIM];
    int64_t stop[CATERVA_MAX_DIM];
    for (int i = 0; i < array->ndim; ++i) {
        start[i] = 0;
        stop[i] = start[i] + array->chunkshape[i];
    }

    CATERVA_ERROR(caterva_array_set_slice_buffer(ctx, chunk, chunksize, start, stop, array));

    return CATERVA_SUCCEED;
}

int caterva_plainbuffer_array_from_buffer(caterva_context_t *ctx, caterva_array_t *array,
                                          void *buffer, int64_t buffersize) {
    CATERVA_ERROR(caterva_array_append(ctx, array, buffer, buffersize));
    return CATERVA_SUCCEED;
}

int caterva_plainbuffer_array_to_buffer(caterva_context_t *ctx, caterva_array_t *array,
                                        void *buffer) {
    CATERVA_UNUSED_PARAM(ctx);
    memcpy(buffer, array->buf, (size_t) array->nitems * array->itemsize);
    return CATERVA_SUCCEED;
}

int caterva_plainbuffer_array_get_slice_buffer(caterva_context_t *ctx, caterva_array_t *array,
                                               int64_t *start, int64_t *stop, int64_t *shape,
                                               void *buffer) {
    CATERVA_UNUSED_PARAM(ctx);

    int64_t start__[CATERVA_MAX_DIM];
    int64_t stop__[CATERVA_MAX_DIM];
    int64_t shape__[CATERVA_MAX_DIM];
    int64_t shape2__[CATERVA_MAX_DIM];

    for (int i = 0; i < CATERVA_MAX_DIM; ++i) {
        start__[i] = (i < array->ndim) ? start[i] : 0;
        stop__[i] = (i < array->ndim) ? stop[i] : 1;
        shape__[i] = (i < array->ndim) ? array->shape[i] : 1;
        shape2__[i] = (i < array->ndim) ? shape[i] : 1;
    }

    uint8_t *bdest = buffer;  // for allowing pointer arithmetic
    int64_t start_[CATERVA_MAX_DIM];
    int64_t stop_[CATERVA_MAX_DIM];
    int64_t d_pshape_[CATERVA_MAX_DIM];
    int8_t s_ndim = array->ndim;

    int64_t s_shape[CATERVA_MAX_DIM];
    for (int i = 0; i < CATERVA_MAX_DIM; ++i) {
        start_[(CATERVA_MAX_DIM - s_ndim + i) % CATERVA_MAX_DIM] = start__[i];
        stop_[(CATERVA_MAX_DIM - s_ndim + i) % CATERVA_MAX_DIM] = stop__[i];
        s_shape[(CATERVA_MAX_DIM - s_ndim + i) % CATERVA_MAX_DIM] = shape__[i];
        d_pshape_[(CATERVA_MAX_DIM - s_ndim + i) % CATERVA_MAX_DIM] = shape2__[i];
    }
    for (int j = 0; j < CATERVA_MAX_DIM - s_ndim; ++j) {
        start_[j] = 0;
    }
    int64_t jj[CATERVA_MAX_DIM];
    jj[7] = start_[7];
    for (jj[0] = start_[0]; jj[0] < stop_[0]; ++jj[0]) {
        for (jj[1] = start_[1]; jj[1] < stop_[1]; ++jj[1]) {
            for (jj[2] = start_[2]; jj[2] < stop_[2]; ++jj[2]) {
                for (jj[3] = start_[3]; jj[3] < stop_[3]; ++jj[3]) {
                    for (jj[4] = start_[4]; jj[4] < stop_[4]; ++jj[4]) {
                        for (jj[5] = start_[5]; jj[5] < stop_[5]; ++jj[5]) {
                            for (jj[6] = start_[6]; jj[6] < stop_[6]; ++jj[6]) {
                                int64_t chunk_pointer = 0;
                                int64_t chunk_pointer_inc = 1;
                                for (int i = CATERVA_MAX_DIM - 1; i >= 0; --i) {
                                    chunk_pointer += jj[i] * chunk_pointer_inc;
                                    chunk_pointer_inc *= s_shape[i];
                                }
                                int64_t buf_pointer = 0;
                                int64_t buf_pointer_inc = 1;
                                for (int i = CATERVA_MAX_DIM - 1; i >= 0; --i) {
                                    buf_pointer += (jj[i] - start_[i]) * buf_pointer_inc;
                                    buf_pointer_inc *= d_pshape_[i];
                                }
                                memcpy(&bdest[buf_pointer * array->itemsize],
                                       &array->buf[chunk_pointer * array->itemsize],
                                       (size_t)(stop_[7] - start_[7]) * array->itemsize);
                            }
                        }
                    }
                }
            }
        }
    }
    return CATERVA_SUCCEED;
}

int caterva_plainbuffer_array_set_slice_buffer(caterva_context_t *ctx, void *buffer,
                                               int64_t buffersize, int64_t *start, int64_t *stop,
                                               caterva_array_t *array) {
    CATERVA_UNUSED_PARAM(ctx);
    CATERVA_UNUSED_PARAM(buffersize);

    uint8_t *bbuffer = buffer;  // for allowing pointer arithmetic
    int64_t start_[CATERVA_MAX_DIM];
    int64_t stop_[CATERVA_MAX_DIM];
    int8_t s_ndim = array->ndim;

    int64_t d_shape[CATERVA_MAX_DIM];
    int64_t s_shape[CATERVA_MAX_DIM];
    for (int i = 0; i < CATERVA_MAX_DIM; ++i) {
        start_[(CATERVA_MAX_DIM - s_ndim + i) % CATERVA_MAX_DIM] = start[i];
        stop_[(CATERVA_MAX_DIM - s_ndim + i) % CATERVA_MAX_DIM] = stop[i];
        d_shape[(CATERVA_MAX_DIM - s_ndim + i) % CATERVA_MAX_DIM] = (stop[i] - start[i]);
        s_shape[(CATERVA_MAX_DIM - s_ndim + i) % CATERVA_MAX_DIM] = array->shape[i];
    }
    for (int j = 0; j < CATERVA_MAX_DIM - s_ndim; ++j) {
        start_[j] = 0;
        stop_[j] = 1;
        d_shape[j] = 1;
    }
    int64_t jj[CATERVA_MAX_DIM];
    jj[7] = start_[7];
    for (jj[0] = start_[0]; jj[0] < stop_[0]; ++jj[0]) {
        for (jj[1] = start_[1]; jj[1] < stop_[1]; ++jj[1]) {
            for (jj[2] = start_[2]; jj[2] < stop_[2]; ++jj[2]) {
                for (jj[3] = start_[3]; jj[3] < stop_[3]; ++jj[3]) {
                    for (jj[4] = start_[4]; jj[4] < stop_[4]; ++jj[4]) {
                        for (jj[5] = start_[5]; jj[5] < stop_[5]; ++jj[5]) {
                            for (jj[6] = start_[6]; jj[6] < stop_[6]; ++jj[6]) {
                                int64_t chunk_pointer = 0;
                                int64_t chunk_pointer_inc = 1;
                                for (int i = CATERVA_MAX_DIM - 1; i >= 0; --i) {
                                    chunk_pointer += jj[i] * chunk_pointer_inc;
                                    chunk_pointer_inc *= s_shape[i];
                                }
                                int64_t buf_pointer = 0;
                                int64_t buf_pointer_inc = 1;
                                for (int i = CATERVA_MAX_DIM - 1; i >= 0; --i) {
                                    buf_pointer += (jj[i] - start_[i]) * buf_pointer_inc;
                                    buf_pointer_inc *= d_shape[i];
                                }
                                memcpy(&array->buf[chunk_pointer * array->itemsize],
                                       &bbuffer[buf_pointer * array->itemsize],
                                       (size_t)(stop_[7] - start_[7]) * array->itemsize);
                            }
                        }
                    }
                }
            }
        }
    }
    return CATERVA_SUCCEED;
}

int caterva_plainbuffer_array_get_slice(caterva_context_t *ctx, caterva_array_t *src,
                                        int64_t *start, int64_t *stop, caterva_array_t *array) {
    int typesize = src->itemsize;

    uint64_t size = 1;
    for (int i = 0; i < src->ndim; ++i) {
        size *= stop[i] - start[i];
    }
    CATERVA_ERROR(caterva_array_get_slice_buffer(ctx, src, start, stop, array->shape, array->buf,
                                                 size * typesize));
    array->filled = true;
    array->empty = false;

    return CATERVA_SUCCEED;
}

int caterva_plainbuffer_update_shape(caterva_array_t *array, int8_t ndim, int64_t *shape) {
    array->ndim = ndim;
    array->nitems = 1;
    array->extnitems = 1;
    array->chunknitems = 1;
    for (int i = 0; i < CATERVA_MAX_DIM; ++i) {
        if (i < ndim) {
            array->shape[i] = shape[i];
            array->extshape[i] = shape[i];
            array->chunkshape[i] = (int32_t)(shape[i]);
        } else {
            array->shape[i] = 1;
            array->extshape[i] = 1;
            array->chunkshape[i] = 1;
        }
        array->nitems *= array->shape[i];
        array->extnitems *= array->extshape[i];
        array->chunknitems *= array->chunkshape[i];
    }

    return CATERVA_SUCCEED;
}

int caterva_plainbuffer_array_squeeze(caterva_context_t *ctx, caterva_array_t *array) {
    CATERVA_UNUSED_PARAM(ctx);

    uint8_t nones = 0;
    int64_t newshape[CATERVA_MAX_DIM];
    for (int i = 0; i < array->ndim; ++i) {
        if (array->shape[i] != 1) {
            newshape[nones] = array->shape[i];
            nones += 1;
        }
    }

    CATERVA_ERROR(caterva_plainbuffer_update_shape(array, nones, newshape));

    return CATERVA_SUCCEED;
}

int caterva_plainbuffer_array_copy(caterva_context_t *ctx, caterva_params_t *params,
                                   caterva_storage_t *storage, caterva_array_t *src,
                                   caterva_array_t **dest) {
    CATERVA_ERROR(caterva_array_empty(ctx, params, storage, dest));

    CATERVA_ERROR(
        caterva_array_to_buffer(ctx, src, (*dest)->buf, (*dest)->nitems * (*dest)->itemsize));
    (*dest)->filled = true;

    return CATERVA_SUCCEED;
}

int caterva_plainbuffer_array_empty(caterva_context_t *ctx, caterva_params_t *params,
                                    caterva_storage_t *storage, caterva_array_t **array) {
    /* Create a caterva_array_t buffer */
    (*array) = (caterva_array_t *) ctx->cfg->alloc(sizeof(caterva_array_t));
    if ((*array) == NULL) {
        DEBUG_PRINT("Pointer is null");
        return CATERVA_ERR_NULL_POINTER;
    }

    (*array)->storage = storage->backend;
    (*array)->ndim = params->ndim;
    (*array)->itemsize = params->itemsize;

    int64_t *shape = params->shape;

    (*array)->nitems = 1;
    (*array)->chunknitems = 1;
    (*array)->extnitems = 1;

    for (int i = 0; i < params->ndim; ++i) {
        (*array)->shape[i] = shape[i];
        (*array)->chunkshape[i] = (uint32_t) shape[i];
        (*array)->extshape[i] = shape[i];

        (*array)->nitems *= shape[i];
        (*array)->chunknitems *= (uint32_t) shape[i];
        (*array)->extnitems *= shape[i];
    }

    for (int i = params->ndim; i < CATERVA_MAX_DIM; ++i) {
        (*array)->shape[i] = 1;
        (*array)->chunkshape[i] = 1;
        (*array)->extshape[i] = 1;
    }

    // The partition cache (empty initially)
    (*array)->chunk_cache.data = NULL;
    (*array)->chunk_cache.nchunk = -1;  // means no valid cache yet

    (*array)->sc = NULL;

    uint8_t *buf = ctx->cfg->alloc((size_t)(*array)->extnitems * params->itemsize);

    (*array)->buf = buf;

    return CATERVA_SUCCEED;
}
