#pragma once
#include "encoding.h"
#include <stdint.h>
#include <stddef.h>

#if 0

#define PULP_NOINLINE __attribute__ ((noinline))

extern char l1_alloc_base;
extern uint32_t atomic_barrier;
extern uint32_t wake_up_reg;

extern char l2_base;
extern char l2_end;

typedef uint32_t pulp_id_t;
typedef uint32_t pulp_timer_t;

/// Obtain the number of cores in the current cluster.
static inline pulp_id_t pulp_get_core_count() {
    extern uint32_t nr_cores_address_reg;
	return nr_cores_address_reg;
}

/// Obtain the ID of the current core.
static inline pulp_id_t pulp_get_core_id() {
	pulp_id_t r;
	asm volatile ("csrr %0, mhartid" : "=r"(r));
    // Hart of snitch starts at 0 (avispado before)
	return r-0x10;
}

/// Obtain a monotonically increasing cycle count.
static inline pulp_timer_t pulp_get_timer() {
	return read_csr(mcycle);
}

/// Synchronize the integer and float pipelines. A no-op on non-F/D cores.
static inline void fpu_fence() {
    if (read_csr(misa) & (1 << 5)) {
        uint32_t tmp;
        asm volatile (
            "fmv.x.w %[tmp], fa0 \n"
            "mv zero, %[tmp] \n"
            : [tmp] "=r"(tmp)
            :: "memory"
        );
    }
}

/// A cluster-local barrier.
static inline void pulp_barrier() {
    // // The following is a software-only barrier using AMOs.
    // uint32_t core_id = pulp_get_core_id();
    // uint32_t core_count = pulp_get_core_count();
    // uint32_t mask = 1 << core_id;
    // uint32_t others = ((1 << core_count) - 1) ^ mask;
    // if (core_id == 0) {
    //     while ((__atomic_load_n(&atomic_barrier, __ATOMIC_RELAXED) & others) != others);
    //     __atomic_or_fetch(&atomic_barrier, mask, __ATOMIC_RELAXED);
    //     while ((__atomic_load_n(&atomic_barrier, __ATOMIC_RELAXED) & others) != 0);
    //     __atomic_and_fetch(&atomic_barrier, ~mask, __ATOMIC_RELAXED);
    // } else {
    //     while ((__atomic_load_n(&atomic_barrier, __ATOMIC_RELAXED) & 1) != 0);
    //     __atomic_or_fetch(&atomic_barrier, mask, __ATOMIC_RELAXED);
    //     while ((__atomic_load_n(&atomic_barrier, __ATOMIC_RELAXED) & 1) != 1);
    //     __atomic_and_fetch(&atomic_barrier, ~mask, __ATOMIC_RELAXED);
    // }

    // The following uses the hardware barrier.
    extern uint32_t barrier_reg;
    uint32_t tmp;
    fpu_fence();
    asm volatile (
        "lw %[tmp], 0(%[addr]) \n"
        "mv zero, %[tmp] \n"
        : [tmp] "=r"(tmp)
        : [addr] "r"(&barrier_reg)
        : "memory");
}

/// A cluster-local barrier *without* FPU fence
static inline void pulp_barrier_nofpu() {
    extern uint32_t barrier_reg;
    uint32_t tmp;
    asm volatile (
        "lw %[tmp], 0(%[addr]) \n"
        "mv zero, %[tmp] \n"
        : [tmp] "=r"(tmp)
        : [addr] "r"(&barrier_reg)
        : "memory");
}

/// The different SSR data movers.
enum ssr_dm {
    SSR_DM0 = 0,
    SSR_DM1 = 1
};

/// The different dimensions.
enum ssr_dim {
    SSR_1D = 0,
    SSR_2D = 1,
    SSR_3D = 2,
    SSR_4D = 3,
};

/// The SSR configuration registers.
typedef union { uint32_t value __attribute__((aligned(8))); } ssr_reg32_t;
typedef struct {
    ssr_reg32_t status;
    ssr_reg32_t repeat;
    ssr_reg32_t bounds[4];
    ssr_reg32_t stride[4];
    ssr_reg32_t _reserved4[14];
    ssr_reg32_t rptr[4];
    ssr_reg32_t wptr[4];
} ssr_cfg_t;
// extern volatile ssr_cfg_t ssr_config_reg[2]; // linker-provided address
static volatile ssr_cfg_t * const ssr_config_reg = (void*)0x204800;

// Configure an SSR data mover for a 1D loop nest.
static inline void pulp_ssr_loop_1d(
    enum ssr_dm dm,
    uint16_t b0,
    uint16_t i0
) {
    --b0;
    ssr_config_reg[dm].bounds[0].value = b0;
    uint16_t a = 0;
    ssr_config_reg[dm].stride[0].value = i0 - a; a += i0*b0;
}

// Configure an SSR data mover for a 2D loop nest.
static inline void pulp_ssr_loop_2d(
    enum ssr_dm dm,
    uint16_t b0,
    uint16_t b1,
    uint16_t i0,
    uint16_t i1
) {
    --b0; --b1;
    ssr_config_reg[dm].bounds[0].value = b0;
    ssr_config_reg[dm].bounds[1].value = b1;
    uint16_t a = 0;
    ssr_config_reg[dm].stride[0].value = i0 - a; a += i0*b0;
    ssr_config_reg[dm].stride[1].value = i1 - a; a += i1*b1;
}

// Configure an SSR data mover for a 3D loop nest.
static inline void pulp_ssr_loop_3d(
    enum ssr_dm dm,
    uint16_t b0,
    uint16_t b1,
    uint16_t b2,
    uint16_t i0,
    uint16_t i1,
    uint16_t i2
) {
    --b0; --b1; --b2;
    ssr_config_reg[dm].bounds[0].value = b0;
    ssr_config_reg[dm].bounds[1].value = b1;
    ssr_config_reg[dm].bounds[2].value = b2;
    uint16_t a = 0;
    ssr_config_reg[dm].stride[0].value = i0 - a; a += i0*b0;
    ssr_config_reg[dm].stride[1].value = i1 - a; a += i1*b1;
    ssr_config_reg[dm].stride[2].value = i2 - a; a += i2*b2;
}

// Configure an SSR data mover for a 4D loop nest.
static inline void pulp_ssr_loop_4d(
    enum ssr_dm dm,
    uint16_t b0,
    uint16_t b1,
    uint16_t b2,
    uint16_t b3,
    uint16_t i0,
    uint16_t i1,
    uint16_t i2,
    uint16_t i3
) {
    --b0; --b1; --b2; --b3;
    ssr_config_reg[dm].bounds[0].value = b0;
    ssr_config_reg[dm].bounds[1].value = b1;
    ssr_config_reg[dm].bounds[2].value = b2;
    ssr_config_reg[dm].bounds[3].value = b3;
    uint16_t a = 0;
    ssr_config_reg[dm].stride[0].value = i0 - a; a += i0*b0;
    ssr_config_reg[dm].stride[1].value = i1 - a; a += i1*b1;
    ssr_config_reg[dm].stride[2].value = i2 - a; a += i2*b2;
    ssr_config_reg[dm].stride[3].value = i3 - a; a += i3*b3;
}

/// Enable SSR.
static inline void pulp_ssr_enable() {
    asm volatile ("csrsi 0x7C0, 1");
}

/// Disable SSR.
static inline void pulp_ssr_disable() {
    asm volatile ("csrci 0x7C0, 1");
}

/// Start a streaming read.
static inline void pulp_ssr_read(enum ssr_dm dm, enum ssr_dim dim, void *ptr) {
    ssr_config_reg[dm].rptr[dim].value = (uint32_t)ptr;
}

/// Start a streaming write.
static inline void pulp_ssr_write(enum ssr_dm dm, enum ssr_dim dim, void *ptr) {
    ssr_config_reg[dm].wptr[dim].value = (uint32_t)ptr;
}

/// A DMA transfer indentifier.
typedef uint32_t dma_txid_t;

/// Initiate an asynchronous 1D DMA transfer with wide 64-bit pointers.
static inline dma_txid_t __dma_start_1d_wideptr_base(
    uint64_t dst,
    uint64_t src,
    size_t size,
    uint32_t const __deserialize
) {
    register uint32_t reg_dst_low   asm ("a0") = dst >> 0;   // 10
    register uint32_t reg_dst_high  asm ("a1") = dst >> 32;  // 11
    register uint32_t reg_src_low   asm ("a2") = src >> 0;   // 12
    register uint32_t reg_src_high  asm ("a3") = src >> 32;  // 13
    register uint32_t reg_size      asm ("a4") = size;       // 14

    // dmsrc a0, a1
    asm volatile (
        ".word (0b0000000 << 25) | \
               (     (13) << 20) | \
               (     (12) << 15) | \
               (    0b000 << 12) | \
               (0b0101011 <<  0)   \n"
        :: "r"(reg_src_high), "r"(reg_src_low) \
    );

    // dmdst a0, a1
    asm volatile (
        ".word (0b0000001 << 25) | \
               (     (11) << 20) | \
               (     (10) << 15) | \
               (    0b000 << 12) | \
               (0b0101011 <<  0)   \n"
        :: "r"(reg_dst_high), "r"(reg_dst_low) \
    );

    // dmcpyi a0, a4, 0b00
    register uint32_t reg_txid asm ("a0");  // 10
    asm volatile (
        ".word (0b0000010 << 25) | \
               (   %[cfg] << 20) | \
               (     (14) << 15) | \
               (    0b000 << 12) | \
               (     (10) <<  7) | \
               (0b0101011 <<  0)   \n"
        : "=r"(reg_txid) : "r"(reg_size), [cfg]"i"((__deserialize & 1) << 2) \
    );

    return reg_txid;
}

/// Initiate a *serialized* (deadlock-safe) asynchronous 1D DMA transfer with wide 64-bit pointers.
static inline dma_txid_t dma_start_1d_wideptr(
    uint64_t dst,
    uint64_t src,
    size_t size
) {
    //register dma_txid_t ret;
    // Cut transferts
    register size_t off = 0;
    for(; size-off > 512; off+=512) {
        __dma_start_1d_wideptr_base(dst+off, src+off, 512, 0);
    }
    return __dma_start_1d_wideptr_base(dst, src, size-off, 0);
}

/// Initiate a *de-serialized* (deadlock-prone) asynchronous 1D DMA transfer with wide 64-bit pointers.
/// CAUTION: Do *not* issue transfers with different direction, src, or dst in sequence without wait_all()!
static inline dma_txid_t dma_start_1d_wideptr_deser(
    uint64_t dst,
    uint64_t src,
    size_t size
) {
    return __dma_start_1d_wideptr_base(dst, src, size, 1);
}

/// Initiate an asynchronous 1D DMA transfer.
static inline dma_txid_t dma_start_1d(
    void *dst,
    const void *src,
    size_t size
) {
    return dma_start_1d_wideptr((size_t)dst, (size_t)src, size);
}

/// Initiate an asynchronous 2D DMA transfer with wide 64-bit pointers.
static inline dma_txid_t __dma_start_2d_wideptr_base(
    uint64_t dst,
    uint64_t src,
    size_t size,
    size_t dst_stride,
    size_t src_stride,
    size_t repeat,
    uint32_t const __deserialize
) {
    register uint32_t reg_dst_low    asm ("a0") = dst >> 0;    // 10
    register uint32_t reg_dst_high   asm ("a1") = dst >> 32;   // 11
    register uint32_t reg_src_low    asm ("a2") = src >> 0;    // 12
    register uint32_t reg_src_high   asm ("a3") = src >> 32;   // 13
    register uint32_t reg_size       asm ("a4") = size;        // 14
    register uint32_t reg_dst_stride asm ("a5") = dst_stride;  // 15
    register uint32_t reg_src_stride asm ("a6") = src_stride;  // 16
    register uint32_t reg_repeat     asm ("a7") = repeat;      // 17

    // dmsrc a0, a1
    asm volatile (
        ".word (0b0000000 << 25) | \
               (     (13) << 20) | \
               (     (12) << 15) | \
               (    0b000 << 12) | \
               (0b0101011 <<  0)   \n"
        :: "r"(reg_src_high), "r"(reg_src_low) \
    );

    // dmdst a0, a1
    asm volatile (
        ".word (0b0000001 << 25) | \
               (     (11) << 20) | \
               (     (10) << 15) | \
               (    0b000 << 12) | \
               (0b0101011 <<  0)   \n"
        :: "r"(reg_dst_high), "r"(reg_dst_low) \
    );

    // dmstr a5, a6
    asm volatile (
        ".word (0b0000110 << 25) | \
               (     (15) << 20) | \
               (     (16) << 15) | \
               (    0b000 << 12) | \
               (0b0101011 <<  0)   \n"
        : : "r"(reg_dst_stride), "r"(reg_src_stride) \
    );

    // dmrep a7
    asm volatile (
        ".word (0b0000111 << 25) | \
               (     (17) << 15) | \
               (    0b000 << 12) | \
               (0b0101011 <<  0)   \n"
        : : "r"(reg_repeat) \
    );

    // dmcpyi a0, a4, 0b10
    register uint32_t reg_txid asm ("a0");  // 10
    asm volatile (
        ".word (0b0000010 << 25) | \
               (   %[cfg] << 20) | \
               (     (14) << 15) | \
               (    0b000 << 12) | \
               (     (10) <<  7) | \
               (0b0101011 <<  0)   \n"
        : "=r"(reg_txid) : "r"(reg_size), [cfg]"i"(((__deserialize & 1) << 2) | 0b00010) \
    );

    return reg_txid;
}

/// Initiate a *serialized* (deadlock-safe) asynchronous 2D DMA transfer with wide 64-bit pointers.
static inline dma_txid_t dma_start_2d_wideptr(
    uint64_t dst,
    uint64_t src,
    size_t size,
    size_t dst_stride,
    size_t src_stride,
    size_t repeat
) {
    return __dma_start_2d_wideptr_base(dst, src, size, dst_stride, src_stride, repeat, 0);
}

/// Initiate a *de-serialized* (deadlock-prone) asynchronous 2D DMA transfer with wide 64-bit pointers.

static inline dma_txid_t dma_start_2d_wideptr_deser(
    uint64_t dst,
    uint64_t src,
    size_t size,
    size_t dst_stride,
    size_t src_stride,
    size_t repeat
) {
    return __dma_start_2d_wideptr_base(dst, src, size, dst_stride, src_stride, repeat, 1);
}

/// Initiate an asynchronous 2D DMA transfer.
static inline dma_txid_t dma_start_2d(
    void *dst,
    const void *src,
    size_t size,
    size_t src_stride,
    size_t dst_stride,
    size_t repeat
) {
    return dma_start_2d_wideptr(
        (size_t)dst, (size_t)src, size,
        src_stride, dst_stride, repeat
    );
}

/// Block until a transfer finishes.
static inline void dma_wait(dma_txid_t tid) {
    // dmstati t0, 0  # 2=status.completed_id
    asm volatile (
        "1: \n"
        ".word (0b0000100 << 25) | \
               (  0b00000 << 20) | \
               (    0b000 << 12) | \
               (      (5) <<  7) | \
               (0b0101011 <<  0)   \n"
        "sub t0, t0, %0 \n"
        "blez t0, 1b \n"
        :: "r"(tid) : "t0"
    );
}

/// Block until all operation on the DMA ceases.
static inline void dma_wait_all() {
    // dmstati t0, 2  # 2=status.busy
    asm volatile (
        "1: \n"
        ".word (0b0000100 << 25) | \
               (  0b00010 << 20) | \
               (    0b000 << 12) | \
               (      (5) <<  7) | \
               (0b0101011 <<  0)   \n"
        "bne t0, zero, 1b \n"
        ::: "t0"
    );
}

/// Initialize CHI bridge with a given source ID.
static inline int chi_init(const uint16_t src_id) {
    // Ensure that source ID does not exceed 11 bit.
    if (src_id >= (1 << 11)) return 1;
    // Write CHI bridge control register in SoC peripherals.
    volatile uint32_t* const chi_bridge_cfg = (volatile uint32_t*)(0x2E000008);
    *chi_bridge_cfg =
                   1 <<  0  // enable CHI bridge
            |      1 <<  1  // enable CHI TX
            |      1 <<  2  // enable CHI RX
            | src_id <<  3  // set source ID
            ;
    // Set up the cacheable region
    volatile uint32_t* const chi_bridge_cacheable_region_mask_lo  = (volatile uint32_t*)(0x2E000018);
    volatile uint32_t* const chi_bridge_cacheable_region_mask_hi  = (volatile uint32_t*)(0x2E00001C);
    volatile uint32_t* const chi_bridge_cacheable_region_value_lo = (volatile uint32_t*)(0x2E000020);
    volatile uint32_t* const chi_bridge_cacheable_region_value_hi = (volatile uint32_t*)(0x2E000024);
    *chi_bridge_cacheable_region_mask_hi  = 0xFFFF8000;
    *chi_bridge_cacheable_region_value_hi = 0x00008000;

    return 0;
}

__attribute__((optnone)) void mutex_release(volatile uint32_t *pmtx) {
    asm volatile("fence \n"
                 "amoswap.w.rl  x0,x0,(%0)   # Release lock by storing 0\n"
                 : "+r"(pmtx));
}

__attribute__((optnone)) void mutex_acquire(volatile uint32_t *pmtx) {
    asm volatile(
        "li            t0,1          # t0 = 1\n"
        "1:\n"
        "  amoswap.w.aq  t0,t0,(%0)   # t0 = oldlock & lock = 1\n"
        "  bnez          t0,1b      # Retry if previously set)\n"
        "  fence \n"
        : "+r"(pmtx)
        :
        : "t0");
}

#endif