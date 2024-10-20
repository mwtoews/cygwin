/*
  (C) Copyright 2001,2006,
  International Business Machines Corporation,
  Sony Computer Entertainment, Incorporated,
  Toshiba Corporation,

  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
    * Neither the names of the copyright holders nor the names of their
  contributors may be used to endorse or promote products derived from this
  software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/
#include <spu_intrinsics.h>
#include <stddef.h>
#include "vec_literal.h"

/* Locates the first occurrence in string pointed to by s1 of
 * any character in the string pointed to by s2. A null pointer
 * is returned if no character in s2 occurs in s1.
 */
char * strpbrk(const char *s1, const char *s2)
{
  unsigned int offset;
  vec_uint4 cnt;
  vec_uchar16 shuffle, match, initial_splat, splat, eos;
  vec_uchar16 data1, data2, dataA, dataB, *ptr1, *ptr2;

  ptr1 = (vec_uchar16 *)s1;

  offset = (unsigned int)(s1) & 15;
  shuffle = (vec_uchar16)spu_add((vec_uint4)spu_splats((unsigned char) offset),
				 VEC_LITERAL(vec_uint4, 0x0010203, 0x04050607, 0x08090A0B, 0x0C0D0E0F));

  dataA = *ptr1++;
  dataB = *ptr1++;

  initial_splat = spu_splats((unsigned char)((unsigned int)(s2) & 0xF));

  /* For each quadword of the string s1.
   */
  do {
    data1 = spu_shuffle(dataA, dataB, shuffle);

    eos = match = spu_cmpeq(data1, 0);

    ptr2 = (vec_uchar16 *)s2;
    data2 = *ptr2;
    data2 = spu_shuffle(data2, data2, initial_splat);
    ptr2 = (vec_uchar16 *)((unsigned int)(ptr2) + 1);
    splat = initial_splat;

    /* For each character of s2, compare against a quadword of s1,
     * accumulating match success in the variable match.
     */
    while (spu_extract((vec_uint4)data2, 0)) {
      match = spu_or(match, spu_cmpeq(data1, data2));

      splat = spu_and((vec_uchar16)(spu_add((vec_uint4)splat, VEC_SPLAT_U32(0x01010101))), 0xF);

      data2 = *ptr2;
      data2 = spu_shuffle(data2, data2, splat);
      ptr2 = (vec_uchar16 *)((unsigned int)(ptr2) + 1);
    }

    cnt = spu_cntlz(spu_gather(match));

    dataA = dataB;
    dataB = *ptr1++;
  } while (spu_extract(cnt, 0) == 32);

  /* Compute the first match pointer, zeroing it (NIL) if it is the end of
   * string.
   */
  return ((char *)spu_extract(spu_andc(spu_add(spu_add(spu_promote((unsigned int)(ptr1), 0), -64), cnt),
				       spu_cmpeq(cnt, spu_cntlz(spu_gather(eos)))), 0));
}
