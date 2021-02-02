/* 
 * This file is part of the HDDP Switch distribution (https://github.com/gistnetserv-uah/eHDDP).
 * Copyright (c) 2020.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OFL_UTILS_H
#define OFL_UTILS_H 1


#include <netinet/in.h>


/* Given an array of pointers _elem_, and the number of elements in the array
   _elem_num_, this function frees each element, as well as the array
   itself. */
#define OFL_UTILS_FREE_ARR(ELEMS, ELEM_NUM)     \
{                                               \
     size_t _iter;                              \
     for (_iter=0; _iter<ELEM_NUM; _iter++) {   \
         free(ELEMS[_iter]);                    \
     }                                          \
     free(ELEMS);                               \
}

 /* Given an array of pointers _elem_, and the number of elements in the array
    _elem_num_, this function frees each element using the provided _free_fun_
    function, and frees the array itself as well. */
#define OFL_UTILS_FREE_ARR_FUN(ELEMS, ELEM_NUM, FREE_FUN) \
{                                               \
     size_t _iter;                              \
     for (_iter=0; _iter<ELEM_NUM; _iter++) {   \
         FREE_FUN(ELEMS[_iter]);                \
     }                                          \
     free(ELEMS);                               \
}

#define OFL_UTILS_FREE_ARR_FUN2(ELEMS, ELEM_NUM, FREE_FUN, ARG2) \
{                                                \
     size_t _iter;                               \
     for (_iter=0; _iter<ELEM_NUM; _iter++) {    \
         FREE_FUN(ELEMS[_iter], ARG2);           \
     }                                           \
     free(ELEMS);                                \
}



/* Given an array of pointers _elem_, and the number of elements in the array
   _elem_num_, this function sums the result of calling the provided _len_fun_
   function for each element. */
#define OFL_UTILS_SUM_ARR_FUN(RESULT, ELEMS, ELEM_NUM, LEN_FUN) \
{                                                \
     size_t _iter, _ret;                         \
                                                 \
     _ret = 0;                                   \
     for (_iter=0; _iter<ELEM_NUM; _iter++) {    \
         _ret += LEN_FUN(ELEMS[_iter]);          \
     }                                           \
                                                 \
     RESULT = _ret;                              \
}


#define OFL_UTILS_SUM_ARR_FUN2(RESULT, ELEMS, ELEM_NUM, LEN_FUN, ARG2) \
{                                                    \
     size_t _iter, _ret;                             \
                                                     \
     _ret = 0;                                       \
     for (_iter=0; _iter<ELEM_NUM; _iter++) {        \
         _ret += LEN_FUN(ELEMS[_iter], ARG2);        \
     }                                               \
                                                     \
     RESULT = _ret;                                  \
}


static inline uint64_t
hton64(uint64_t n) {
#if __BYTE_ORDER == __BIG_ENDIAN
    return n;
#else
    return (((uint64_t)htonl(n)) << 32) + htonl(n >> 32);
#endif
}

static inline uint64_t
ntoh64(uint64_t n) {
#if __BYTE_ORDER == __BIG_ENDIAN
    return n;
#else
    return (((uint64_t)ntohl(n)) << 32) + ntohl(n >> 32);
#endif
}


#endif /* OFL_UTILS_H */
