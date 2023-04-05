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

#include "main.h"

const char methodstrs[NUM_METHODS][16] = {
	"flat",
	"1bit",
	"2bit",
	"3bit",
	"rle",
	"hline",
	"vline",
	"commonbyte",
	"ancestor",
	"uncompressed"
};

void tochr(const u8 *in, u8 * const out) {
	u8 x, y;
	memset(out, 0, 32);

	for (y = 0; y < 8; y++) {
		for (x = 0; x < 8; x++) {
			const u8 pix = *in++;
			if (pix & 1)
				out[y * 2] |= 1 << (7 - x);
			if (pix & 2)
				out[y * 2 + 1] |= 1 << (7 - x);
			if (pix & 4)
				out[16 + y * 2] |= 1 << (7 - x);
			if (pix & 8)
				out[16 + y * 2 + 1] |= 1 << (7 - x);
		}
	}
}

static u8 comp_flat(const u8 *in, u8 *out) {
	*out++ = M_FLAT | in[0] << 4;

	return 1;
}

static u8 comp_1bit(const u8 *in, u8 *out, const u8 cols[]) {
	u8 i, flood = 0xff, other = 0xff, highest = 0;

	for (i = 0; i < 16; i++) {
		if (!cols[i])
			continue;
		if (cols[i] > highest) {
			flood = i;
			highest = cols[i];
		}
	}

	for (i = 0; i < 16; i++) {
		if (!cols[i])
			continue;
		if (i == flood)
			continue;
		other = i;
	}

	if (flood >= 16 || other >= 16)
		die("1bit bug\n");

	*out++ = M_1BIT;
	*out++ = flood | other << 4;

	for (i = 0; i < 8; i++, in += 8) {
		u8 val = 0, j;
		for (j = 0; j < 8; j++) {
			if (in[j] == other)
				val |= 1 << j;
		}
		*out++ = val;
	}

	return 10;
}

static u8 comp_2bit(const u8 *in, u8 *out, const u8 cols[]) {
	u8 i, vals[4];
	u16 used = 0;

	memset(vals, 0xff, 4);

	u8 curval = 0;
	for (i = 0; i < 16; i++) {
		if (cols[i]) {
			used |= 1 << i;
			vals[curval++] = i;
		}
	}

	if (curval > 4)
		die("2bit bug");

	*out++ = M_2BIT;
	*out++ = used;
	*out++ = used >> 8;

	for (i = 0; i < 8; i++, in += 8) {
		u16 val = 0;
		u8 j;
		for (j = 0; j < 8; j++) {
			u8 k;
			for (k = 0; k < 4; k++) {
				if (in[j] == vals[k]) {
					val |= k << j * 2;
					break;
				}
			}
		}
		*out++ = val;
		*out++ = val >> 8;
	}

	return 19;
}

static u8 comp_3bit(const u8 *in, u8 *out, const u8 cols[]) {
	u8 i, vals[8];
	u16 used = 0;

	memset(vals, 0xff, 8);

	u8 curval = 0;
	for (i = 0; i < 16; i++) {
		if (cols[i]) {
			used |= 1 << i;
			vals[curval++] = i;
		}
	}

	if (curval > 8)
		die("3bit bug");

	*out++ = M_3BIT;
	*out++ = used;
	*out++ = used >> 8;

	for (i = 0; i < 8; i++, in += 8) {
		u32 val = 0;
		u8 j;
		for (j = 0; j < 8; j++) {
			u8 k;
			for (k = 0; k < 8; k++) {
				if (in[j] == vals[k]) {
					val |= k << j * 3;
					break;
				}
			}
		}
		*out++ = val;
		*out++ = val >> 8;
		*out++ = val >> 16;
	}

	return 27;
}

static u8 writerun(const u8 val, u8 run, u8 *buf, u8 *n, u8 *out) {
	#define put(arg) \
		if (*n) { \
			*out++ = *buf | arg << 4; \
			*n = 0; \
		} else { \
			*buf = arg; \
			*n = 1; \
		}

	if (run == 1) {
		put(val);
		if (!*n)
			return 1;
		return 0;
	}

	const u8 * const origout = out;

	put(val);
	put(val);

	run -= 2;

	while (1) {
		if (run >= 15) {
			put(15);
			run -= 15;
		} else {
			put(run);
			break;
		}
	}

	#undef put

	return out - origout;
}

static u8 comp_rle(const u8 *in, u8 *out) {
	const u8 * const origout = out;
	u8 i;
	u8 prev = in[0], run = 1;
	u8 outbuf = 0, n = 0;
	*out++ = M_RLE;

	for (i = 1; i < 64; i++) {
		if (in[i] != prev) {
			out += writerun(prev, run, &outbuf, &n, out);
			if (out - origout >= 32)
				return 0xff;

			prev = in[i];
			run = 1;
		} else {
			run++;
		}

		if (i == 63) {
			out += writerun(prev, run, &outbuf, &n, out);
			if (out - origout >= 32)
				return 0xff;
		}
	}

	if (n)
		*out++ = outbuf;

	return out - origout;
}

static u8 comp_line_inner(u8 *out, u32 lines[]) {

	u8 i, j, used[8] = { 0 }, highest = 0xff, highestcount = 0;

	for (i = 0; i < 8; i++) {
		for (j = 0; j < 8; j++) {
			if (lines[i] == lines[j]) {
				used[j]++;
				break;
			}
		}
	}

	for (i = 0; i < 8; i++) {
		if (used[i] > highestcount) {
			highest = i;
			highestcount = used[i];
		}
	}

	if (highestcount == 1)
		return 0xff; // not compressible

	u8 * const valpos = out + 1;
	out += 2;
	memcpy(out, &lines[highest], 4);
	out += 4;

	u8 val = 0;
	for (i = 0; i < 8; i++) {
		if (lines[i] == lines[highest]) {
			val |= 1 << i;
		} else {
			memcpy(out, &lines[i], 4);
			out += 4;
		}
	}

	*valpos = val;

	return 6 + (8 - highestcount) * 4;
}

static u8 comp_hline_inner(const u8 *in, u8 *out) {
	u32 lines[8] = { 0 };
	u8 i, j;

	for (i = 0; i < 8; i++) {
		for (j = 0; j < 8; j++)
			lines[i] |= in[i * 8 + j] << 4 * j;
	}

	return comp_line_inner(out, lines);
}

static u8 comp_vline_inner(const u8 *in, u8 *out) {
	u32 lines[8] = { 0 };
	u8 i, j;

	for (i = 0; i < 8; i++) {
		for (j = 0; j < 8; j++)
			lines[i] |= in[i + j * 8] << 4 * j;
	}

	return comp_line_inner(out, lines);
}

static u8 comp_hline(const u8 *origin, u8 *out) {
	u8 twice[128], r, comps[16][32];
	memcpy(twice, origin, 64);
	memcpy(twice + 64, origin, 64);

	// Rotations
	u8 best = 0xff, bestlen = 0xff;
	for (r = 0; r < 16; r++) {
		const u8 len = comp_hline_inner(twice + r, comps[r]);
		comps[r][0] = M_HLINE | r << 4;
		if (len < bestlen) {
			bestlen = len;
			best = r;
		}
		//printf("\tHline rotation %u gave %u\n", r, len);
	}

	memcpy(out, comps[best], bestlen);
	return bestlen;
}

static u8 comp_vline(const u8 *origin, u8 *out) {
	u8 twice[128], r, comps[16][32];
	memcpy(twice, origin, 64);
	memcpy(twice + 64, origin, 64);

	// Rotations
	u8 best = 0xff, bestlen = 0xff;
	for (r = 0; r < 16; r++) {
		const u8 len = comp_vline_inner(twice + r, comps[r]);
		comps[r][0] = M_VLINE | r << 4;
		if (len < bestlen) {
			bestlen = len;
			best = r;
		}
		//printf("\tVline rotation %u gave %u\n", r, len);
	}

	memcpy(out, comps[best], bestlen);
	return bestlen;
}

static u8 comp_commonbyte(const u8 chr[], u8 *out) {
	u16 i;
	u8 used[256] = { 0 }, highest = 0, highestcount = 0;
	for (i = 0; i < 32; i++) {
		used[chr[i]]++;
	}

	for (i = 0; i < 256; i++) {
		if (used[i] > highestcount) {
			highest = i;
			highestcount = used[i];
		}
	}

	if (highestcount < 6)
		return 0xff; // not compressible

	*out++ = M_COMMONBYTE;
	*out++ = highest;

	u8 *valpos = out;
	out += 4;
	u32 val = 0;
	for (i = 0; i < 32; i++) {
		if (chr[i] == highest)
			val |= 1 << i;
		else
			*out++ = chr[i];
	}
	memcpy(valpos, &val, 4);

	return 6 + 32 - highestcount;
}

static u8 comp_ancestor(const u16 t, const u8 chr[], u8 *out) {

	const u8 * const orig = out;
	u16 i;
	u16 best = 0xffff;
	u32 bestval = 0;
	u8 bestcount = 0;
	const u8 * const me = &chr[t * 32];
	u8 b, matches;
	u32 val;

	for (i = 0; i < t; i++) {
		matches = val = 0;
		for (b = 0; b < 32; b++) {
			if (chr[i * 32 + b] == me[b]) {
				matches++;
			} else {
				val |= 1 << b;
			}
		}

		if (matches > bestcount) {
			bestcount = matches;
			best = i;
			bestval = val;
		}
	}

	if (bestcount < 5)
		return 0xff; // not compressible

	u8 summary = 0;
	if (bestval & 0xff)
		summary |= 1;
	if (bestval & 0xff00)
		summary |= 2;
	if (bestval & 0xff0000)
		summary |= 4;
	if (bestval & 0xff000000)
		summary |= 8;

	*out++ = M_ANCESTOR | summary << 4;

	u16 dist = t - best - 1;
	while (dist >= 255) {
		*out++ = 255;
		dist -= 255;
	}
	*out++ = dist;

	if (summary & 1)
		*out++ = bestval;
	if (summary & 2)
		*out++ = bestval >> 8;
	if (summary & 4)
		*out++ = bestval >> 16;
	if (summary & 8)
		*out++ = bestval >> 24;

	for (b = 0; b < 32; b++) {
		if (chr[best * 32 + b] != me[b])
			*out++ = me[b];
	}

	if (out - orig >= 32)
		return 0xff;

	return out - orig;
}

static u8 comp_uncompressed(const u8 chr[], u8 *out) {
	*out++ = M_UNCOMPRESSED;
	memcpy(out, chr, 32);
	return 33;
}

static u8 countcolors(const u8 *in, u8 *cols) {
	u8 i;
	for (i = 0; i < 64; i++) {
		if (in[i] >= 16)
			die("Over 16 colors\n");

		cols[in[i]]++;
	}

	u8 sum = 0;
	for (i = 0; i < 16; i++) {
		if (cols[i])
			sum++;
	}

	return sum;
}

u16 stc1_compress(const u8 *in, const u8 *chr, u8 *out, const u32 len) {
	const u8 * const orig = out;
	u8 comps[NUM_METHODS][33], complens[NUM_METHODS];

	if (len % 64)
		die("Invalid input data\n");

	const u16 numtiles = len / 64;
	memcpy(out, &numtiles, 2);
	out += 2;

	u16 t;
	for (t = 0; t < numtiles; t++, in += 64) {

		u8 cols[16] = { 0 };
		const u8 numcols = countcolors(in, cols);
		memset(complens, 0xff, NUM_METHODS);

		if (numcols == 1)
			complens[M_FLAT] = comp_flat(in, comps[M_FLAT]);
		else if (numcols == 2)
			complens[M_1BIT] = comp_1bit(in, comps[M_1BIT], cols);
		else if (numcols <= 4)
			complens[M_2BIT] = comp_2bit(in, comps[M_2BIT], cols);
		else if (numcols <= 8)
			complens[M_3BIT] = comp_3bit(in, comps[M_3BIT], cols);
		complens[M_RLE] = comp_rle(in, comps[M_RLE]);
		complens[M_HLINE] = comp_hline(in, comps[M_HLINE]);
		complens[M_VLINE] = comp_vline(in, comps[M_VLINE]);

		if (t)
			complens[M_ANCESTOR] = comp_ancestor(t, chr, comps[M_ANCESTOR]);

		if (numcols > 8) {
			complens[M_COMMONBYTE] =
				comp_commonbyte(chr + t * 32, comps[M_COMMONBYTE]);
			complens[M_UNCOMPRESSED] =
				comp_uncompressed(chr + t * 32, comps[M_UNCOMPRESSED]);
		}

		u8 i;
		u8 best = 0xff, bestlen = 0xff;
		for (i = 0; i < NUM_METHODS; i++) {
			if (complens[i] < bestlen) {
				bestlen = complens[i];
				best = i;
			}
		}

		printf("Tile %u: %u colors, method %s was best (%u)\n",
			t, numcols, methodstrs[best], bestlen);

		memcpy(out, comps[best], bestlen);
		out += bestlen;
	}

	return out - orig;
}
