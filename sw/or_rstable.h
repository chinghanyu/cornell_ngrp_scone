/*
 * Authors: NGRP
 * Date: 04/2013
 *
 */

#ifndef OR_RSTABLE_H_
#define OR_RSTABLE_H_

#include "or_data_types.h"
#include "sr_base_internal.h"

double get_rate(struct in_addr* destination, struct in_addr* mask, router_state* rs);

node* get_rstable_entry(struct in_addr* destination, struct in_addr* mask, router_state* rs);
int add_rstable_entry(struct in_addr* destination, struct in_addr* mask, unsigned int length, router_state* rs);
int del_rstable_entry(struct in_addr* destination, struct in_addr* mask, router_state* rs);
int sprint_rstable_entry(node* n, unsigned int index);
int update_flow_only(unsigned int length, node* n);
int update_rstable_entry(struct in_addr* destination, struct in_addr* mask, double rate, unsigned int flow, unsigned int last_flow, struct timeval* now, node* n);
int reset_rstable_entry(node* n);

void* rstable_thread(void* arg);

int compute_rstable(router_state* rs);
int delete_rstable(router_state* rs);
int sprint_rstable(router_state* rs);

void lock_rstable_rd(router_state *rs);
void lock_rstable_wr(router_state *rs);
void unlock_rstable(router_state *rs);

#endif /*OR_RSTABLE_H_*/
