/*
 * Authors: NGRP
 * Date: 04/2013
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>

#include "or_rstable.h"
#include "or_data_types.h"
#include "or_utils.h"

/* !! NOT THREAD SAFE !!
 * LOCK RS FOR READING BEFORE CALLING THE FUNCTION
 */
double get_rate(struct in_addr* destination, struct in_addr* mask, router_state* rs) {

	/* Logic:
	 *	 Given a destination IP adress and mask, output the corresponding rate.
	 */

	assert(destination);
	assert(mask);
	assert(rs);

	node* n = get_rstable_entry(destination, mask, rs);
	
	if (n) {
	
		rstable_entry* rse = (rstable_entry*)n->data;
	
		return rse->rate;
		
	} else {
	
		return -1.0;
		
	}

}

/* !! NOT THREAD SAFE !!
 * LOCK RS FOR READING BEFORE CALLING THE FUNCTION
 */
node* get_rstable_entry(struct in_addr* destination, struct in_addr* mask, router_state* rs) {

	/* Logic:
	 *   Determine if a particular entry exists in the atable. If so, return an
	 *	 address, otherwise return a NULL pointer.
	 */

	assert(rs);
	node* n = rs->rstable;
	
	while (n) {
	
		/* have a check on ip addresses here */
		
		rstable_entry* rse = (rstable_entry*)n->data;
		
		uint32_t rse_mask = ntohl(rse->mask.s_addr);
		uint32_t rse_ip = ntohl(rse->ip.s_addr) & rse_mask;
		
		uint32_t dest_mask = ntohl(mask->s_addr);
		uint32_t dest_ip = ntohl(destination->s_addr) & dest_mask;
		
		if (rse_ip == dest_ip) {
			return n;
		}
		
		n = n->next;
		
	}
	
	return NULL;

}

/* !! NOT THREAD SAFE !!
 * LOCK RS FOR WRITING BEFORE CALLING THE FUNCTION
 */
int add_rstable_entry(struct in_addr* destination, struct in_addr* mask, unsigned int length, router_state* rs) {

	/* Logic:
	 *	 Add a new rstable entry. If it exists, update it and leave its 
	 *	 last_update untouched. Otherwise insert a new entry with some initial
	 *	 values and set its last_update to current time.
	 */

	assert(destination);
	assert(mask);
	assert(rs);
	
	node* n = get_rstable_entry(destination, mask, rs);
	struct timeval now;
	gettimeofday(&now, NULL);
/*	
	char ip_str[INET_ADDRSTRLEN], mask_str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, destination, ip_str, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, mask, mask_str, INET_ADDRSTRLEN);
	
	printf("IP: %s, MASK: %s\n", ip_str, mask_str);
*/
	if (!n) {
	
		/* This must be a new entry, and we should set its:
		 *   rate = 0;
		 *	 flow = length;
		 *   last_flow = 0;
		 *   last_update = now.
		 */
		
		n = node_create();
		n->data = (rstable_entry*)calloc(1, sizeof(rstable_entry));
		
		update_rstable_entry(destination, mask, 0.0, length, 0, &now, n);

		if (rs->rstable == NULL) {
		
			rs->rstable = n;
			
		} else {
		
			node_push_back(rs->rstable, n);
			
		}
		
	} else {
	
		/* This is an existing entry, so we update its flow only and leave 
		 * the value of last_update untouched.
		 */
	
		update_flow_only(length, n);
		
	}
	
	//sprint_rstable_entry(n, 9999);
	
	return 1;

}

/* !! NOT THREAD SAFE !!
 * LOCK RS FOR WRITING BEFORE CALLING THE FUNCTION
 */
int del_rstable_entry(struct in_addr* destination, struct in_addr* mask, router_state* rs) {

	/* Logic:
	 *	 Delete an existing entry. If really exists, delete it. If not, the do
	 *	 nothing.
	 */
	
	assert(destination);
	assert(mask);
	assert(rs);
	
	node* n = get_rstable_entry(destination, mask, rs);
	
	if (!n) {
	
		/* Entry not found. */
		return 0;
		
	} else {
	
		/* Entry found and gonna be removed. */
		node_remove(&(rs->rstable), n);
		return 1;
	
	}

}

/* !! NOT THREAD SAFE !!
 * LOCK RS FOR READING BEFORE CALLING THE FUNCTION
 */
int sprint_rstable_entry(node* n, unsigned int index) {

	/* Logic:
	 *	 Given a pointer of an entry, print its content.
	 */
	
	assert(n);
	rstable_entry* rse = (rstable_entry*)n->data;
	
	char ip_str[INET_ADDRSTRLEN], mask_str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(rse->ip), ip_str, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &(rse->mask), mask_str, INET_ADDRSTRLEN);
	
	printf("%5u %-15s %-15s %8.2f %10u %10u %10d\n", index, ip_str, mask_str, rse->rate, rse->flow, rse->last_flow, rse->last_update_time);
	
	return 1;

}

/* !! NOT THREAD SAFE !!
 * LOCK RS FOR WRITING BEFORE CALLING THE FUNCTION
 */
int update_flow_only(unsigned int length, node* n) {

	/* Logic:
	 *   For each existing entry, modify only the flow pointed by the given 
	 *	 node pointer.
	 */

	assert(n);
	
	rstable_entry* rse = (rstable_entry*)n->data;

	rse->flow += length;	

	return 1;

}

/* !! NOT THREAD SAFE !!
 * LOCK RS FOR WRITING BEFORE CALLING THE FUNCTION
 */
int update_rstable_entry(struct in_addr* destination, struct in_addr* mask, double rate, unsigned int flow, unsigned int last_flow, struct timeval* now, node* n) {

	/* Logic:
	 *   For each newly joined entry, modify the content pointed by the given 
	 *	 node pointer.
	 */
	
	assert(destination);
	assert(mask);
	assert(now);
	assert(n);
	
	rstable_entry* rse = (rstable_entry*)n->data;
	
	rse->ip.s_addr = destination->s_addr;
	rse->mask.s_addr = mask->s_addr;
	rse->rate = rate;
	rse->flow = flow;
	rse->last_flow = last_flow;
	rse->last_update_time = *now;
	
	return 1;
	
}

/* !! NOT THREAD SAFE !!
 * LOCK RS FOR WRITING BEFORE CALLING THE FUNCTION
 */
int reset_rstable_entry(node* n) {

	rstable_entry* rse = (rstable_entry*)n->data;

	rse->flow = 0;
	
	rse->last_flow = 0;
	
	return 1;
	
}

/* THREAD ITSELF */
void* rstable_thread(void* arg) {

	router_state* rs = (router_state*)arg;
	
	//struct timeval now;
	
	while (1) {
	
		//gettimeofday(&now, NULL);
	
		//printf("or_rstable.c: rstable_thread called at %d\n", now);
	
		lock_rstable_wr(rs);
				
		compute_rstable(rs);
		//sprint_rstable(rs);
		
		unlock_rstable(rs);
		
		usleep(900000);
		
	}
	
	return NULL;

}

/* !! NOT THREAD SAFE !!
 * LOCK RS FOR WRITING BEFORE CALLING THE FUNCTION
 */
int compute_rstable(router_state* rs) {

	/* Logic:
	 *	 When called, the function will update the rate for each entry in the
	 *	 table according to
	 *
	 *              flow - last_flow        1
	 *		rate = ------------------- * ------
	 *              now - last_update     1024
	 *
	 */

	assert(rs);
	
	node* n = rs->rstable;
	rstable_entry* rse = NULL;
	
	struct timeval now;
	gettimeofday(&now, NULL);
	
	long double timeDiff;	
	
	while (n) {
		
		rse = (rstable_entry*)n->data;
		
		/* Calculate time difference using following if statements */
		if (now.tv_sec == rse->last_update_time.tv_sec) {

			/* must be a later usec time */
			timeDiff = (((long double)(now.tv_usec - rse->last_update_time.tv_usec)) / (long double)1000000);

		} else {

			timeDiff = (now.tv_sec - rse->last_update_time.tv_sec - 1) + ((long double)((1000000 - rse->last_update_time.tv_usec) + now.tv_usec) / (long double)1000000);

		}
		
		/* Update rate of the current entry */
		rse->rate = (double)((rse->flow - rse->last_flow) / timeDiff / 1024.0);
		
		/* Assign the value of flow to last_flow, and value of now to 
		 * last_update
		 */
		
		rse->last_flow = rse->flow;
		rse->last_update_time = now;
		
		n = n->next;
		
	}

	return 1;	
	
}

/* !! NOT THREAD SAFE !!
 * LOCK RS FOR WRITING BEFORE CALLING THE FUNCTION
 */
int delete_rstable(router_state* rs) {

	assert(rs);
	rs->rstable = NULL;
	
	return 1;
	
}

/* !! NOT THREAD SAFE !!
 * LOCK RS FOR READING BEFORE CALLING THE FUNCTION
 */
int sprint_rstable(router_state* rs) {

	/* Logic:
	 *   Print out the whole table.
	 */

	assert(rs);
	node* n = rs->rstable;
	unsigned int i = 0;
	
	printf("---RSTABLE---\n");
	printf("Index Destination     Mask            rate      flow       last_flow last_update\n");
	printf("=================================================================================\n");
	      //    0 140.120.31.137  255.255.255.0       0.00       2997          0 1365183742
	
	if (!n) {
	
		printf("THERE IS NO ENTRY IN RSTABLE\n");
	
	}
	
	while (n) {
	
		sprint_rstable_entry(n, i);
		i++;
		n = n->next;
		
	}
	
	printf("=================================================================================\n");
	
	return 1;
	
}

void lock_rstable_rd(router_state *rs) {
	assert(rs);

	if(pthread_rwlock_rdlock(rs->rstable_lock) != 0) {
		perror("Failure getting rstable read lock");
	}
}

void lock_rstable_wr(router_state *rs) {
	assert(rs);

	if(pthread_rwlock_wrlock(rs->rstable_lock) != 0) {
		perror("Failure getting rstable write lock");
	}
}

void unlock_rstable(router_state *rs) {
	assert(rs);

	if(pthread_rwlock_unlock(rs->rstable_lock) != 0) {
		perror("Failure unlocking rstable lock");
	}
}
