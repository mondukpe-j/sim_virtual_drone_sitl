#ifndef MISSION_H
#define MISSION_H

#include "connection.h"
#include "mavlink/common/mavlink.h"

typedef struct {
    double lat;      // degrés
    double lon;      // degrés
    float alt;       // mètres, relative au home
} waypoint_t;

// Envoie MAV_CMD_COMPONENT_ARM_DISARM. arm=1 pour armer, arm=0 pour désarmer.
int command_arm(mav_connection_t *conn,
                 uint8_t target_system, uint8_t target_component,
                 uint8_t arm);

// Bloque jusqu'à réception d'un COMMAND_ACK correspondant à expected_command.
// Retourne 0 si succès (et remplit *out), -1 en cas d'erreur de réception.
int wait_command_ack(mav_connection_t *conn, uint16_t expected_command,
                      mavlink_command_ack_t *out);

// Envoie MAV_CMD_NAV_TAKEOFF avec l'altitude cible (relative au home, en mètres)
int command_takeoff(mav_connection_t *conn,
                     uint8_t target_system, uint8_t target_component,
                     float altitude);

// Vérification _ Bloque jusqu'à réception d'un GLOBAL_POSITION_INT, remplit *out
int wait_global_position(mav_connection_t *conn, mavlink_global_position_int_t *out);

// Envoie une liste de waypoints à PX4 via le protocole MISSION_COUNT -> REQUEST_INT -> ITEM_INT -> ACK.
// Retourne 0 si la mission est acceptée, -1 sinon.
int mission_upload(mav_connection_t *conn,
                    uint8_t target_system, uint8_t target_component,
                    const waypoint_t *waypoints, int count);

// Déclenche l'exécution de la mission chargée sur PX4.
int command_mission_start(mav_connection_t *conn,
                          uint8_t target_system, uint8_t target_component);

// Attends un MESSAGE MISSION_CURRENT et remplit *out.
int wait_mission_current(mav_connection_t *conn, mavlink_mission_current_t *out);

// Attends un MESSAGE MISSION_ITEM_REACHED et remplit *out.
int wait_mission_item_reached(mav_connection_t *conn, mavlink_mission_item_reached_t *out);

// Envoie MAV_CMD_NAV_RETURN_TO_LAUNCH — PX4 gère seul le retour et l'atterrissage au point de départ
int command_rtl(mav_connection_t *conn,
                 uint8_t target_system, uint8_t target_component);

#endif