//
// Data Prefetching Championship Simulator 2
// Seth Pugsley, seth.h.pugsley@intel.com
//

/*

  This file does NOT implement any prefetcher, and is just an outline

 */

#include <stdio.h>
#include "./inc/prefetcher.h"
#include <stdlib.h>

#define GHB_SIZE 512
#define IT_SIZE 512

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



void l2_prefetcher_initialize(int cpu_num)
{

  printf("No Prefetching\n");
  // you can inspect these knob values from your code to see which configuration you're runnig in
  printf("Knobs visible from prefetcher: %d %d %d\n", knob_scramble_loads, knob_small_llc, knob_low_bandwidth);
}

void l2_prefetcher_operate(int cpu_num, unsigned long long int addr, unsigned long long int ip, int cache_hit) {

	// update index and global history tables
	if(!cache_hit) {
		if(ghb[it[ip % IT_SIZE].prev].pc == ip) {
			printf("%lld\n", ip);
			ghb[curr_idx].prev = it[ip % IT_SIZE].prev;
		} else {
			ghb[curr_idx].prev = curr_idx; // break the pc linked list if ips do not match anymore (by pointing to itself)
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
		unsigned int cnt = 0;
		while(ghb[elem_idx].prev != elem_idx &&/* ghb[elem_idx].pc == ip &&*/ state != FOUND_MATCH && cnt < GHB_SIZE) {
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
					break;
				case COND2:
					printf("case COND2\n");
					delta = delta - ghb[elem_idx].addr;
					if(delta == delta2) {
//						printf("found match");
						printf("delta1: %lld  delta2: %lld  delta: %lld\n", delta1, delta2, delta);
						state = FOUND_MATCH;
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
		curr_idx = (curr_idx + 1) % GHB_SIZE;

	}
	/*
	char sign = (delta < 0);
	long long int old_delta = delta;
	delta = sign ? (~delta + 1) : delta;
	if(delta < IT_SIZE) {
		printf("%lld -> %lld\n",old_delta, (delta  ^ sign) % IT_SIZE);
	}
	// printf("%lld -> %lld\n",old_delta, (delta  ^ sign) % IT_SIZE);
	prev_addr = addr;
	*/
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
  printf("Prefetcher final stats\n");
}
