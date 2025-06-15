#include <crypto/gf128mul.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/ktime.h>

#define gf128mul_dat(q) { \
	q(0x00), q(0x01), q(0x02), q(0x03), q(0x04), q(0x05), q(0x06), q(0x07),\
	q(0x08), q(0x09), q(0x0a), q(0x0b), q(0x0c), q(0x0d), q(0x0e), q(0x0f),\
	q(0x10), q(0x11), q(0x12), q(0x13), q(0x14), q(0x15), q(0x16), q(0x17),\
	q(0x18), q(0x19), q(0x1a), q(0x1b), q(0x1c), q(0x1d), q(0x1e), q(0x1f),\
	q(0x20), q(0x21), q(0x22), q(0x23), q(0x24), q(0x25), q(0x26), q(0x27),\
	q(0x28), q(0x29), q(0x2a), q(0x2b), q(0x2c), q(0x2d), q(0x2e), q(0x2f),\
	q(0x30), q(0x31), q(0x32), q(0x33), q(0x34), q(0x35), q(0x36), q(0x37),\
	q(0x38), q(0x39), q(0x3a), q(0x3b), q(0x3c), q(0x3d), q(0x3e), q(0x3f),\
	q(0x40), q(0x41), q(0x42), q(0x43), q(0x44), q(0x45), q(0x46), q(0x47),\
	q(0x48), q(0x49), q(0x4a), q(0x4b), q(0x4c), q(0x4d), q(0x4e), q(0x4f),\
	q(0x50), q(0x51), q(0x52), q(0x53), q(0x54), q(0x55), q(0x56), q(0x57),\
	q(0x58), q(0x59), q(0x5a), q(0x5b), q(0x5c), q(0x5d), q(0x5e), q(0x5f),\
	q(0x60), q(0x61), q(0x62), q(0x63), q(0x64), q(0x65), q(0x66), q(0x67),\
	q(0x68), q(0x69), q(0x6a), q(0x6b), q(0x6c), q(0x6d), q(0x6e), q(0x6f),\
	q(0x70), q(0x71), q(0x72), q(0x73), q(0x74), q(0x75), q(0x76), q(0x77),\
	q(0x78), q(0x79), q(0x7a), q(0x7b), q(0x7c), q(0x7d), q(0x7e), q(0x7f),\
	q(0x80), q(0x81), q(0x82), q(0x83), q(0x84), q(0x85), q(0x86), q(0x87),\
	q(0x88), q(0x89), q(0x8a), q(0x8b), q(0x8c), q(0x8d), q(0x8e), q(0x8f),\
	q(0x90), q(0x91), q(0x92), q(0x93), q(0x94), q(0x95), q(0x96), q(0x97),\
	q(0x98), q(0x99), q(0x9a), q(0x9b), q(0x9c), q(0x9d), q(0x9e), q(0x9f),\
	q(0xa0), q(0xa1), q(0xa2), q(0xa3), q(0xa4), q(0xa5), q(0xa6), q(0xa7),\
	q(0xa8), q(0xa9), q(0xaa), q(0xab), q(0xac), q(0xad), q(0xae), q(0xaf),\
	q(0xb0), q(0xb1), q(0xb2), q(0xb3), q(0xb4), q(0xb5), q(0xb6), q(0xb7),\
	q(0xb8), q(0xb9), q(0xba), q(0xbb), q(0xbc), q(0xbd), q(0xbe), q(0xbf),\
	q(0xc0), q(0xc1), q(0xc2), q(0xc3), q(0xc4), q(0xc5), q(0xc6), q(0xc7),\
	q(0xc8), q(0xc9), q(0xca), q(0xcb), q(0xcc), q(0xcd), q(0xce), q(0xcf),\
	q(0xd0), q(0xd1), q(0xd2), q(0xd3), q(0xd4), q(0xd5), q(0xd6), q(0xd7),\
	q(0xd8), q(0xd9), q(0xda), q(0xdb), q(0xdc), q(0xdd), q(0xde), q(0xdf),\
	q(0xe0), q(0xe1), q(0xe2), q(0xe3), q(0xe4), q(0xe5), q(0xe6), q(0xe7),\
	q(0xe8), q(0xe9), q(0xea), q(0xeb), q(0xec), q(0xed), q(0xee), q(0xef),\
	q(0xf0), q(0xf1), q(0xf2), q(0xf3), q(0xf4), q(0xf5), q(0xf6), q(0xf7),\
	q(0xf8), q(0xf9), q(0xfa), q(0xfb), q(0xfc), q(0xfd), q(0xfe), q(0xff) \
}

#define xx(p, q)	0x##p##q

#define xda_bbe(i) ( \
	(i & 0x80 ? xx(43, 80) : 0) ^ (i & 0x40 ? xx(21, c0) : 0) ^ \
	(i & 0x20 ? xx(10, e0) : 0) ^ (i & 0x10 ? xx(08, 70) : 0) ^ \
	(i & 0x08 ? xx(04, 38) : 0) ^ (i & 0x04 ? xx(02, 1c) : 0) ^ \
	(i & 0x02 ? xx(01, 0e) : 0) ^ (i & 0x01 ? xx(00, 87) : 0) \
)

#define xda_lle(i) ( \
	(i & 0x80 ? xx(e1, 00) : 0) ^ (i & 0x40 ? xx(70, 80) : 0) ^ \
	(i & 0x20 ? xx(38, 40) : 0) ^ (i & 0x10 ? xx(1c, 20) : 0) ^ \
	(i & 0x08 ? xx(0e, 10) : 0) ^ (i & 0x04 ? xx(07, 08) : 0) ^ \
	(i & 0x02 ? xx(03, 84) : 0) ^ (i & 0x01 ? xx(01, c2) : 0) \
)

static const u16 gf128mul_table_lle[256] = gf128mul_dat(xda_lle);
static const u16 gf128mul_table_bbe[256] = gf128mul_dat(xda_bbe);

static void gf128mul_x_lle(be128 *r, const be128 *x)
{
	u64 a = be64_to_cpu(x->a);
	u64 b = be64_to_cpu(x->b);
	u64 _tt = gf128mul_table_lle[(b << 7) & 0xff];

	r->b = cpu_to_be64((b >> 1) | (a << 63));
	r->a = cpu_to_be64((a >> 1) ^ (_tt << 48));
}

static void gf128mul_x_bbe(be128 *r, const be128 *x)
{
	u64 a = be64_to_cpu(x->a);
	u64 b = be64_to_cpu(x->b);
	u64 _tt = gf128mul_table_bbe[a >> 63];

	r->a = cpu_to_be64((a << 1) | (b >> 63));
	r->b = cpu_to_be64((b << 1) ^ _tt);
}

void gf128mul_x_ble(be128 *r, const be128 *x)
{
	u64 a = le64_to_cpu(x->a);
	u64 b = le64_to_cpu(x->b);
	u64 _tt = gf128mul_table_bbe[b >> 63];

	r->a = cpu_to_le64((a << 1) ^ _tt);
	r->b = cpu_to_le64((b << 1) | (a >> 63));
}
EXPORT_SYMBOL(gf128mul_x_ble);

static void gf128mul_x8_lle(be128 *x)
{
	u64 a = be64_to_cpu(x->a);
	u64 b = be64_to_cpu(x->b);
	u64 _tt = gf128mul_table_lle[b & 0xff];

	x->b = cpu_to_be64((b >> 8) | (a << 56));
	x->a = cpu_to_be64((a >> 8) ^ (_tt << 48));
}

static void gf128mul_x8_bbe(be128 *x)
{
	u64 a = be64_to_cpu(x->a);
	u64 b = be64_to_cpu(x->b);
	u64 _tt = gf128mul_table_bbe[a >> 56];

	x->a = cpu_to_be64((a << 8) | (b >> 56));
	x->b = cpu_to_be64((b << 8) ^ _tt);
}

/* ===== Schoolbook 곱셈기 구현 ===== */
/* 가장 기본적인 다항식 곱셈 방법
 * 128비트를 바이트 단위로 처리하며, 각 바이트의 비트를 하나씩 확인
 * 시간복잡도: O(n²), 공간복잡도: O(1)
 * 구현이 단순하지만 큰 필드에서는 비효율적 */
void gf128mul_lle(be128 *r, const be128 *b)
{
	be128 p[8];
	int i;

	/* r의 각 비트 shift 결과를 미리 계산 (r*x^0 ~ r*x^7) */
	p[0] = *r;
	for (i = 0; i < 7; ++i)
		gf128mul_x_lle(&p[i + 1], &p[i]);

	memset(r, 0, sizeof(*r));
	/* 16바이트를 역순으로 처리 (little-endian) */
	for (i = 0;;) {
		u8 ch = ((u8 *)b)[15 - i];

		/* 각 비트가 1이면 해당하는 shift 결과를 XOR */
		if (ch & 0x80)
			be128_xor(r, r, &p[0]);
		if (ch & 0x40)
			be128_xor(r, r, &p[1]);
		if (ch & 0x20)
			be128_xor(r, r, &p[2]);
		if (ch & 0x10)
			be128_xor(r, r, &p[3]);
		if (ch & 0x08)
			be128_xor(r, r, &p[4]);
		if (ch & 0x04)
			be128_xor(r, r, &p[5]);
		if (ch & 0x02)
			be128_xor(r, r, &p[6]);
		if (ch & 0x01)
			be128_xor(r, r, &p[7]);

		if (++i >= 16)
			break;

		/* 다음 바이트 처리를 위해 x^8 곱셈 */
		gf128mul_x8_lle(r);
	}
}
EXPORT_SYMBOL(gf128mul_lle);

void gf128mul_bbe(be128 *r, const be128 *b)
{
	be128 p[8];
	int i;

	p[0] = *r;
	for (i = 0; i < 7; ++i)
		gf128mul_x_bbe(&p[i + 1], &p[i]);

	memset(r, 0, sizeof(*r));
	for (i = 0;;) {
		u8 ch = ((u8 *)b)[i];

		if (ch & 0x80)
			be128_xor(r, r, &p[7]);
		if (ch & 0x40)
			be128_xor(r, r, &p[6]);
		if (ch & 0x20)
			be128_xor(r, r, &p[5]);
		if (ch & 0x10)
			be128_xor(r, r, &p[4]);
		if (ch & 0x08)
			be128_xor(r, r, &p[3]);
		if (ch & 0x04)
			be128_xor(r, r, &p[2]);
		if (ch & 0x02)
			be128_xor(r, r, &p[1]);
		if (ch & 0x01)
			be128_xor(r, r, &p[0]);

		if (++i >= 16)
			break;

		gf128mul_x8_bbe(r);
	}
}
EXPORT_SYMBOL(gf128mul_bbe);

/* ===== 64KB Look-Up Table 곱셈기 구현 ===== */
/* 64KB 크기의 사전계산 테이블 사용
 * 16개의 서브테이블, 각각 256개 엔트리 (총 4096개의 128비트 값)
 * t[i][j]는 g * x^(8*i) * j를 저장
 * 시간복잡도: O(n), 공간복잡도: O(2^8 * 16) = 64KB
 * 가장 빠르지만 메모리 사용량이 많고 캐시 압박 가능 */
struct gf128mul_64k *gf128mul_init_64k_lle(const be128 *g)
{
	struct gf128mul_64k *t;
	int i, j, k;

	t = kzalloc(sizeof(*t), GFP_KERNEL);
	if (!t)
		goto out;

	/* 16개의 서브테이블 할당 */
	for (i = 0; i < 16; i++) {
		t->t[i] = kzalloc(sizeof(*t->t[i]), GFP_KERNEL);
		if (!t->t[i]) {
			gf128mul_free_64k(t);
			t = NULL;
			goto out;
		}
	}

	/* 첫 번째 서브테이블 초기화: g의 배수들 계산 */
	t->t[0]->t[128] = *g;
	for (j = 64; j > 0; j >>= 1)
		gf128mul_x_lle(&t->t[0]->t[j], &t->t[0]->t[j + j]);

	for (i = 0;;) {
		/* 각 서브테이블의 나머지 엔트리 계산 (XOR 조합) */
		for (j = 2; j < 256; j += j)
			for (k = 1; k < j; ++k)
				be128_xor(&t->t[i]->t[j + k],
					  &t->t[i]->t[j], &t->t[i]->t[k]);

		if (++i >= 16)
			break;

		/* 다음 서브테이블은 이전 테이블에 x^8 곱셈 */
		for (j = 128; j > 0; j >>= 1) {
			t->t[i]->t[j] = t->t[i - 1]->t[j];
			gf128mul_x8_lle(&t->t[i]->t[j]);
		}
	}

out:
	return t;
}
EXPORT_SYMBOL(gf128mul_init_64k_lle);

struct gf128mul_64k *gf128mul_init_64k_bbe(const be128 *g)
{
	struct gf128mul_64k *t;
	int i, j, k;

	t = kzalloc(sizeof(*t), GFP_KERNEL);
	if (!t)
		goto out;

	for (i = 0; i < 16; i++) {
		t->t[i] = kzalloc(sizeof(*t->t[i]), GFP_KERNEL);
		if (!t->t[i]) {
			gf128mul_free_64k(t);
			t = NULL;
			goto out;
		}
	}

	t->t[0]->t[1] = *g;
	for (j = 1; j <= 64; j <<= 1)
		gf128mul_x_bbe(&t->t[0]->t[j + j], &t->t[0]->t[j]);

	for (i = 0;;) {
		for (j = 2; j < 256; j += j)
			for (k = 1; k < j; ++k)
				be128_xor(&t->t[i]->t[j + k],
					  &t->t[i]->t[j], &t->t[i]->t[k]);

		if (++i >= 16)
			break;

		for (j = 128; j > 0; j >>= 1) {
			t->t[i]->t[j] = t->t[i - 1]->t[j];
			gf128mul_x8_bbe(&t->t[i]->t[j]);
		}
	}

out:
	return t;
}
EXPORT_SYMBOL(gf128mul_init_64k_bbe);

void gf128mul_free_64k(struct gf128mul_64k *t)
{
	int i;

	for (i = 0; i < 16; i++)
		kfree(t->t[i]);
	kfree(t);
}
EXPORT_SYMBOL(gf128mul_free_64k);

/* 64KB LUT를 사용한 실제 곱셈 수행
 * 각 바이트에 대해 테이블 조회 후 XOR
 * 총 16번의 테이블 조회로 곱셈 완료 */
void gf128mul_64k_lle(be128 *a, struct gf128mul_64k *t)
{
	u8 *ap = (u8 *)a;
	be128 r[1];
	int i;

	/* 첫 번째 바이트로 초기값 설정 */
	*r = t->t[0]->t[ap[0]];
	/* 나머지 15바이트에 대해 테이블 조회 및 XOR */
	for (i = 1; i < 16; ++i)
		be128_xor(r, r, &t->t[i]->t[ap[i]]);
	*a = *r;
}
EXPORT_SYMBOL(gf128mul_64k_lle);

void gf128mul_64k_bbe(be128 *a, struct gf128mul_64k *t)
{
	u8 *ap = (u8 *)a;
	be128 r[1];
	int i;

	*r = t->t[0]->t[ap[15]];
	for (i = 1; i < 16; ++i)
		be128_xor(r, r, &t->t[i]->t[ap[15 - i]]);
	*a = *r;
}
EXPORT_SYMBOL(gf128mul_64k_bbe);

/* ===== 4KB Look-Up Table 곱셈기 구현 ===== */
/* 4KB 크기의 사전계산 테이블 사용
 * 256개의 128비트 엔트리를 저장 (g의 모든 바이트 배수)
 * 시간복잡도: O(n), 공간복잡도: O(2^8) = 4KB
 * 메모리 효율적이며 L1 캐시에 적합한 크기 */
struct gf128mul_4k *gf128mul_init_4k_lle(const be128 *g)
{
	struct gf128mul_4k *t;
	int j, k;

	t = kzalloc(sizeof(*t), GFP_KERNEL);
	if (!t)
		goto out;

	/* g의 2의 거듭제곱 배수들 계산 (g*128, g*64, ..., g*1) */
	t->t[128] = *g;
	for (j = 64; j > 0; j >>= 1)
		gf128mul_x_lle(&t->t[j], &t->t[j+j]);

	/* 나머지 엔트리는 XOR 조합으로 생성 */
	for (j = 2; j < 256; j += j)
		for (k = 1; k < j; ++k)
			be128_xor(&t->t[j + k], &t->t[j], &t->t[k]);

out:
	return t;
}
EXPORT_SYMBOL(gf128mul_init_4k_lle);

struct gf128mul_4k *gf128mul_init_4k_bbe(const be128 *g)
{
	struct gf128mul_4k *t;
	int j, k;

	t = kzalloc(sizeof(*t), GFP_KERNEL);
	if (!t)
		goto out;

	t->t[1] = *g;
	for (j = 1; j <= 64; j <<= 1)
		gf128mul_x_bbe(&t->t[j + j], &t->t[j]);

	for (j = 2; j < 256; j += j)
		for (k = 1; k < j; ++k)
			be128_xor(&t->t[j + k], &t->t[j], &t->t[k]);

out:
	return t;
}
EXPORT_SYMBOL(gf128mul_init_4k_bbe);

/* 4KB LUT를 사용한 실제 곱셈 수행
 * 최상위 바이트부터 시작하여 누적
 * 각 단계마다 x^8 shift 후 다음 바이트 처리
 * 총 32번의 테이블 조회 (64KB 대비 2배) */
void gf128mul_4k_lle(be128 *a, struct gf128mul_4k *t)
{
	u8 *ap = (u8 *)a;
	be128 r[1];
	int i = 15;

	/* 최상위 바이트로 시작 */
	*r = t->t[ap[15]];
	while (i--) {
		/* x^8 곱셈 후 다음 바이트 처리 */
		gf128mul_x8_lle(r);
		be128_xor(r, r, &t->t[ap[i]]);
	}
	*a = *r;
}
EXPORT_SYMBOL(gf128mul_4k_lle);

void gf128mul_4k_bbe(be128 *a, struct gf128mul_4k *t)
{
	u8 *ap = (u8 *)a;
	be128 r[1];
	int i = 0;

	*r = t->t[ap[0]];
	while (++i < 16) {
		gf128mul_x8_bbe(r);
		be128_xor(r, r, &t->t[ap[i]]);
	}
	*a = *r;
}
EXPORT_SYMBOL(gf128mul_4k_bbe);

/* ===== Karatsuba-Ofman Algorithm (KOA) 곱셈기 구현 ===== */
/* 분할정복 방식으로 곱셈 횟수를 줄이는 알고리즘
 * 128비트 → 64비트 → 32비트로 재귀적 분할
 * 시간복잡도: O(n^1.585), 공간복잡도: O(log n)
 * 이론적으로 우수하나 작은 크기에서는 오버헤드로 인해 느림 */

/* 32비트 carry-less 곱셈 (기본 연산) */
static inline u64 clmul32(u32 a, u32 b)
{
	u64 result = 0;
	u64 temp_a = a;
	int i;
	
	for (i = 0; i < 32; i++) {
		if (b & (1U << i))
			result ^= temp_a;
		temp_a <<= 1;
	}
	
	return result;
}

/* 64비트 Karatsuba 곱셈
 * 입력을 32비트로 분할하여 4회 곱셈을 3회로 축소
 * P0 = AL*BL, P2 = AH*BH, P1 = (AL⊕AH)*(BL⊕BH)⊕P0⊕P2 */
void gf64mul_koa(u64 *r_hi, u64 *r_lo, u64 a, u64 b)
{
	u32 a_hi = a >> 32;
	u32 a_lo = (u32)a;
	u32 b_hi = b >> 32;
	u32 b_lo = (u32)b;
	u64 p0, p1, p2;
	u64 h, l;
	
	p0 = clmul32(a_lo, b_lo);
	p2 = clmul32(a_hi, b_hi);
	p1 = clmul32(a_lo ^ a_hi, b_lo ^ b_hi);
	
	p1 ^= p0 ^ p2;
	
	l = p0 ^ (p1 << 32);
	h = p2 ^ (p1 >> 32);
	
	*r_lo = l;
	*r_hi = h;
}
EXPORT_SYMBOL(gf64mul_koa);

/* 256비트 → 128비트 reduction
 * GF(2^128)의 기약다항식 x^128 + x^7 + x^2 + x + 1 사용
 * 기존 커널의 x^8 shift 함수를 재활용하여 안정성 확보 */
void gf128_reduce_lle_safe(be128 *r,
                                  u64 r3, u64 r2, u64 r1, u64 r0)
{
	int i;
	be128 upper = {
			cpu_to_be64(r3),
			cpu_to_be64(r2)
	};

	r->a = cpu_to_be64(r1);
	r->b = cpu_to_be64(r0);

	for (i = 0; i < 15; i++)
		gf128mul_x8_lle(&upper);
	be128_xor(r, r, &upper);
}
EXPORT_SYMBOL(gf128_reduce_lle_safe);

/* 128비트 Karatsuba 곱셈 (메인 함수)
 * 2단계 분할: 128비트 → 64비트 → 32비트
 * 재귀 대신 명시적 분할로 커널 스택 사용 최소화 */
void gf128mul_koa_lle(be128 *r, const be128 *a, const be128 *b)
{
	u64 a_hi, a_lo, b_hi, b_lo;
	u64 p0_hi, p0_lo, p1_hi, p1_lo, p2_hi, p2_lo;
	u64 r0, r1, r2, r3;
	
	a_hi = be64_to_cpu(a->a);
	a_lo = be64_to_cpu(a->b);
	b_hi = be64_to_cpu(b->a);
	b_lo = be64_to_cpu(b->b);
	
	gf64mul_koa(&p0_hi, &p0_lo, a_lo, b_lo);
	gf64mul_koa(&p2_hi, &p2_lo, a_hi, b_hi);
	gf64mul_koa(&p1_hi, &p1_lo, a_lo ^ a_hi, b_lo ^ b_hi);
	
	p1_hi ^= p0_hi ^ p2_hi;
	p1_lo ^= p0_lo ^ p2_lo;
	
	r0 = p0_lo;
	r1 = p0_hi ^ p1_lo;
	r2 = p1_hi ^ p2_lo;
	r3 = p2_hi;
	
	gf128_reduce_lle_safe(r, r3, r2, r1, r0);
}
EXPORT_SYMBOL(gf128mul_koa_lle);

void gf128_reduce_bbe_safe(be128 *r,
                                  u64 r3, u64 r2, u64 r1, u64 r0)
{
	int i;

	be128 upper = {
			cpu_to_be64(r3),
			cpu_to_be64(r2)
	};

	r->a = cpu_to_be64(r0);
	r->b = cpu_to_be64(r1);

	for (i = 0; i < 15; i++)
		gf128mul_x8_bbe(&upper);
	be128_xor(r, r, &upper);
}
EXPORT_SYMBOL(gf128_reduce_bbe_safe);

void gf128mul_koa_bbe(be128 *r, const be128 *a, const be128 *b)
{
	u64 a_hi, a_lo, b_hi, b_lo;
	u64 p0_hi, p0_lo, p1_hi, p1_lo, p2_hi, p2_lo;
	u64 r0, r1, r2, r3;
	
	a_lo = be64_to_cpu(a->a);
	a_hi = be64_to_cpu(a->b);
	b_lo = be64_to_cpu(b->a);
	b_hi = be64_to_cpu(b->b);
	
	gf64mul_koa(&p0_hi, &p0_lo, a_lo, b_lo);
	gf64mul_koa(&p2_hi, &p2_lo, a_hi, b_hi);
	gf64mul_koa(&p1_hi, &p1_lo, a_lo ^ a_hi, b_lo ^ b_hi);
	
	p1_hi ^= p0_hi ^ p2_hi;
	p1_lo ^= p0_lo ^ p2_lo;
	
	r0 = p0_lo;
	r1 = p0_hi ^ p1_lo;
	r2 = p1_hi ^ p2_lo;
	r3 = p2_hi;
	
	gf128_reduce_bbe_safe(r, r3, r2, r1, r0);
}
EXPORT_SYMBOL(gf128mul_koa_bbe);

/* ===== Hybrid 곱셈기 구현 (LUT + Karatsuba) ===== */
/* 상위 바이트는 LUT, 하위 바이트는 Karatsuba 사용
 * 두 방식의 장점을 결합하려 했으나 실제로는 더 느림 */

/* 4KB Hybrid 곱셈기 초기화 */
struct gf128mul_4k_koa *gf128mul_init_4k_koa_lle(const be128 *g)
{
	struct gf128mul_4k_koa *t;
	int j, k;
	
	t = kzalloc(sizeof(*t), GFP_KERNEL);
	if (!t)
		goto out;
	
	t->t[128] = *g;
	for (j = 64; j > 0; j >>= 1)
		gf128mul_x_lle(&t->t[j], &t->t[j+j]);
	
	for (j = 2; j < 256; j += j)
		for (k = 1; k < j; ++k)
			be128_xor(&t->t[j + k], &t->t[j], &t->t[k]);
	
out:
	return t;
}
EXPORT_SYMBOL(gf128mul_init_4k_koa_lle);

void gf128mul_free_4k_koa(struct gf128mul_4k_koa *t)
{
	kfree(t);
}
EXPORT_SYMBOL(gf128mul_free_4k_koa);

/* 4KB Hybrid 곱셈 수행
 * 상위 8바이트: LUT 사용
 * 하위 8바이트: Karatsuba 사용 */
void gf128mul_4k_koa_lle(be128 *a, struct gf128mul_4k_koa *t)
{
	u8 *ap = (u8 *)a;
	be128 r;
	int i;
	
	r = t->t[ap[15]];
	for (i = 14; i >= 8; i--) {
		gf128mul_x8_lle(&r);
		be128_xor(&r, &r, &t->t[ap[i]]);
	}
	
	if (ap[7] | ap[6] | ap[5] | ap[4] | ap[3] | ap[2] | ap[1] | ap[0]) {
		be128 temp, lower;
		
		lower.a = cpu_to_be64(((u64)ap[7] << 56) | ((u64)ap[6] << 48) |
		                      ((u64)ap[5] << 40) | ((u64)ap[4] << 32) |
		                      ((u64)ap[3] << 24) | ((u64)ap[2] << 16) |
		                      ((u64)ap[1] << 8)  |  (u64)ap[0]);
		lower.b = 0;
		
		temp.a = 0;
		temp.b = r.a;
		r.a = r.b;
		r.b = 0;
		
		gf128mul_koa_lle(&lower, &lower, &t->t[1]);
		
		be128_xor(&r, &r, &temp);
		be128_xor(&r, &r, &lower);
	}
	
	*a = r;
}
EXPORT_SYMBOL(gf128mul_4k_koa_lle);

/* 64KB Hybrid 곱셈기 초기화
 * 상위 8바이트용 테이블만 생성 (32KB) */
struct gf128mul_64k_koa *gf128mul_init_64k_koa_lle(const be128 *g)
{
    struct gf128mul_64k_koa *t;
    int i, j, k;
	be128 g_x64;
    
    t = kzalloc(sizeof(*t), GFP_KERNEL);
    if (!t)
        goto out;
    
    t->g = *g;
    
    for (i = 0; i < 8; i++) {
        t->t[i] = kzalloc(sizeof(*t->t[i]), GFP_KERNEL);
        if (!t->t[i]) {
            gf128mul_free_64k_koa(t);
            t = NULL;
            goto out;
        }
    }
    
    g_x64 = *g;
	for (j = 0; j < 8; j++)
		gf128mul_x8_lle(&g_x64);
    
    t->t[0]->t[128] = g_x64;
    for (j = 64; j > 0; j >>= 1)
        gf128mul_x_lle(&t->t[0]->t[j], &t->t[0]->t[j + j]);
    
    for (i = 0; i < 8; i++) {
        if (i > 0) {
            for (j = 128; j > 0; j >>= 1) {
                t->t[i]->t[j] = t->t[i - 1]->t[j];
                gf128mul_x8_lle(&t->t[i]->t[j]);
            }
        }
        
        for (j = 2; j < 256; j += j)
            for (k = 1; k < j; ++k)
                be128_xor(&t->t[i]->t[j + k],
                          &t->t[i]->t[j], &t->t[i]->t[k]);
    }
    
out:
    return t;
}
EXPORT_SYMBOL(gf128mul_init_64k_koa_lle);

/* 64KB Hybrid 곱셈 수행
 * 상위 8바이트: LUT 사용 (8회 조회)
 * 하위 8바이트: Karatsuba 사용 */
void gf128mul_64k_koa_lle(be128 *a, struct gf128mul_64k_koa *t)
{
    u8 *ap = (u8 *)a;
    be128 r_upper = {0, 0};
    be128 r_lower;
    be128 shifted;
    int i;
    
    for (i = 0; i < 8; i++)
        be128_xor(&r_upper, &r_upper, &t->t[i]->t[ap[8 + i]]);
    
    r_lower.a = 0;
    r_lower.b = cpu_to_be64(((u64)ap[7] << 56) | ((u64)ap[6] << 48) |
                            ((u64)ap[5] << 40) | ((u64)ap[4] << 32) |
                            ((u64)ap[3] << 24) | ((u64)ap[2] << 16) |
                            ((u64)ap[1] << 8)  |  (u64)ap[0]);
    
    if (r_lower.b != 0) {
        gf128mul_koa_lle(&r_lower, &r_lower, &t->g);
    }
    
    shifted.a = r_upper.b;
    shifted.b = 0;
    
    *a = r_lower;
    be128_xor(a, a, &shifted);
    
    if (r_upper.a) {
        int j;
        be128 tmp = { .a = r_upper.a, .b = 0 };
        for (j = 0; j < 15; j++)
            gf128mul_x8_lle(&tmp);
        be128_xor(a, a, &tmp);
    }
}
EXPORT_SYMBOL(gf128mul_64k_koa_lle);

void gf128mul_free_64k_koa(struct gf128mul_64k_koa *t)
{
    int i;
    for (i = 0; i < 8; i++)
        kfree(t->t[i]);
    kfree(t);
}
EXPORT_SYMBOL(gf128mul_free_64k_koa);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Functions for multiplying elements of GF(2^128)");
