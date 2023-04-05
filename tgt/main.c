#include <stdio.h>

void stc1_decompress(const unsigned char *in, unsigned char *out);
unsigned char src[22 * 1024], dst[32 * 1024];

int main() {
	int a;
	unsigned int cnt;

	while (1) {
		a = getc(stdin);
		if (a == EOF)
			break;
		src[cnt++] = a;
	}

	stc1_decompress(src, dst);

	cnt = src[0] | src[1] << 8;
	fwrite(dst, 1, cnt, stdout);

	return 0;
}
