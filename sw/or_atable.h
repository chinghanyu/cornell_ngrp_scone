/*
 * Authors: David Erickson, Filip Paun
 * Date: 06/2007
 *
 */
 
#ifndef OR_ATABLE_H_
#define OR_ATABLE_H_

#include "or_data_types.h"
#include "sr_base_internal.h"

double get_alpha(struct in_addr* destination, char* next_hop_iface, router_state* rs);

void lock_atable_rd(router_state *rs);
void lock_atable_wr(router_state *rs);
void unlock_atable(router_state *rs);
