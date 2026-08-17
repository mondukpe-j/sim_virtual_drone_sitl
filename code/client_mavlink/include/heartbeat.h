#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include "connection.h"
#include <stdint.h>
#include "mavlink/common/mavlink.h"

// Envoie un unique message HEARTBEAT via conn
// system_id/component_id : identifiants du système (ex: 255/190, plage GCS/companion)
int heartbeat_send(mav_connection_t *conn, uint8_t system_id, uint8_t component_id);

// Bloque jusqu'à réception d'un HEARTBEAT de PX4, remplit *out
// Retourne 0 si succès, -1 si erreur de réception
int heartbeat_wait(mav_connection_t *conn, mavlink_heartbeat_t *out,
                    uint8_t *sender_sysid, uint8_t *sender_compid);

#endif