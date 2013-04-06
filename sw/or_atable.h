/*
 * Authors: NGRP
 * Date: 04/2013
 *
 */
 
#ifndef OR_ATABLE_H_
#define OR_ATABLE_H_

#include "or_data_types.h"
#include "sr_base_internal.h"

double get_alpha(struct in_addr* destination, struct in_addr* mask, char* next_hop_iface, router_state* rs);

node* get_atable_entry(struct in_addr* destination, struct in_addr* mask, router_state* rs);
int add_atable_entry(struct in_addr* destination, struct in_addr* mask, struct in_addr* next_hop_ip, double* alpha, router_state* rs);
int del_atable_entry(struct in_addr* destination, struct in_addr* mask, router_state* rs);
int sprint_atable_entry(node* n, unsigned int index);
int update_atable_entry(struct in_addr* destination, struct in_addr* mask, struct in_addr* next_hop_ip, double* alpha, node* n);

int compute_atable(router_state* rs);
int delete_atable(router_state* rs);
int sprint_atable(router_state* rs);

void lock_atable_rd(router_state *rs);
void lock_atable_wr(router_state *rs);
void unlock_atable(router_state *rs);

#endif /*OR_ATABLE_H_*/
