#ifndef MISSION_H
#define MISSION_H

#include "connection.h"
#include "mavlink/common/mavlink.h"

// Envoie MAV_CMD_COMPONENT_ARM_DISARM. arm=1 pour armer, arm=0 pour désarmer.
int command_arm(mav_connection_t *conn,
                 uint8_t target_system, uint8_t target_component,
                 uint8_t arm);

// Bloque jusqu'à réception d'un COMMAND_ACK correspondant à expected_command.
// Retourne 0 si succès (et remplit *out), -1 en cas d'erreur de réception.
int wait_command_ack(mav_connection_t *conn, uint16_t expected_command,
                      mavlink_command_ack_t *out);

#endif