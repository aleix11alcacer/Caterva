/*
 * Created by Aleix Alcacer
 */

#include "tests_common.h"

void assert_buf(double *exp, double *real, size_t size, double tol) {
    for (int i = 0; i < size; ++i) {
        LWTEST_ASSERT_ALMOST_EQUAL_DOUBLE(exp[i], real[i], tol);
    }
}

void print_buf(double *buf, size_t size) {
    for (int i = 0; i < size; ++i) {
        printf("%.f  ", buf[i]);
    }
}

void test_get_slice(caterva_array *src, size_t *start, size_t *stop, double *result) {
    size_t buf_size = 1;
    for (int i = 0; i < src->ndim; ++i) {
        buf_size *= (stop[i] - start[i]);
    }

    double *buf = (double *) malloc(buf_size * src->sc->typesize);
    caterva_get_slice(src, buf, start, stop);
    assert_buf(buf, result, buf_size, 1e-14);
    //print_buf(buf, buf_size);
    free(buf);
}

LWTEST_DATA(get_slice) {
    blosc2_cparams cp;
    blosc2_dparams dp;
};

LWTEST_SETUP(get_slice) {
    data->cp = BLOSC_CPARAMS_DEFAULTS;
    data->cp.typesize = sizeof(double);
    data->dp = BLOSC_DPARAMS_DEFAULTS;
}

LWTEST_TEARDOWN(get_slice) {

}

LWTEST_FIXTURE(get_slice, ndim_2) {
    const size_t ndim = 2;
    size_t shape[ndim] = {10, 10};
    size_t cshape[ndim] = {3, 2};
    size_t start[ndim] = {5, 3};
    size_t stop[ndim] = {9, 10};
    double result[1024] = {53, 54, 55, 56, 57, 58, 59, 63, 64, 65, 66, 67, 68, 69, 73, 74, 75, 76,
                           77, 78, 79, 83, 84, 85, 86, 87, 88, 89};
    caterva_pparams pp = caterva_new_pparams(shape, cshape, ndim);
    caterva_array *src = caterva_new_array(data->cp, data->dp, NULL, pp);

    double *buf = (double *) malloc(src->size * src->sc->typesize);
    for (int i = 0; i < src->size; ++i) {
        buf[i] = i;
    }
    caterva_from_buffer(src, buf);

    test_get_slice(src, start, stop, result);

    caterva_free_array(src);
    free(buf);
}


LWTEST_FIXTURE(get_slice, ndim_3) {
    const size_t ndim = 3;
    size_t shape[ndim] = {10, 10, 10};
    size_t cshape[ndim] = {3, 5, 2};
    size_t start[ndim] = {3, 0, 3};
    size_t stop[ndim] = {6, 7, 10};
    double result[1024] = {303, 304, 305, 306, 307, 308, 309, 313, 314, 315, 316, 317, 318, 319,
                           323, 324, 325, 326, 327, 328, 329, 333, 334, 335, 336, 337, 338, 339,
                           343, 344, 345, 346, 347, 348, 349, 353, 354, 355, 356, 357, 358, 359,
                           363, 364, 365, 366, 367, 368, 369, 403, 404, 405, 406, 407, 408, 409,
                           413, 414, 415, 416, 417, 418, 419, 423, 424, 425, 426, 427, 428, 429,
                           433, 434, 435, 436, 437, 438, 439, 443, 444, 445, 446, 447, 448, 449,
                           453, 454, 455, 456, 457, 458, 459, 463, 464, 465, 466, 467, 468, 469,
                           503, 504, 505, 506, 507, 508, 509, 513, 514, 515, 516, 517, 518, 519,
                           523, 524, 525, 526, 527, 528, 529, 533, 534, 535, 536, 537, 538, 539,
                           543, 544, 545, 546, 547, 548, 549, 553, 554, 555, 556, 557, 558, 559,
                           563, 564, 565, 566, 567, 568, 569};
    caterva_pparams pp = caterva_new_pparams(shape, cshape, ndim);
    caterva_array *src = caterva_new_array(data->cp, data->dp, NULL, pp);

    double *buf = (double *) malloc(src->size * src->sc->typesize);
    for (int i = 0; i < src->size; ++i) {
        buf[i] = i;
    }
    caterva_from_buffer(src, buf);

    test_get_slice(src, start, stop, result);

    caterva_free_array(src);
    free(buf);
}

LWTEST_FIXTURE(get_slice, ndim_4) {
    const size_t ndim = 4;
    size_t shape[ndim] = {10, 10, 10, 10};
    size_t cshape[ndim] = {3, 5, 2, 7};
    size_t start[ndim] = {5, 3, 9, 2};
    size_t stop[ndim] = {9, 6, 10, 7};
    double result[1024] = {5392, 5393, 5394, 5395, 5396, 5492, 5493, 5494, 5495, 5496, 5592, 5593,
                           5594, 5595, 5596, 6392, 6393, 6394, 6395, 6396, 6492, 6493, 6494, 6495,
                           6496, 6592, 6593, 6594, 6595, 6596, 7392, 7393, 7394, 7395, 7396, 7492,
                           7493, 7494, 7495, 7496, 7592, 7593, 7594, 7595, 7596, 8392, 8393, 8394,
                           8395, 8396, 8492, 8493, 8494, 8495, 8496, 8592, 8593, 8594, 8595, 8596};
    caterva_pparams pp = caterva_new_pparams(shape, cshape, ndim);
    caterva_array *src = caterva_new_array(data->cp, data->dp, NULL, pp);

    double *buf = (double *) malloc(src->size * src->sc->typesize);
    for (int i = 0; i < src->size; ++i) {
        buf[i] = i;
    }
    caterva_from_buffer(src, buf);

    test_get_slice(src, start, stop, result);

    caterva_free_array(src);
    free(buf);
}

LWTEST_FIXTURE(get_slice, ndim_5) {
    const size_t ndim = 5;
    size_t shape[ndim] = {10, 10, 10, 10, 10};
    size_t cshape[ndim] = {3, 5, 2, 4, 5};
    size_t start[ndim] = {6, 0, 5, 5, 7};
    size_t stop[ndim] = {8, 9, 6, 6, 10};
    double result[1024] = {60557, 60558, 60559, 61557, 61558, 61559, 62557, 62558, 62559, 63557,
                           63558, 63559, 64557, 64558, 64559, 65557, 65558, 65559, 66557, 66558,
                           66559, 67557, 67558, 67559, 68557, 68558, 68559, 70557, 70558, 70559,
                           71557, 71558, 71559, 72557, 72558, 72559, 73557, 73558, 73559, 74557,
                           74558, 74559, 75557, 75558, 75559, 76557, 76558, 76559, 77557, 77558,
                           77559, 78557, 78558, 78559};
    caterva_pparams pp = caterva_new_pparams(shape, cshape, ndim);
    caterva_array *src = caterva_new_array(data->cp, data->dp, NULL, pp);

    double *buf = (double *) malloc(src->size * src->sc->typesize);
    for (int i = 0; i < src->size; ++i) {
        buf[i] = i;
    }
    caterva_from_buffer(src, buf);

    test_get_slice(src, start, stop, result);

    caterva_free_array(src);
    free(buf);
}

LWTEST_FIXTURE(get_slice, ndim_6) {
    const size_t ndim = 6;
    size_t shape[ndim] = {10, 10, 10, 10, 10, 10};
    size_t cshape[ndim] = {4, 5, 3, 8, 3, 3};
    size_t start[ndim] = {0, 4, 2, 4, 5, 1};
    size_t stop[ndim] = {1, 7, 4, 6, 8, 3};
    double result[1024] = {42451, 42452, 42461, 42462, 42471, 42472, 42551, 42552, 42561, 42562,
                           42571, 42572, 43451, 43452, 43461, 43462, 43471, 43472, 43551, 43552,
                           43561, 43562, 43571, 43572, 52451, 52452, 52461, 52462, 52471, 52472,
                           52551, 52552, 52561, 52562, 52571, 52572, 53451, 53452, 53461, 53462,
                           53471, 53472, 53551, 53552, 53561, 53562, 53571, 53572, 62451, 62452,
                           62461, 62462, 62471, 62472, 62551, 62552, 62561, 62562, 62571, 62572,
                           63451, 63452, 63461, 63462, 63471, 63472, 63551, 63552, 63561, 63562,
                           63571, 63572};
    caterva_pparams pp = caterva_new_pparams(shape, cshape, ndim);
    caterva_array *src = caterva_new_array(data->cp, data->dp, NULL, pp);

    double *buf = (double *) malloc(src->size * src->sc->typesize);
    for (int i = 0; i < src->size; ++i) {
        buf[i] = i;
    }
    caterva_from_buffer(src, buf);

    test_get_slice(src, start, stop, result);

    caterva_free_array(src);
    free(buf);
}

LWTEST_FIXTURE(get_slice, ndim_7) {
    const size_t ndim = 7;
    size_t shape[ndim] = {10, 10, 10, 10, 10, 10, 10};
    size_t cshape[ndim] = {4, 5, 1, 8, 5, 3, 10};
    size_t start[ndim] = {5, 4, 3, 8, 4, 5, 1};
    size_t stop[ndim] = {8, 6, 5, 9, 7, 7, 3};
    double result[1024] = {5438451, 5438452, 5438461, 5438462, 5438551, 5438552, 5438561, 5438562,
                           5438651, 5438652, 5438661, 5438662, 5448451, 5448452, 5448461, 5448462,
                           5448551, 5448552, 5448561, 5448562, 5448651, 5448652, 5448661, 5448662,
                           5538451, 5538452, 5538461, 5538462, 5538551, 5538552, 5538561, 5538562,
                           5538651, 5538652, 5538661, 5538662, 5548451, 5548452, 5548461, 5548462,
                           5548551, 5548552, 5548561, 5548562, 5548651, 5548652, 5548661, 5548662,
                           6438451, 6438452, 6438461, 6438462, 6438551, 6438552, 6438561, 6438562,
                           6438651, 6438652, 6438661, 6438662, 6448451, 6448452, 6448461, 6448462,
                           6448551, 6448552, 6448561, 6448562, 6448651, 6448652, 6448661, 6448662,
                           6538451, 6538452, 6538461, 6538462, 6538551, 6538552, 6538561, 6538562,
                           6538651, 6538652, 6538661, 6538662, 6548451, 6548452, 6548461, 6548462,
                           6548551, 6548552, 6548561, 6548562, 6548651, 6548652, 6548661, 6548662,
                           7438451, 7438452, 7438461, 7438462, 7438551, 7438552, 7438561, 7438562,
                           7438651, 7438652, 7438661, 7438662, 7448451, 7448452, 7448461, 7448462,
                           7448551, 7448552, 7448561, 7448562, 7448651, 7448652, 7448661, 7448662,
                           7538451, 7538452, 7538461, 7538462, 7538551, 7538552, 7538561, 7538562,
                           7538651, 7538652, 7538661, 7538662, 7548451, 7548452, 7548461, 7548462,
                           7548551, 7548552, 7548561, 7548562, 7548651, 7548652, 7548661, 7548662};
    caterva_pparams pp = caterva_new_pparams(shape, cshape, ndim);
    caterva_array *src = caterva_new_array(data->cp, data->dp, NULL, pp);

    double *buf = (double *) malloc(src->size * src->sc->typesize);
    for (int i = 0; i < src->size; ++i) {
        buf[i] = i;
    }
    caterva_from_buffer(src, buf);

    test_get_slice(src, start, stop, result);

    caterva_free_array(src);
    free(buf);
}

LWTEST_FIXTURE(get_slice, ndim_8) {
    const size_t ndim = 8;
    size_t shape[ndim] = {10, 10, 10, 10, 10, 10, 10, 10};
    size_t cshape[ndim] = {2, 3, 4, 2, 3, 2, 4, 10};
    size_t start[ndim] = {3, 5, 2, 4, 5, 1, 6, 0};
    size_t stop[ndim] = {6, 6, 4, 6, 7, 3, 7, 3};
    double result[1024] = {35245160, 35245161, 35245162, 35245260, 35245261, 35245262, 35246160,
                           35246161, 35246162, 35246260, 35246261, 35246262, 35255160, 35255161,
                           35255162, 35255260, 35255261, 35255262, 35256160, 35256161, 35256162,
                           35256260, 35256261, 35256262, 35345160, 35345161, 35345162, 35345260,
                           35345261, 35345262, 35346160, 35346161, 35346162, 35346260, 35346261,
                           35346262, 35355160, 35355161, 35355162, 35355260, 35355261, 35355262,
                           35356160, 35356161, 35356162, 35356260, 35356261, 35356262, 45245160,
                           45245161, 45245162, 45245260, 45245261, 45245262, 45246160, 45246161,
                           45246162, 45246260, 45246261, 45246262, 45255160, 45255161, 45255162,
                           45255260, 45255261, 45255262, 45256160, 45256161, 45256162, 45256260,
                           45256261, 45256262, 45345160, 45345161, 45345162, 45345260, 45345261,
                           45345262, 45346160, 45346161, 45346162, 45346260, 45346261, 45346262,
                           45355160, 45355161, 45355162, 45355260, 45355261, 45355262, 45356160,
                           45356161, 45356162, 45356260, 45356261, 45356262, 55245160, 55245161,
                           55245162, 55245260, 55245261, 55245262, 55246160, 55246161, 55246162,
                           55246260, 55246261, 55246262, 55255160, 55255161, 55255162, 55255260,
                           55255261, 55255262, 55256160, 55256161, 55256162, 55256260, 55256261,
                           55256262, 55345160, 55345161, 55345162, 55345260, 55345261, 55345262,
                           55346160, 55346161, 55346162, 55346260, 55346261, 55346262, 55355160,
                           55355161, 55355162, 55355260, 55355261, 55355262, 55356160, 55356161,
                           55356162, 55356260, 55356261, 55356262};
    caterva_pparams pp = caterva_new_pparams(shape, cshape, ndim);
    caterva_array *src = caterva_new_array(data->cp, data->dp, NULL, pp);

    double *buf = (double *) malloc(src->size * src->sc->typesize);
    for (int i = 0; i < src->size; ++i) {
        buf[i] = i;
    }
    caterva_from_buffer(src, buf);

    test_get_slice(src, start, stop, result);

    caterva_free_array(src);
    free(buf);
}