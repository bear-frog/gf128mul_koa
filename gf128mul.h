#ifndef _CRYPTO_GF128MUL_H
#define _CRYPTO_GF128MUL_H

#include <crypto/b128ops.h>
#include <linux/slab.h>

/* ===== GF(2^128) 곱셈 함수 헤더 파일 =====
 * 
 * GF(2^128) = GF(2)[X]/(X^128-X^7-X^2-X^1-1)
 * 기약다항식: x^128 + x^7 + x^2 + x + 1
 * 
 * 128비트 유한체 연산을 위한 다양한 곱셈기 구현
 * - Schoolbook: 기본 다항식 곱셈
 * - LUT: Look-Up Table 기반 고속 곱셈
 * - KOA: Karatsuba-Ofman Algorithm
 * - Hybrid: LUT + KOA 결합
 */

/* ===== 엔디안 형식 설명 =====
 * 
 * GF(2^128)의 원소는 128비트로 표현되며, 메모리 저장 방식에 따라 구분:
 * 
 * 1. lle (little-little-endian)
 *    - 비트: little-endian (LSB가 낮은 주소)
 *    - 바이트: little-endian (낮은 바이트가 낮은 주소)
 *    - GCM, ABL에서 사용
 * 
 * 2. bbe (big-big-endian)
 *    - 비트: big-endian (MSB가 낮은 주소)
 *    - 바이트: big-endian (높은 바이트가 낮은 주소)
 *    - LRW에서 사용
 * 
 * 3. ble (big-little-endian)
 *    - 비트: big-endian
 *    - 바이트: little-endian
 *    - EME에서 사용 (특허 문제)
 */

/* ===== Schoolbook 곱셈기 =====
 * 가장 기본적인 다항식 곱셈 구현
 * 시간복잡도: O(n²)
 * 메모리: 추가 할당 없음
 */
void gf128mul_lle(be128 *a, const be128 *b);
void gf128mul_bbe(be128 *a, const be128 *b);

/* XTS 모드용 x 곱셈 (ble 형식) */
void gf128mul_x_ble(be128 *a, const be128 *b);

/* ===== 4KB Look-Up Table 곱셈기 =====
 * 256개 엔트리 (각 128비트)를 가진 테이블 사용
 * 시간복잡도: O(n)
 * 메모리: 4KB
 * L1 캐시에 적합한 크기
 */
struct gf128mul_4k {
	be128 t[256];  /* g의 모든 바이트 배수 저장 */
};

struct gf128mul_4k *gf128mul_init_4k_lle(const be128 *g);
struct gf128mul_4k *gf128mul_init_4k_bbe(const be128 *g);
void gf128mul_4k_lle(be128 *a, struct gf128mul_4k *t);
void gf128mul_4k_bbe(be128 *a, struct gf128mul_4k *t);

static inline void gf128mul_free_4k(struct gf128mul_4k *t)
{
	kfree(t);
}

/* ===== 64KB Look-Up Table 곱셈기 =====
 * 16개의 4KB 서브테이블로 구성
 * 시간복잡도: O(n), 4KB보다 2배 빠름
 * 메모리: 64KB
 * 캐시 압박 가능성 있음
 */
struct gf128mul_64k {
	struct gf128mul_4k *t[16];  /* t[i]는 g*x^(8*i)의 배수들 */
};

struct gf128mul_64k *gf128mul_init_64k_lle(const be128 *g);
struct gf128mul_64k *gf128mul_init_64k_bbe(const be128 *g);
void gf128mul_free_64k(struct gf128mul_64k *t);
void gf128mul_64k_lle(be128 *a, struct gf128mul_64k *t);
void gf128mul_64k_bbe(be128 *a, struct gf128mul_64k *t);

/* ===== Karatsuba-Ofman Algorithm (KOA) 곱셈기 =====
 * 분할정복 방식으로 곱셈 횟수 감소
 * 시간복잡도: O(n^1.585)
 * 재귀 대신 2단계 명시적 분할 사용
 */

/* 64비트 Karatsuba 곱셈 (내부 헬퍼) */
void gf64mul_koa(u64 *r_hi, u64 *r_lo, u64 a, u64 b);

/* 256비트 → 128비트 reduction 함수 */
void gf128_reduce_lle_safe(be128 *r, u64 r3, u64 r2, u64 r1, u64 r0);
void gf128_reduce_bbe_safe(be128 *r, u64 r3, u64 r2, u64 r1, u64 r0);

/* 128비트 Karatsuba 곱셈 (메인 함수) */
void gf128mul_koa_lle(be128 *r, const be128 *a, const be128 *b);
void gf128mul_koa_bbe(be128 *r, const be128 *a, const be128 *b);

/* ===== Hybrid 곱셈기 (LUT + KOA) =====
 * 상위 바이트: LUT 사용
 * 하위 바이트: Karatsuba 사용
 * 두 방식의 장점 결합 시도
 */

/* 4KB Hybrid 구조체 (캐시 정렬 포함) */
struct gf128mul_4k_koa {
	be128 t[256];
	u8 pad[0] __attribute__((aligned(64)));  /* 캐시라인 정렬 */
};

struct gf128mul_4k_koa *gf128mul_init_4k_koa_lle(const be128 *g);
void gf128mul_free_4k_koa(struct gf128mul_4k_koa *t);
void gf128mul_4k_koa_lle(be128 *a, struct gf128mul_4k_koa *t);

/* 64KB Hybrid 구조체
 * 상위 8바이트용 테이블 + KOA용 키 저장
 */
struct gf128mul_64k_koa {
    struct gf128mul_4k *t[8];  /* 상위 8바이트용 (32KB) */
    be128 g;                   /* KOA용 원본 키 */
};

struct gf128mul_64k_koa *gf128mul_init_64k_koa_lle(const be128 *g);
void gf128mul_free_64k_koa(struct gf128mul_64k_koa *t);
void gf128mul_64k_koa_lle(be128 *a, struct gf128mul_64k_koa *t);

#endif /* _CRYPTO_GF128MUL_H */
