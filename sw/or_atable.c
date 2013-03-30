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

//#include "or_rtable.h"
#include "or_atable.h"
//#include "or_main.h"
#include "or_data_types.h"
//#include "or_output.h"
//#include "or_utils.h"
//#include "or_netfpga.h"
//#include "nf2/nf2util.h"
//#include "reg_defines.h"

double get_alpha(struct in_addr* destination, char* next_hop_iface, router_state* rs) {

	node* n = rs->atable;
	assert(n);
	
	while (n) {
		atable_entry* ae = (atable_entry*)n->data;
		uint32_t mask = ntohl(ae->mask.s_addr);
		uint32_t ip = ntohl(ae->ip.s_addr) & mask;
		uint32_t dest_ip = ntohl(destination->s_addr) & mask;
		
		if (ip == dest_ip) {
			return
				strcmp(next_hop_iface, "eth0")?ae->alpha[0]:
				strcmp(next_hop_iface, "eth1")?ae->alpha[1]:
				strcmp(next_hop_iface, "eth2")?ae->alpha[2]:
				strcmp(next_hop_iface, "eth3")?ae->alpha[3];
		}
		
		n = n->next;
		
	}

	return -1.0;

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
