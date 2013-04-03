/*
 * Authors: NGRP
 * Date: 06/2007
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <assert.h>

#include "or_atable.h"
#include "or_data_types.h"
#include "or_utils.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/* !! NOT THREAD SAFE !!
 * LOCK RS FOR READING BEFORE CALLING THE FUNCTION
 */
double get_alpha(struct in_addr* destination, struct in_addr* mask, char* next_hop_iface, router_state* rs) {

	/* Logic:
	 *	 Given a destination IP adress and the next hop interface, output the
	 *	 corresponding alpha.
	 */

	assert(destination);
	assert(mask);
	assert(next_hop_iface);
	assert(rs);

	node* n = get_atable_entry(destination, mask, rs);
	
	if (n) {
	
		atable_entry* ae = (atable_entry*)n->data;
	
		if(!strcmp(next_hop_iface, "eth0")) {
		
			return ae->alpha[0];
		
		} else if(!strcmp(next_hop_iface, "eth1")) {
	
			return ae->alpha[1];
		
		} else if(!strcmp(next_hop_iface, "eth2")) {
	
			return ae->alpha[2];
		
		} else {
	
			return ae->alpha[3];
		
		} 
		
	} else {
	
		return -1.0;
		
	}
	
}

/* !! NOT THREAD SAFE !!
 * LOCK RS FOR READING BEFORE CALLING THE FUNCTION
 */
node* get_atable_entry(struct in_addr* destination, struct in_addr* mask, router_state* rs) {

	/* Logic:
	 *   Determine if a particular entry exists in the atable. If so, return an
	 *	 address, otherwise return a NULL pointer.
	 */

	assert(rs);
	node* n = rs->atable;
	
	while (n) {
	
		/* have a check on ip addresses here */
		
		atable_entry* ae = (atable_entry*)n->data;
		
		uint32_t ae_mask = ntohl(ae->mask.s_addr);
		uint32_t ae_ip = ntohl(ae->ip.s_addr) & ae_mask;
		
		uint32_t dest_mask = ntohl(mask->s_addr);
		uint32_t dest_ip = ntohl(destination->s_addr) & dest_mask;
		
		if (ae_ip == dest_ip) {
			return n;
		}
		
		n = n->next;
		
	}
	
	return NULL;
	
}

/* !! NOT THREAD SAFE !!
 * LOCK RS FOR WRITING BEFORE CALLING THE FUNCTION
 */
int add_atable_entry(struct in_addr* destination, struct in_addr* mask, double* alpha, router_state* rs) {

	/* Logic:
	 *	 Add a new atable entry. If it exists, update it, otherwise insert a new
	 *	 entry.
	 */

	assert(destination);
	assert(mask);
	assert(alpha);
	assert(rs);
	
	node* n = get_atable_entry(destination, mask, rs);
	
	if (!n) {
	
		/* This must be a new entry. */
		
		n = node_create();
		n->data = (atable_entry*)calloc(1, sizeof(atable_entry));
		
		update_atable_entry(destination, mask, alpha, n);

		if (rs->atable == NULL) {
		
			rs->atable = n;
			
		} else {
		
			node_push_back(rs->atable, n);
			
		}
		
	} else {
	
		update_atable_entry(destination, mask, alpha, n);
		
	}
	
	//sprint_atable_entry(ae, 9999);
	
	return 1;

}
 
/* !! NOT THREAD SAFE !!
 * LOCK RS FOR WRITING BEFORE CALLING THE FUNCTION
 */
int del_atable_entry(struct in_addr* destination, struct in_addr* mask, router_state* rs) {

	/* Logic:
	 *	 Delete an existing entry. If really exists, delete it. If not, the do
	 *	 nothing.
	 */
	
	assert(destination);
	assert(mask);
	assert(rs);
	
	node* n = get_atable_entry(destination, mask, rs);
	
	if (!n) {
	
		/* Entry not found. */
		return 0;
		
	} else {
	
		/* Entry found and gonna be removed. */
		node_remove(&(rs->atable), n);
		return 1;
	
	}

}

/* !! NOT THREAD SAFE !!
 * LOCK RS FOR READING BEFORE CALLING THE FUNCTION
 */
int sprint_atable_entry(node* n, unsigned int index) {

	/* Logic:
	 *	 Given a pointer of an entry, print its content.
	 */
	
	assert(n);
	atable_entry* ae = (atable_entry*)n->data;
	
	char ip_str[INET_ADDRSTRLEN], mask_str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(ae->ip), ip_str, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &(ae->mask), mask_str, INET_ADDRSTRLEN);
	
	printf("%5u %-15s %-15s %1.7f %1.7f %1.7f %1.7f\n", index, ip_str, mask_str, ae->alpha[0], ae->alpha[1], ae->alpha[2], ae->alpha[3]);
	
	return 1;

}

 
/* !! NOT THREAD SAFE !!
 * LOCK RS FOR WRITING BEFORE CALLING THE FUNCTION
 */
int update_atable_entry(struct in_addr* destination, struct in_addr* mask, double* alpha, node* n) {

	/* Logic:
	 *   Modify the content pointed by the given node pointer.
	 */
	assert(destination);
	assert(mask);
	assert(alpha);
	assert(n);
	
	atable_entry* ae = (atable_entry*)n->data;
	
	ae->ip.s_addr = destination->s_addr;
	ae->mask.s_addr = mask->s_addr;
	
	ae->alpha[0] = alpha[0];
	ae->alpha[1] = alpha[1];
	ae->alpha[2] = alpha[2];
	ae->alpha[3] = alpha[3];
	
	return 1;

}

/* !! NOT THREAD SAFE !!
 * LOCK RS FOR WRITING BEFORE CALLING THE FUNCTION
 */
int compute_atable(router_state* rs) {

	/* Logic:
	 *   After each Dijkstra's computation, we need to update our atable 
	 *   according to the new rtable. For each entry in rtable, check if it is
	 *	 already in atable. If so, then update its alpha values. If not, that
	 *	 means it is a newly joined node and we should therefore include it into
	 *	 atable and initialize it.
	 */

	assert(rs);
	node* n = rs->rtable;
	
	double delta = 0.01;
	double eta = 1.0;
	double rate = 1.0;
	double alpha[4];
	
	node* p = NULL;	// for atable linked list
	
	if (!n)
		printf("THERE IS NO ENTRY IN RTABLE\n");
	
	while (n) {
	
		rtable_entry* re = (rtable_entry*)n->data;	
		
		if (re->is_active) {

			char iface[32];
			strncpy(iface, re->iface, 32);
			
			p = (node*)get_atable_entry(&(re->ip), &(re->mask), rs);
			
			if (!p) {
			
			/* If there is a newly joined destination which does not appear in
			 * our current atable, we create a new entry and insert it into 
			 * atable.
			 */
			 
			 	//printf("or_atable.c: atable is emtpy\n");
			 
			 	/* allocate memory for a node and its data */
				
				alpha[0] = 0;
				alpha[1] = 0;
				alpha[2] = 0;
				alpha[3] = 0;
				
				if (!strcmp(iface, "eth0")) {
					alpha[0] = 1;
				} else if (!strcmp(iface, "eth1")) {
					alpha[1] = 1;
				} else if (!strcmp(iface, "eth2")) {
					alpha[2] = 1;
				} else if (!strcmp(iface, "eth3")) {
					alpha[3] = 1;
				}

				add_atable_entry(&(re->ip), &(re->mask), alpha, rs);
				
			} else {
			
			/* If the destination is already known, then calculate alpha dot 
			 * and update alpha
			 */
			 	atable_entry* ae = (atable_entry*)p->data;
			
				if (!strcmp(iface, "eth0")) {
					alpha[1] = MIN(1, MAX(0, ae->alpha[1] - ae->alpha[1] * delta / eta / rate));
					alpha[2] = MIN(1, MAX(0, ae->alpha[2] - ae->alpha[2] * delta / eta / rate));
					alpha[3] = MIN(1, MAX(0, ae->alpha[3] - ae->alpha[3] * delta / eta / rate));
					alpha[0] = 1 - ae->alpha[1] - ae->alpha[2] - ae->alpha[3];
				} else if (!strcmp(iface, "eth1")) {
					alpha[0] = MIN(1, MAX(0, ae->alpha[0] - ae->alpha[0] * delta / eta / rate));
					alpha[2] = MIN(1, MAX(0, ae->alpha[2] - ae->alpha[2] * delta / eta / rate));
					alpha[3] = MIN(1, MAX(0, ae->alpha[3] - ae->alpha[3] * delta / eta / rate));
					alpha[1] = 1 - ae->alpha[0] - ae->alpha[2] - ae->alpha[3];
				} else if (!strcmp(iface, "eth2")) {
					alpha[0] = MIN(1, MAX(0, ae->alpha[0] - ae->alpha[0] * delta / eta / rate));
					alpha[1] = MIN(1, MAX(0, ae->alpha[1] - ae->alpha[1] * delta / eta / rate));
					alpha[3] = MIN(1, MAX(0, ae->alpha[3] - ae->alpha[3] * delta / eta / rate));
					alpha[2] = 1 - ae->alpha[0] - ae->alpha[1] - ae->alpha[3];
				} else if (!strcmp(iface, "eth3")) {
					alpha[0] = MIN(1, MAX(0, ae->alpha[0] - ae->alpha[0] * delta / eta / rate));
					alpha[1] = MIN(1, MAX(0, ae->alpha[1] - ae->alpha[1] * delta / eta / rate));
					alpha[2] = MIN(1, MAX(0, ae->alpha[2] - ae->alpha[2] * delta / eta / rate));
					alpha[3] = 1 - ae->alpha[0] - ae->alpha[1] - ae->alpha[2];
				}
				
				update_atable_entry(&(re->ip), &(re->mask), alpha, p);
				
			}
			
		}
		
		n = n->next;
		
	}
	
	return 1;
}

/* NOT THREAD SAFE
 * LOCK RS FOR WRITING BEFORE CALLING THE FUNCTION
 */
int delete_atable(router_state* rs) {

	assert(rs);
	rs->atable = NULL;
	
	return 1;
}

/* NOT THREAD SAFE
 * LOCK RS FOR READING BEFORE CALLING THE FUNCTION
 */
int sprint_atable(router_state *rs) {

	/* Logic:
	 *   Print out the whole table.
	 */

	assert(rs);
	node* n = rs->atable;
	unsigned int i = 0;
	
	printf("---ATABLE AFTER DIJKSTRA---\n");
	printf("Index Destination     Mask            alpha[0]  alpha[1]  alpha[2]  alpha[3] \n");
	printf("=============================================================================\n");
	      //    0 140.120.31.137  255.255.255.0   0.2500000 0.0000000 0.3000000 0.4500000
	
	if (!n) {
	
		printf("THERE IS NO ENTRY IN ATABLE\n");
	
	}
	
	while (n) {
	
		sprint_atable_entry(n, i);
		i++;
		n = n->next;
		
	}
	
	printf("=============================================================================\n");
	
	return 1;
}

void lock_atable_rd(router_state *rs) {
	assert(rs);

	if(pthread_rwlock_rdlock(rs->atable_lock) != 0) {
		perror("Failure getting atable read lock");
	}
}

void lock_atable_wr(router_state *rs) {
	assert(rs);

	if(pthread_rwlock_wrlock(rs->atable_lock) != 0) {
		perror("Failure getting atable write lock");
	}
}

void unlock_atable(router_state *rs) {
	assert(rs);

	if(pthread_rwlock_unlock(rs->atable_lock) != 0) {
		perror("Failure unlocking atable lock");
	}
}
