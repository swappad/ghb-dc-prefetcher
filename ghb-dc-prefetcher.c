//
// Data Prefetching Championship Simulator 2
// Seth Pugsley, seth.h.pugsley@intel.com
//


#include <stdio.h>
#include "./inc/prefetcher.h"

#define GHB_SIZE 1024
#define IT_SIZE 256
#define DELTA_RANGE 6000

// Implementation Variants:
// these three must be used mutually
//#define ACCURATE_PREFETCH
#define SEMI_ACCURATE_PREFETCH
//#define PRIMITIVE_PREFETCH

//#define CHECK_BOUNDS


typedef struct {
	unsigned long long int pc;
	unsigned int prev;
	unsigned long long int addr;
} GHB_ELEM;

GHB_ELEM ghb[GHB_SIZE] = {0};
unsigned int curr_idx = 0;

typedef struct {
	unsigned int prev;
} IDX_ELEM;

IDX_ELEM it[IT_SIZE] = {0};

unsigned long long int prev_addr = 0;


typedef enum {
	DELTA1 = 0,
	DELTA2 = 1,
	COND1 = 2,
	COND2 = 3,
	FOUND_MATCH = 4
} STATE;


void print_ghb() {
	printf("IT: \n");
	for(int i=0; i < IT_SIZE; i++) {
		printf("prev: %d\n",it[i].prev);
	}
	for(int i=0; i < GHB_SIZE; i++) {
		printf("i: %d, pc: %lld, prev: %d, addr: %lld, delta1: %lld\n", i, ghb[i].pc, ghb[i].prev, ghb[i].addr, ghb[i].addr - ghb[ghb[i].prev].addr);
	}
}

void l2_prefetcher_initialize(int cpu_num)
{

  printf("GHB PC/DC prefetching\n");
  // you can inspect these knob values from your code to see which configuration you're runnig in
  printf("Knobs visible from prefetcher: %d %d %d\n", knob_scramble_loads, knob_small_llc, knob_low_bandwidth);
}

float no_match = 0.0f;
float match = 0.0f;
float cache_access = 0;
float cache_miss = 0;
void l2_prefetcher_operate(int cpu_num, unsigned long long int addr, unsigned long long int ip, int cache_hit) {
	cache_access += 1.0;

	if(!cache_hit) {
		cache_miss+= 1.0;

		// update index and global history tables on cache miss
		if(ghb[it[ip % IT_SIZE].prev].pc == ip) {
			ghb[curr_idx].prev = it[ip % IT_SIZE].prev;
		} else {
			ghb[curr_idx].prev = curr_idx; // break the pc linked list if ip does not match anymore (by pointing to itself)
		}
		ghb[curr_idx].addr = addr;
		ghb[curr_idx].pc = ip;
		it[ip % IT_SIZE].prev = curr_idx;

		// check for prefetching candidates 
		unsigned int elem_idx = ghb[curr_idx].prev;
		STATE state = DELTA1;
		long long int delta1 = ghb[curr_idx].addr;
		long long int delta2 = 0;
		long long int delta = 0;
		unsigned int cnt = 0; // counter avoids infinite search loop


		while(ghb[elem_idx].prev != elem_idx && ghb[elem_idx].pc == ip && state != FOUND_MATCH && cnt < GHB_SIZE) {
//			printf("%d\n", ghb[elem_idx].prev);
//			printf("current index: %d\n", curr_idx);
			switch(state) {
				case DELTA1:
//					printf("case DELTA1\n");
					delta1 = delta1 - ghb[elem_idx].addr;
					delta2 = ghb[elem_idx].addr;
					state = DELTA2;
					elem_idx = ghb[elem_idx].prev;
					break;
				case DELTA2:
//					printf("case DELTA2\n");
					delta2 = delta2 - ghb[elem_idx].addr;
					state = COND1;
					delta = ghb[elem_idx].addr;
					elem_idx = ghb[elem_idx].prev;
					break;
				case COND1:
					cnt++;
//					printf("case COND1\n");
					delta = delta - ghb[elem_idx].addr;
					if(delta == delta1) {
						state = COND2;
					} 
					delta = ghb[elem_idx].addr;
//					printf("elem_idx: %d\n", elem_idx);
					elem_idx = ghb[elem_idx].prev;
//					printf("delta: %lld\n", delta);
//					printf("delta1: %lld\n", delta1);
					if(cnt >= GHB_SIZE) {
						no_match += 1.0;
					}
					break;
				case COND2:
//					printf("case COND2\n");
					delta = delta - ghb[elem_idx].addr;
					if(delta == delta2) {
//						printf("found match");

						unsigned long long int lower;
						unsigned long long int upper;
						unsigned long long int high_delta;
						unsigned long long int low_delta;
#ifdef CHECK_BOUNDS
						if((delta1 > DELTA_RANGE || delta1 < -DELTA_RANGE) && (delta2 > DELTA_RANGE || delta2 < -DELTA_RANGE)) return;
						if(delta2 > DELTA_RANGE || delta2 < -DELTA_RANGE) {
							delta2 = 0;
						}
						if(delta1 > DELTA_RANGE|| delta1 < -DELTA_RANGE) {
							addr = addr + delta1;
							delta1 = 0;
						}
#endif

						int out = 0;

#ifdef SEMI_ACCURATE_PREFETCH
						if(delta1 >= delta2) {
							high_delta = delta1;
							low_delta = delta2;
						} else {
							high_delta = delta2;
							low_delta = delta1;
						}
						lower = (low_delta < 0) ? addr + low_delta: addr;
						upper = (high_delta > 0) ? addr + high_delta : addr;
						out = l2_prefetch_line(0, lower, upper, FILL_L2);
#endif
#ifdef ACCURATE_PREFETCH

						if(delta1 < 0 && delta1+delta2 < 0) {
							upper = addr;
						}
						if(delta1 >=0 && delta2 < 0) {
							upper = addr + delta1;
						}
						if(delta2 >=0 && delta1+delta2 > 0) {
							upper = addr+delta1+delta2;
						}
						if(delta1 >= 0 && delta1+delta2 >=0) {
							lower = addr;
						}
						if(delta1 < 0 && delta2 >= 0) {
							lower = addr+delta1;
						}
						if(delta2 < 0 && delta1+delta2 < 0) {
							lower = addr+delta1+delta2;
						}
						
						if(get_l2out = _mshr_occupancy(0) > 8) {
							out = // conservatively prefetch into the LLC, because MSHRs are scarce
							out = l2_prefetch_line(0, lower, upper, FILL_LLC);
						} else {
							// MSHRs not too busy, so prefetch into L2
							out = l2_prefetch_line(0, lower, upper, FILL_L2);
						}

						//out = l2_prefetch_line(0, lower, upper, FILL_L2);
#endif

//						printf("lower: %lld, target addr: %lld\n", lower, ghb[elem_idx].addr);
//						printf("upper: %lld, miss addr: %lld\n", upper, addr);

#ifdef PRIMITIVE_PREFETCH
						  out = l2_prefetch_line(0, addr, addr + delta1, FILL_L2);
#endif

//						printf("%s\n", out ? "successfull\0" : "not successfull\0");

//						if(!out) printf("delta1: %lld  delta2: %lld  delta: %lld\n\n", delta1, delta2, delta);
						state = FOUND_MATCH;
						match+=1.0;
						//printf("match at pos: %d\n", cnt);
						elem_idx = ghb[elem_idx].prev;
					} else {
						state = COND1;
						delta = ghb[elem_idx].addr;

					}
					break;
				default: 
					printf("Something went wrong!\n");
					break;
			}

		}
		// circular buffer pointer
		curr_idx = (curr_idx + 1) % GHB_SIZE;
//		print_ghb();

	}
  // uncomment this line to see all the information available to make prefetch decisions
  // printf("(0x%llx 0x%llx %d %d %d) ", addr, ip, cache_hit, get_l2_read_queue_occupancy(0), get_l2_mshr_occupancy(0));
}


void l2_cache_fill(int cpu_num, unsigned long long int addr, int set, int way, int prefetch, unsigned long long int evicted_addr)
{
  // uncomment this line to see the information available to you when there is a cache fill event
  //printf("0x%llx %d %d %d 0x%llx\n", addr, set, way, prefetch, evicted_addr);
}

void l2_prefetcher_heartbeat_stats(int cpu_num)
{
  printf("Prefetcher heartbeat stats\n");
}

void l2_prefetcher_warmup_stats(int cpu_num)
{
  printf("Prefetcher warmup complete stats\n\n");
}

void l2_prefetcher_final_stats(int cpu_num)
{
	printf("match/no match: %.3f\n", (match/no_match));
	printf("miss rate: %0.3f\n", cache_miss/cache_access);
	printf("Prefetcher final stats\n");
}
