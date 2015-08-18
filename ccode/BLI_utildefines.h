/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#ifndef __BLI_UTILDEFINES_H__
#define __BLI_UTILDEFINES_H__

/** \file BLI_utildefines.h
 *  \ingroup bli
 */

#define MIN2(a, b)  ((a) < (b) ? (a) : (b))
#define MAX2(a, b)  ((a) > (b) ? (a) : (b))

#define MIN3(a, b, c)       (MIN2(MIN2((a), (b)), (c)))
#define MIN4(a, b, c, d)    (MIN2(MIN2((a), (b)), MIN2((c), (d))))

#define MAX3(a, b, c)       (MAX2(MAX2((a), (b)), (c)))
#define MAX4(a, b, c, d)    (MAX2(MAX2((a), (b)), MAX2((c), (d))))

/* min/max that return a value of our choice */
#define MAX3_PAIR(cmp_a, cmp_b, cmp_c, ret_a, ret_b, ret_c) \
	((cmp_a > cmp_b) ? ((cmp_a > cmp_c) ? ret_a : ret_c) : \
	                   ((cmp_b > cmp_c) ? ret_b : ret_c))

#define MIN3_PAIR(cmp_a, cmp_b, cmp_c, ret_a, ret_b, ret_c) \
	((cmp_a < cmp_b) ? ((cmp_a < cmp_c) ? ret_a : ret_c) : \
	                   ((cmp_b < cmp_c) ? ret_b : ret_c))

#define INIT_MINMAX(min, max) {                                               \
		(min)[0] = (min)[1] = (min)[2] =  1.0e30f;                            \
		(max)[0] = (max)[1] = (max)[2] = -1.0e30f;                            \
	} (void)0
#define INIT_MINMAX2(min, max) {                                              \
		(min)[0] = (min)[1] = 1.0e30f;                                        \
		(max)[0] = (max)[1] = -1.0e30f;                                       \
	} (void)0
#define DO_MIN(vec, min) {                                                    \
		if ((min)[0] > (vec)[0]) (min)[0] = (vec)[0];                         \
		if ((min)[1] > (vec)[1]) (min)[1] = (vec)[1];                         \
		if ((min)[2] > (vec)[2]) (min)[2] = (vec)[2];                         \
	} (void)0
#define DO_MAX(vec, max) {                                                    \
		if ((max)[0] < (vec)[0]) (max)[0] = (vec)[0];                         \
		if ((max)[1] < (vec)[1]) (max)[1] = (vec)[1];                         \
		if ((max)[2] < (vec)[2]) (max)[2] = (vec)[2];                         \
	} (void)0
#define DO_MINMAX(vec, min, max) {                                            \
		if ((min)[0] > (vec)[0] ) (min)[0] = (vec)[0];                        \
		if ((min)[1] > (vec)[1] ) (min)[1] = (vec)[1];                        \
		if ((min)[2] > (vec)[2] ) (min)[2] = (vec)[2];                        \
		if ((max)[0] < (vec)[0] ) (max)[0] = (vec)[0];                        \
		if ((max)[1] < (vec)[1] ) (max)[1] = (vec)[1];                        \
		if ((max)[2] < (vec)[2] ) (max)[2] = (vec)[2];                        \
	} (void)0
#define DO_MINMAX2(vec, min, max) {                                           \
		if ((min)[0] > (vec)[0] ) (min)[0] = (vec)[0];                        \
		if ((min)[1] > (vec)[1] ) (min)[1] = (vec)[1];                        \
		if ((max)[0] < (vec)[0] ) (max)[0] = (vec)[0];                        \
		if ((max)[1] < (vec)[1] ) (max)[1] = (vec)[1];                        \
	} (void)0

/* some math and copy defines */


#define SWAP(type, a, b)  {    \
	type sw_ap;                \
	sw_ap = (a);               \
	(a) = (b);                 \
	(b) = sw_ap;               \
} (void)0

/* swap with a temp value */
#define SWAP_TVAL(tval, a, b)  {  \
	(tval) = (a);                 \
	(a) = (b);                    \
	(b) = (tval);                 \
} (void)0

/* no-op for expressions we don't want to instansiate, but must remian valid */
#define EXPR_NOP(expr) (void)(0 ? ((void)(expr), 1) : 0)
#define VECCOPY(v1, v2) {                                                     \
		*(v1) =   *(v2);                                                      \
		*(v1 + 1) = *(v2 + 1);                                                \
		*(v1 + 2) = *(v2 + 2);                                                \
} (void)0
#define VECCOPY2D(v1, v2) {                                                   \
		*(v1) =   *(v2);                                                      \
		*(v1 + 1) = *(v2 + 1);                                                \
} (void)0
#define VECADD(v1, v2, v3) {                                                  \
		*(v1) =   *(v2)   + *(v3);                                            \
		*(v1 + 1) = *(v2 + 1) + *(v3 + 1);                                    \
		*(v1 + 2) = *(v2 + 2) + *(v3 + 2);                                    \
} (void)0
#define VECSUB(v1, v2, v3) {                                                  \
		*(v1) =   *(v2)   - *(v3);                                            \
		*(v1 + 1) = *(v2 + 1) - *(v3 + 1);                                    \
		*(v1 + 2) = *(v2 + 2) - *(v3 + 2);                                    \
} (void)0
#define VECSUB2D(v1, v2, v3)     {                                            \
		*(v1) =   *(v2)   - *(v3);                                            \
		*(v1 + 1) = *(v2 + 1) - *(v3 + 1);                                    \
} (void)0
#define VECADDFAC(v1, v2, v3, fac) {                                          \
		*(v1) =   *(v2)   + *(v3) * (fac);                                    \
		*(v1 + 1) = *(v2 + 1) + *(v3 + 1) * (fac);                            \
		*(v1 + 2) = *(v2 + 2) + *(v3 + 2) * (fac);                            \
} (void)0
#define VECMADD(v1, v2, v3, v4) {                                             \
		*(v1) =   *(v2)   + *(v3) * (*(v4));                                  \
		*(v1 + 1) = *(v2 + 1) + *(v3 + 1) * (*(v4 + 1));                      \
		*(v1 + 2) = *(v2 + 2) + *(v3 + 2) * (*(v4 + 2));                      \
} (void)0
#define VECSUBFAC(v1, v2, v3, fac) {                                          \
		*(v1) =   *(v2)   - *(v3) * (fac);                                    \
		*(v1 + 1) = *(v2 + 1) - *(v3 + 1) * (fac);                            \
		*(v1 + 2) = *(v2 + 2) - *(v3 + 2) * (fac);                            \
} (void)0

#define ABS(a)  ((a) < 0 ? (-(a)) : (a))
#define SQUARE(a)  ((a) * (a))

#define CLAMPIS(a, b, c)  ((a) < (b) ? (b) : (a) > (c) ? (c) : (a))

#define CLAMP(a, b, c)  {           \
	if      ((a) < (b)) (a) = (b);  \
	else if ((a) > (c)) (a) = (c);  \
} (void)0

#define CLAMP_MAX(a, c)  {          \
	if ((a) > (c)) (a) = (c);       \
} (void)0

#define CLAMP_MIN(a, b)  {          \
	if      ((a) < (b)) (a) = (b);  \
} (void)0

#define CLAMP2(vec, b, c) { \
	CLAMP((vec)[0], b, c); \
	CLAMP((vec)[1], b, c); \
} (void)0

#define CLAMP2_MIN(vec, b) { \
	CLAMP_MIN((vec)[0], b); \
	CLAMP_MIN((vec)[1], b); \
} (void)0

#define CLAMP2_MAX(vec, b) { \
	CLAMP_MAX((vec)[0], b); \
	CLAMP_MAX((vec)[1], b); \
} (void)0

#define CLAMP3(vec, b, c) { \
	CLAMP((vec)[0], b, c); \
	CLAMP((vec)[1], b, c); \
	CLAMP((vec)[2], b, c); \
} (void)0

#define CLAMP3_MIN(vec, b) { \
	CLAMP_MIN((vec)[0], b); \
	CLAMP_MIN((vec)[1], b); \
	CLAMP_MIN((vec)[2], b); \
} (void)0

#define CLAMP3_MAX(vec, b) { \
	CLAMP_MAX((vec)[0], b); \
	CLAMP_MAX((vec)[1], b); \
	CLAMP_MAX((vec)[2], b); \
} (void)0

#define CLAMP4(vec, b, c) { \
	CLAMP((vec)[0], b, c); \
	CLAMP((vec)[1], b, c); \
	CLAMP((vec)[2], b, c); \
	CLAMP((vec)[3], b, c); \
} (void)0

#define CLAMP4_MIN(vec, b) { \
	CLAMP_MIN((vec)[0], b); \
	CLAMP_MIN((vec)[1], b); \
	CLAMP_MIN((vec)[2], b); \
	CLAMP_MIN((vec)[3], b); \
} (void)0

#define CLAMP4_MAX(vec, b) { \
	CLAMP_MAX((vec)[0], b); \
	CLAMP_MAX((vec)[1], b); \
	CLAMP_MAX((vec)[2], b); \
	CLAMP_MAX((vec)[3], b); \
} (void)0

/*little macro so inline keyword works*/
#if defined(_MSC_VER)
#  define BLI_INLINE static __forceinline
#else
#  if (defined(__APPLE__) && defined(__ppc__))
/* static inline __attribute__ here breaks osx ppc gcc42 build */
#    define BLI_INLINE static __attribute__((always_inline))
#  else
#    define BLI_INLINE static inline __attribute__((always_inline))
#  endif
#endif

#endif  /* __BLI_UTILDEFINES_H__ */
