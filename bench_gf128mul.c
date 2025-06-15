#include <linux/module.h>
#include <linux/init.h>
#include <linux/ktime.h>
#include <linux/printk.h>
#include <crypto/gf128mul.h>   /* be128, bench_* 프로토타입 */

#define GF128_BENCH_ROUNDS   1000000UL     /* 1 M iterations */

/* ===== GF(2^128) 곱셈기 마이크로 벤치마크 =====
 * 
 * 각 곱셈기의 순수 연산 성능을 측정하기 위한 커널 모듈
 * 1백만 회 반복 수행하여 평균 성능을 계산
 * 
 * 측정 대상:
 * 1. Schoolbook - 기본 구현
 * 2. KOA - Karatsuba 알고리즘
 * 3. LUT-4KB - 4KB 테이블
 * 4. LUT-64KB - 64KB 테이블
 * 5. Hybrid-4KB - 4KB LUT + KOA
 * 6. Hybrid-64KB - 64KB LUT + KOA
 */

/* ── Benchmark wrappers ───────────────────────────────────────────────────────── */

/* Schoolbook 곱셈기 벤치마크
 * - noinline: 컴파일러 인라인 최적화 방지
 * - 매 반복마다 입력값 복사하여 실제 사용 패턴 모방
 * - in-place 연산 수행
 */
static noinline void bench_school_lle(const be128 *key, const be128 *in,
                                      unsigned long loops)
{
    be128 tmp;
    unsigned long i;
    for (i = 0; i < loops; i++) {
        tmp = *in;
        gf128mul_lle(&tmp, key);
    }
}

/* Karatsuba 곱셈기 벤치마크
 * - 3개 인자 인터페이스 (결과, 입력1, 입력2)
 * - 동일한 버퍼를 입력과 출력으로 사용
 */
static noinline void bench_koa_lle(const be128 *key, const be128 *in,
                                   unsigned long loops)
{
    be128 tmp;
    unsigned long i;
    for (i = 0; i < loops; i++) {
        tmp = *in;
        gf128mul_koa_lle(&tmp, &tmp, key);
    }
}

/* 4KB LUT 곱셈기 벤치마크
 * - static 변수로 테이블 한 번만 초기화
 * - unlikely(): 첫 호출 시에만 초기화 수행
 * - 테이블 초기화 시간은 측정에서 제외
 */
static noinline void bench_lut4k_lle(const be128 *key, const be128 *in,
                                     unsigned long loops)
{
    static struct gf128mul_4k *tbl;
    be128 tmp;
    unsigned long i;
    if (unlikely(!tbl))
        tbl = gf128mul_init_4k_lle(key);
    for (i = 0; i < loops; i++) {
        tmp = *in;
        gf128mul_4k_lle(&tmp, tbl);
    }
}

/* 64KB LUT 곱셈기 벤치마크
 * - 매번 테이블 할당/해제하여 실제 사용 패턴 반영
 * - 초기화 오버헤드 포함된 측정
 */
static noinline void bench_lut64k_lle(const be128 *key, const be128 *in,
                                      unsigned long loops)
{
    struct gf128mul_64k *tbl = gf128mul_init_64k_lle(key);
    be128 tmp;
    unsigned long i;
    for (i = 0; i < loops; i++) {
        tmp = *in;
        gf128mul_64k_lle(&tmp, tbl);
    }

	gf128mul_free_64k(tbl);
}

/* 4KB Hybrid (LUT+KOA) 곱셈기 벤치마크
 * - 상위 8바이트는 LUT, 하위 8바이트는 KOA
 * - 4KB 테이블만 사용하여 메모리 절약
 */
static noinline void bench_hybrid4k_koa_lle(const be128 *key, const be128 *in,
                                            unsigned long loops)
{
    static struct gf128mul_4k_koa *tbl;
    be128 tmp;
    unsigned long i;
    if (unlikely(!tbl))
        tbl = gf128mul_init_4k_koa_lle(key);
    for (i = 0; i < loops; i++) {
        tmp = *in;
        gf128mul_4k_koa_lle(&tmp, tbl);
    }
}

/* 64KB Hybrid (LUT+KOA) 곱셈기 벤치마크
 * - 상위 8바이트용 32KB 테이블 + KOA
 * - 두 방식의 장점 결합 시도
 */
static noinline void bench_hybrid64k_koa_lle(const be128 *key, const be128 *in,
                                             unsigned long loops)
{
    static struct gf128mul_64k_koa *tbl;
    be128 tmp;
    unsigned long i;
    if (unlikely(!tbl))
        tbl = gf128mul_init_64k_koa_lle(key);
    for (i = 0; i < loops; i++) {
        tmp = *in;
        gf128mul_64k_koa_lle(&tmp, tbl);
    }
}

/* ── Benchmark entry point ──────────────────────────────────────────────── */

/* 모듈 초기화 함수
 * - 모듈 로드 시 자동으로 벤치마크 실행
 * - 고정된 테스트 벡터 사용으로 일관성 확보
 * - do_gettimeofday()로 마이크로초 단위 측정
 */
static int __init gf128mul_bench_init(void)
{
    /* 테스트용 128비트 키와 메시지
     * - 다양한 비트 패턴 포함
     * - 모든 벤치마크에서 동일한 값 사용
     */
    const be128 key = {
        cpu_to_be64(0x0123456789abcdefULL),
        cpu_to_be64(0xfedcba9876543210ULL)
    };
    const be128 msg = {
        cpu_to_be64(0x0f1e2d3c4b5a6978ULL),
        cpu_to_be64(0x8070605040302010ULL)
    };
    struct timeval t0, t1;
    u64 ns_total;

    pr_info("gf128mul-bench: Starting benchmark with %lu iterations\n", 
            GF128_BENCH_ROUNDS);

    /* 1) schoolbook */
    do_gettimeofday(&t0);
    bench_school_lle(&key, &msg, GF128_BENCH_ROUNDS);
    do_gettimeofday(&t1);
    /* 마이크로초를 나노초로 변환하여 정밀도 향상 */
    ns_total = (t1.tv_sec - t0.tv_sec) * 1000000000ULL +
               (t1.tv_usec - t0.tv_usec) * 1000ULL;
    pr_info("gf128mul-bench: school       %llu ns  (%llu ns/op)\n",
            ns_total, ns_total / GF128_BENCH_ROUNDS);

    /* 2) KOA */
    do_gettimeofday(&t0);
    bench_koa_lle(&key, &msg, GF128_BENCH_ROUNDS);
    do_gettimeofday(&t1);
    ns_total = (t1.tv_sec - t0.tv_sec) * 1000000000ULL +
               (t1.tv_usec - t0.tv_usec) * 1000ULL;
    pr_info("gf128mul-bench: koa          %llu ns  (%llu ns/op)\n",
            ns_total, ns_total / GF128_BENCH_ROUNDS);

    /* 3) LUT-4k */
    do_gettimeofday(&t0);
    bench_lut4k_lle(&key, &msg, GF128_BENCH_ROUNDS);
    do_gettimeofday(&t1);
    ns_total = (t1.tv_sec - t0.tv_sec) * 1000000000ULL +
               (t1.tv_usec - t0.tv_usec) * 1000ULL;
    pr_info("gf128mul-bench: lut-4k       %llu ns  (%llu ns/op)\n",
            ns_total, ns_total / GF128_BENCH_ROUNDS);

    /* 4) LUT-64k */
    do_gettimeofday(&t0);
    bench_lut64k_lle(&key, &msg, GF128_BENCH_ROUNDS);
    do_gettimeofday(&t1);
    ns_total = (t1.tv_sec - t0.tv_sec) * 1000000000ULL +
               (t1.tv_usec - t0.tv_usec) * 1000ULL;
    pr_info("gf128mul-bench: lut-64k      %llu ns  (%llu ns/op)\n",
            ns_total, ns_total / GF128_BENCH_ROUNDS);

    /* 5) Hybrid 4K+KOA */
    do_gettimeofday(&t0);
    bench_hybrid4k_koa_lle(&key, &msg, GF128_BENCH_ROUNDS);
    do_gettimeofday(&t1);
    ns_total = (t1.tv_sec - t0.tv_sec) * 1000000000ULL +
               (t1.tv_usec - t0.tv_usec) * 1000ULL;
    pr_info("gf128mul-bench: hybrid-4k    %llu ns  (%llu ns/op)\n",
            ns_total, ns_total / GF128_BENCH_ROUNDS);

    /* 6) Hybrid 64K+KOA */
    do_gettimeofday(&t0);
    bench_hybrid64k_koa_lle(&key, &msg, GF128_BENCH_ROUNDS);
    do_gettimeofday(&t1);
    ns_total = (t1.tv_sec - t0.tv_sec) * 1000000000ULL +
               (t1.tv_usec - t0.tv_usec) * 1000ULL;
    pr_info("gf128mul-bench: hybrid-64k   %llu ns  (%llu ns/op)\n",
            ns_total, ns_total / GF128_BENCH_ROUNDS);

    return 0;
}

/* 모듈 종료 함수
 * - 특별한 정리 작업 없음
 * - static 테이블들은 커널이 자동으로 해제
 */
static void __exit gf128mul_bench_exit(void)
{
    pr_info("gf128mul-bench: unloaded\n");
}

module_init(gf128mul_bench_init);
module_exit(gf128mul_bench_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Out-of-tree GF128mul microbenchmark");
