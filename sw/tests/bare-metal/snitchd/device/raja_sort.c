
// DMA Test with double buffering

//#include "printf.h"
#include "runtime.h"
//#include "sw_mailbox.h"
#include <inttypes.h>


void _putchar(char byte)
{
    //mailbox_write((uint32_t)byte);
}

extern volatile char l1_alloc_base;
__attribute__((section(".init_l1"))) char     * const  l1_alloc_base_ptr = (char *)     &l1_alloc_base;
extern volatile uint32_t jump_address;
__attribute__((section(".init_l1"))) uint32_t * const  jump_address_ptr  = (uint32_t *) &jump_address;
extern volatile uint32_t scratch_reg;
__attribute__((section(".init_l1"))) uint32_t * const  scratch_reg_ptr   = (uint32_t *) &scratch_reg;
extern volatile uint32_t barrier_reg;
__attribute__((section(".init_l1"))) uint32_t * const  barrier_reg_ptr   = (uint32_t *) &barrier_reg;

uint64_t l1_buf_1[1024] __attribute__((section(".noinit_l1")));
uint64_t l1_buf_2[1024] __attribute__((section(".noinit_l1")));


/*

extern volatile uint32_t eoc_address;

const uint32_t *eoc_address_ptr   = (uint32_t *)&eoc_address;
const uint32_t *l1_alloc_base_ptr = (uint32_t *)&l1_alloc_base;

//#define BUF_SIZE (64)

// program inputs
uint64_t l3_vec_in_addr __attribute__((section(".noinit_l1")));

// merge_bufs local work buffers
double l1_buf_1[8][2*BUF_SIZE] __attribute__((section(".noinit_l1")));
double l1_buf_2[8][2*BUF_SIZE] __attribute__((section(".noinit_l1")));
double l1_buf_out[8][BUF_SIZE] __attribute__((section(".noinit_l1")));

// merge_bufs communication variables
uint32_t buf_1_head[8] __attribute__((section(".noinit_l1")));
uint32_t buf_1_tail[8] __attribute__((section(".noinit_l1")));
uint32_t buf_2_head[8] __attribute__((section(".noinit_l1")));
uint32_t buf_2_tail[8] __attribute__((section(".noinit_l1")));
uint32_t buf_1_off[8] __attribute__((section(".noinit_l1")));
uint32_t buf_2_off[8] __attribute__((section(".noinit_l1")));

// merge_bufs arguments
uint64_t buf_1_addr[8] __attribute__((section(".noinit_l1")));
uint64_t buf_2_addr[8] __attribute__((section(".noinit_l1")));
uint64_t buf_out_addr[8] __attribute__((section(".noinit_l1")));

// performance counters
pulp_timer_t dma_time __attribute__((section(".noinit_l1")));

static inline uint32_t dma_fill_buf(double *buf_l1, uint32_t buf_l1_size, uint32_t head, uint32_t tail, uint32_t off, uint64_t buf_l3_addr, uint32_t l3_elems_left, uint32_t core) {
    // Note that each transfer here is maximum of size BUF_SIZE (since the compute cores only process BUF_SIZE elements each)
    uint32_t tx_size = (tail <= head) ? head - tail : 2*buf_l1_size - tail + head;
    if(tx_size > l3_elems_left)
        tx_size = l3_elems_left;
    //if(core == 0)
    //    printf("head %u tail %u l3_left %u rx %u\n\r", head, tail, l3_elems_left, tx_size);
    if(tx_size == 0)
        return 0;
    if(tail > head) {
        uint32_t tx_1_size = 2*buf_l1_size - tail;
        // First transfer from tail to the end of the buffer
        __dma_start_1d_wideptr_base((uint64_t) &buf_l1[tail], buf_l3_addr + off*sizeof(double), tx_1_size*sizeof(double), 0);
        // Second transfer from start of the buffer to tail
        if(tx_1_size != tx_size)
            __dma_start_1d_wideptr_base((uint64_t) &buf_l1[0], buf_l3_addr + off*sizeof(double) + tx_1_size*sizeof(double), (tx_size-tx_1_size)*sizeof(double), 0);
    } else if (tail < head) {
        // One transfer from head to tail
        __dma_start_1d_wideptr_base((uint64_t) &buf_l1[tail], buf_l3_addr + off*sizeof(double), tx_size*sizeof(double), 0);
    }
    return tx_size;
}

PULP_NOINLINE void merge_bufs(uint64_t buf_1_addr[8], uint64_t buf_2_addr[8], uint64_t buf_out_addr[8], uint64_t l3_buf_size) {
    pulp_timer_t dma_timer;
    uint32_t core_idx     = pulp_get_core_id();

    // If l3_buf_size is smaller than BUF_SIZE, limit the working size
    uint32_t buf_size = l3_buf_size < BUF_SIZE ? l3_buf_size : BUF_SIZE;

    // Local copies used by the compute core
    uint32_t next_buf_1_head = 0;
    uint32_t next_buf_2_head = 0;
    uint32_t buf_1_size = l3_buf_size;
    uint32_t buf_2_size = l3_buf_size;

    // Only wait in (issue with dma out...)
    dma_txid_t wait_id = 0;

    if(core_idx < 8) {
        // Global pointers used by the DMA core
        buf_1_head[core_idx] = 0;
        buf_1_tail[core_idx] = buf_size;
        buf_2_head[core_idx] = 0;
        buf_2_tail[core_idx] = buf_size;
        // offset in l3 buffer
        buf_1_off[core_idx] = buf_size;
        buf_2_off[core_idx] = buf_size;
    } else if (core_idx == 8) {
        for(int core = 0; core < 8; core++) {
            //if(core == 0)
            //        printf("-> %u (%x)\n\r", (uint32_t)(buf_1_addr[core]/8), buf_size);
            if(buf_1_addr[core] == 0)
                continue;
            __dma_start_1d_wideptr_base((uint64_t) &l1_buf_1[core][0], buf_1_addr[core], buf_size*sizeof(double), 0);
            __dma_start_1d_wideptr_base((uint64_t) &l1_buf_2[core][0], buf_2_addr[core], buf_size*sizeof(double), 0);
        }
    }

    for(int elems_out = 0; elems_out < 2*l3_buf_size; elems_out += buf_size) {

        // Issue DMA to re-fill the L1 buffers for next iteration
        if(core_idx == 8) {
            dma_timer = pulp_get_timer();
            dma_wait_all();
            dma_time += pulp_get_timer() - dma_timer;

            for(int core = 0; core < 8; core++) {
                if(buf_1_addr[core] == 0)
                    continue;
                if(core == 0)
                    printf("-> %u (%x)\n\r", (uint32_t)(buf_1_addr[core]/8) + (buf_1_off[core]), buf_size);
                buf_1_off[core] += dma_fill_buf(&l1_buf_1[core][0], buf_size, buf_1_head[core], buf_1_tail[core], buf_1_off[core], buf_1_addr[core], l3_buf_size-buf_1_off[core], core);
                buf_2_off[core] += dma_fill_buf(&l1_buf_2[core][0], buf_size, buf_2_head[core], buf_2_tail[core], buf_2_off[core], buf_2_addr[core], l3_buf_size-buf_2_off[core], core);

            }
        }

        // Synch with after the dma wait all
        pulp_barrier();

        //if(core_idx == 0) {
        //    uint32_t k = buf_1_head[core_idx];
        //    printf("Buf 1 (%2u-%2u) : ", buf_1_head[core_idx], buf_1_tail[core_idx]);
        //    for(uint32_t kk = 0; kk < ((buf_1_size < buf_size) ? buf_1_size : buf_size); kk++) {
        //        printf("%.2f - ", l1_buf_1[core_idx][k]);
        //        k = (k+1) % (2*buf_size);
        //    }
        //    printf("\n\rBuf 2 (%2u-%2u) : ", buf_2_head[core_idx], buf_2_tail[core_idx]);
        //    k = buf_2_head[core_idx];
        //    for(uint32_t kk = 0; kk < ((buf_2_size < buf_size) ? buf_2_size : buf_size); kk++) {
        //        printf("%.2f - ", l1_buf_2[core_idx][k]);
        //        k = (k+1) % (2*buf_size);
        //    }
        //}

        // Compare L1 buffers (only what have been filled from last iteration) and fill output buffer
        if(core_idx < 8 && buf_1_addr[core_idx] != 0) {
            for(int k = 0; k < buf_size; k++) {
                //if(core_idx == 0)
                //    printf("%.2f (%u) vs %.2f (%u)\n\r", l1_buf_1[core_idx][next_buf_1_head], buf_1_size, l1_buf_2[core_idx][next_buf_2_head], buf_2_size);
                if((!buf_2_size || l1_buf_1[core_idx][next_buf_1_head] <= l1_buf_2[core_idx][next_buf_2_head]) && buf_1_size) {
                    l1_buf_out[core_idx][k] = l1_buf_1[core_idx][next_buf_1_head];
                    next_buf_1_head = (next_buf_1_head+1) % (2*buf_size);
                    buf_1_size--;
                } else if (buf_2_size) {
                    l1_buf_out[core_idx][k] = l1_buf_2[core_idx][next_buf_2_head];
                    next_buf_2_head = (next_buf_2_head+1) % (2*buf_size);
                    buf_2_size--;
                }
            }
        }

        pulp_barrier();

        //if(core_idx == 0) {
        //    printf("\n\rBuf o         : ");
        //    for(uint32_t k = 0; k < buf_size; k++) {
        //        printf("%.2f - ", l1_buf_out[core_idx][k]);
        //    }
        //    printf("\n\r");
        //}
        //pulp_barrier();

        // Now that dma have been issued we can update heads and tails
        if(core_idx < 8 && buf_1_addr[core_idx] != 0) {
            buf_1_tail[core_idx] = buf_1_head[core_idx];
            buf_2_tail[core_idx] = buf_2_head[core_idx];
            buf_1_head[core_idx] = next_buf_1_head;
            buf_2_head[core_idx] = next_buf_2_head;
        }

        // Send back result
        if(core_idx == 8) {
            // WTF: L1 to L3 DMA transfers fail above 64-bytes, probably due to overlapping
            // of IN and OUT transfers...
            uint32_t allowed_size = (buf_size > 8) ? 8 : buf_size;
            for(int core = 0; core < 8; core++) {
                dma_timer = pulp_get_timer();
                dma_wait_all();
                dma_time += pulp_get_timer() - dma_timer;
                if(buf_1_addr[core] == 0)
                    continue;
                if(core == 0)
                    printf("<- %u (%x) [eo=%u]\n\r", (uint32_t)(buf_out_addr[core]/8) + (elems_out), buf_size, elems_out);
                //for(uint32_t tx = 0; tx < buf_size; tx += allowed_size)
                //    __dma_start_1d_wideptr_base(buf_out_addr[core] + (elems_out+tx)*sizeof(double), (uint64_t) &l1_buf_out[core][tx], allowed_size*sizeof(double), 0);
                //__dma_start_1d_wideptr_base(buf_out_addr[core] + (elems_out)*sizeof(double), (uint64_t) &l1_buf_out[core][0], buf_size*sizeof(double), 0);
            }
        }

        pulp_barrier();
    }


    //if(core_idx == 8)
    //    printf("Time : %u (dma : %u)\n\r", all_time, dma_time);
}
*/

int main()
{
    uint32_t core_idx     = pulp_get_core_id();
    int err               = 0;
    
    if (core_idx == 8) {
        __dma_start_1d_wideptr_base((uint64_t) &l1_buf_1, 0xC0000000, 1024*sizeof(uint64_t), 0);
        __dma_start_1d_wideptr_base((uint64_t) &l1_buf_2, 0xC0020000, 1024*sizeof(uint64_t), 0);
    }

/*
    if (core_idx == 8) {
        lock       = 0;
        g_mboxes   = (struct mboxes *)*eoc_address_ptr;
        g_a2h_mbox = (struct ring_buf *)g_mboxes->a2h_ptr;
        g_h2a_mbox = (struct ring_buf *)g_mboxes->h2a_ptr;
        err        = chi_init(12);
        // Read two uint32_t to get one uint64_t
        mailbox_read((uint32_t *)&l3_vec_in_addr, 2);
        printf("Inputs: %llx\n\r", l3_vec_in_addr);
        printf("\n\r");
    }

    pulp_barrier();

    uint32_t global_itr = 0;
    uint32_t log_counter = VEC_SIZE / 8;
    uint32_t itr_counter = 1;

    // Only power of two works
    if((log_counter & (log_counter - 1)) != 0 && log_counter != 0)
        return 0xC4C4F3F3;

    pulp_timer_t all_time = 0, all_timer = pulp_get_timer();

    while(log_counter > 1) {
        //if(core_idx == 8)
        //    printf("\n\r(%u %u)\n\r\n\r", log_counter, itr_counter);
        for (int i = 0; i < VEC_SIZE / 8; i += (itr_counter*2)) {
            if(core_idx < 8) {
                buf_1_addr[core_idx]   = l3_vec_in_addr + global_itr%2*VEC_SIZE*sizeof(double)     + core_idx*(VEC_SIZE/8)*sizeof(double) + i*sizeof(double);
                buf_2_addr[core_idx]   = l3_vec_in_addr + global_itr%2*VEC_SIZE*sizeof(double)     + core_idx*(VEC_SIZE/8)*sizeof(double) + (i+itr_counter)*sizeof(double);
                buf_out_addr[core_idx] = l3_vec_in_addr + (global_itr+1)%2*VEC_SIZE*sizeof(double) + core_idx*(VEC_SIZE/8)*sizeof(double) + i*sizeof(double);
            }

            pulp_barrier();
            merge_bufs(buf_1_addr, buf_2_addr, buf_out_addr, itr_counter);
            global_itr++;
            pulp_barrier();
        }
        itr_counter = itr_counter << 1;
        log_counter = log_counter >> 1;
    }

    itr_counter = 2;
    while(itr_counter <= 8) {
        // Final merges (some cores don't work anymore)
        if(core_idx < 8 && (core_idx % itr_counter == 0)) {
            asm volatile ("fence");
            //if(core_idx == 0)
            //    printf("\n\r[%u %u (%u)]\n\r\n\r", core_idx*(VEC_SIZE/8), (core_idx + itr_counter/2)*(VEC_SIZE/8), itr_counter/2*VEC_SIZE/8);
            buf_1_addr[core_idx]   = l3_vec_in_addr + global_itr%2*VEC_SIZE*sizeof(double)     + core_idx*(VEC_SIZE/8)*sizeof(double);
            buf_2_addr[core_idx]   = l3_vec_in_addr + global_itr%2*VEC_SIZE*sizeof(double)     + (core_idx + itr_counter/2)*(VEC_SIZE/8)*sizeof(double);
            buf_out_addr[core_idx] = l3_vec_in_addr + (global_itr+1)%2*VEC_SIZE*sizeof(double) + core_idx*(VEC_SIZE/8)*sizeof(double);
            asm volatile ("fence");
        } else if (core_idx < 8) {
            asm volatile ("fence");
            buf_1_addr[core_idx] = 0;
            buf_2_addr[core_idx] = 0;
            buf_out_addr[core_idx] = 0;
            asm volatile ("fence");
        }
        pulp_barrier();
        merge_bufs(buf_1_addr, buf_2_addr, buf_out_addr, itr_counter/2*VEC_SIZE/8);
        global_itr++;
        pulp_barrier();
        itr_counter = itr_counter * 2;
    }

    all_time = pulp_get_timer() - all_timer;
    if(core_idx == 8)
        printf("Time : %u (dma : %u)\n\r", all_time, dma_time);
*/
    return (uint32_t)0x0;
}
