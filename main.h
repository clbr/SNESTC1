/*  SNES tile compression
    Copyright (C) 2023 Lauri Kasanen

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MAIN_H
#define MAIN_H

#include <limits.h>
#include <lrtypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void die(const char fmt[], ...);
void tochr(const u8 *in, u8 * const out);

u16 stc1_compress(const u8 *in, u8 *out, const u32 len);
void stc1_decompress(const u8 *in, u8 *out);

enum {
	M_FLAT,
	M_1BIT,
	M_2BIT,
	M_3BIT,
	M_HLINE,
	M_VLINE,
	M_COMMONBYTE,
	M_UNCOMPRESSED,

	NUM_METHODS,
};

extern const char methodstrs[NUM_METHODS][16];

#endif
