#include "colors.h"

void * LilyCoolMalloc(uint size) {
	int lala = (int)malloc(size + 31);
	lala += lala % 32;
	printf("Lily cool malloc to the rescue!!! %d bytes at 0x%x\n", size, lala);
	return (void *)lala;
}

u8 * sqrtlut = NULL;

void gensqrtlut() {
	if(sqrtlut) return;
	sqrtlut = malloc(256);
	for(int i = 0; i < 256; i++) {
		float a = i / 255.0f;
		a = sqrtf(a);
		a *= 255.0f;
		a = (a > 255.0f) ? 255.0f : a;
		sqrtlut[i] = a;
	}
}

u8 u8sqrt(u8 a) {
	return sqrtlut[a];
}
