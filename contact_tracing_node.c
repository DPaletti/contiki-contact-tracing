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
#include "mqtt-client.h"

#include <stdio.h> /* For printf() */
#include <rpl-neighbor.h>
#include <contiki-net.h>

#define LOG_LEVEL LOG_LEVEL_INFO
#define MAXIMUM_NODE_ID_SIZE 120

#define CONFIG_ORG_ID_LEN        32
#define CONFIG_TYPE_ID_LEN       32
#define CONFIG_AUTH_TOKEN_LEN    32
#define CONFIG_EVENT_TYPE_ID_LEN 32
#define CONFIG_CMD_TYPE_LEN       8
#define CONFIG_IP_ADDR_STR_LEN   64

#ifdef MQTT_CLIENT_CONF_ORG_ID
#define MQTT_CLIENT_ORG_ID MQTT_CLIENT_CONF_ORG_ID
#else
#define MQTT_CLIENT_ORG_ID "quickstart"
#endif
/*---------------------------------------------------------------------------*/
/* MQTT token */
#ifdef MQTT_CLIENT_CONF_AUTH_TOKEN
#define MQTT_CLIENT_AUTH_TOKEN MQTT_CLIENT_CONF_AUTH_TOKEN
#else
#define MQTT_CLIENT_AUTH_TOKEN "AUTHTOKEN"
#endif
/*---------------------------------------------------------------------------*/
#if MQTT_CLIENT_WITH_IBM_WATSON
/* With IBM Watson support */
static const char *broker_ip = "0064:ff9b:0000:0000:0000:0000:b8ac:7cbd";
#define MQTT_CLIENT_USERNAME "use-token-auth"

#else /* MQTT_CLIENT_WITH_IBM_WATSON */
/* Without IBM Watson support. To be used with other brokers, e.g. Mosquitto */
static const char *broker_ip = MQTT_CLIENT_BROKER_IP_ADDR;

#ifdef MQTT_CLIENT_CONF_USERNAME
#define MQTT_CLIENT_USERNAME MQTT_CLIENT_CONF_USERNAME
#else
#define MQTT_CLIENT_USERNAME "use-token-auth"
#endif


// RPL STATICS
static int last_table_row= -1;

// MQTT STATICS
static const mqtt_client_extension_t *mqtt_client_extensions[] = { NULL };
static const uint8_t mqtt_client_extension_count = 0;

typedef struct mqtt_client_config {
    char org_id[CONFIG_ORG_ID_LEN];
    char type_id[CONFIG_TYPE_ID_LEN];
    char auth_token[CONFIG_AUTH_TOKEN_LEN];
    char event_type_id[CONFIG_EVENT_TYPE_ID_LEN];
    char broker_ip[CONFIG_IP_ADDR_STR_LEN];
    char cmd_type[CONFIG_CMD_TYPE_LEN];
    clock_time_t pub_interval;
    int def_rt_ping_interval;
    uint16_t broker_port;
} mqtt_client_config_t;
static mqtt_client_config_t conf;

/*---------------------------------------------------------------------------*/
PROCESS(contact_tracing_process, "Contact Tracing Process");
AUTOSTART_PROCESSES(&contact_tracing_process);

/*---------------------------------------------------------------------------*/
static int
init_config()
{
    /* Populate configuration with default values */
    memset(&conf, 0, sizeof(mqtt_client_config_t));

    memcpy(conf.org_id, MQTT_CLIENT_ORG_ID, strlen(MQTT_CLIENT_ORG_ID));
    memcpy(conf.type_id, DEFAULT_TYPE_ID, strlen(DEFAULT_TYPE_ID));
    memcpy(conf.auth_token, MQTT_CLIENT_AUTH_TOKEN,
           strlen(MQTT_CLIENT_AUTH_TOKEN));
    memcpy(conf.event_type_id, DEFAULT_EVENT_TYPE_ID,
           strlen(DEFAULT_EVENT_TYPE_ID));
    memcpy(conf.broker_ip, broker_ip, strlen(broker_ip));
    memcpy(conf.cmd_type, DEFAULT_SUBSCRIBE_CMD_TYPE, 1);

    conf.broker_port = DEFAULT_BROKER_PORT;
    conf.pub_interval = DEFAULT_PUBLISH_INTERVAL;
    conf.def_rt_ping_interval = DEFAULT_RSSI_MEAS_INTERVAL;

    return 1;
}
/*---------------------------------------------------------------------------*/
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
