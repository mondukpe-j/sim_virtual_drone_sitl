#include "heartbeat.h"

int heartbeat_send(mav_connection_t *conn, uint8_t system_id, uint8_t component_id) {
    // Crée un message HEARTBEAT MAVLink et l'envoie via la connexion spécifiée.
    
    mavlink_message_t msg;
    mavlink_msg_heartbeat_pack(system_id, component_id, &msg,
                               MAV_TYPE_GCS, MAV_AUTOPILOT_INVALID, 0, 0, 0);
    
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    int len = mavlink_msg_to_send_buffer(buf, &msg);
    
    return connection_send(conn, buf, len);
}

int heartbeat_wait(mav_connection_t *conn, mavlink_heartbeat_t *out,
                    uint8_t *sender_sysid, uint8_t *sender_compid) {
    // Bloque jusqu'à réception d'un message HEARTBEAT MAVLink.
    // Remplit la structure out avec les données du message reçu.
    
    uint8_t buf[1024];
    mavlink_message_t msg;
    mavlink_status_t status;
    
    while (1) {
        int n = connection_recv(conn, buf, sizeof(buf));
        if (n <= 0) {
            return -1; // Erreur de réception
        }
        
        for (int i = 0; i < n; ++i) {
            if (mavlink_parse_char(MAVLINK_COMM_0, buf[i], &msg, &status)) {
                if (msg.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
                    mavlink_msg_heartbeat_decode(&msg, out);
                    *sender_sysid = msg.sysid;
                    *sender_compid = msg.compid;
                    return 0; // Succès
                }
            }
        }
    }
}