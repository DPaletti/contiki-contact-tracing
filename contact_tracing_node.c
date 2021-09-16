/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         A very simple Contiki application showing how Contiki programs look
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"

#include <stdio.h> /* For printf() */
#include <rpl-neighbor.h>
#include <contiki-net.h>

#define LOG_LEVEL LOG_LEVEL_INFO
#define MAXIMUM_NODE_ID_SIZE 120


static int last_table_row= -1;

/*---------------------------------------------------------------------------*/
PROCESS(contact_tracing_process, "Hello world process");
AUTOSTART_PROCESSES(&contact_tracing_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(contact_tracing_process, ev, data)
{
  static struct etimer timer;

  PROCESS_BEGIN();

  /* Setup a periodic timer that expires after 2 seconds. */
  etimer_set(&timer, CLOCK_SECOND * 2);

  while(1) {
      if(curr_instance.used) {
          rpl_nbr_t *nbr = nbr_table_head(rpl_neighbors);

          int curr_rows = -1;
          while(nbr != NULL) {
              if(curr_rows < last_table_row){
                  nbr = nbr_table_next(rpl_neighbors, nbr);
                  curr_rows++;
                  continue;
              }
              char nbr_ipaddr[MAXIMUM_NODE_ID_SIZE];
              int ip_len;
              ip_len = uiplib_ipaddr_snprint(nbr_ipaddr, sizeof(nbr_ipaddr), rpl_neighbor_get_ipaddr(nbr));
              if (ip_len <= 0 || ip_len > MAXIMUM_NODE_ID_SIZE){
                  printf("IP_LEN either < 0 or too large, Failed at line 68 in contact_tracing_node.c: ip_len = %d", ip_len);
                  return 1;
              }
              printf("%s\n", nbr_ipaddr);
              nbr = nbr_table_next(rpl_neighbors, nbr);
              curr_rows++;
              last_table_row++;
          }
      }
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
      etimer_reset(&timer);
  }


  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
