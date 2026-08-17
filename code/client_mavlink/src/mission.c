#include "../include/mission.h"

int command_arm(mav_connection_t *conn,
                 uint8_t target_system, uint8_t target_component,
                 uint8_t arm) {
    // Crée un message MAV_CMD_COMPONENT_ARM_DISARM et l'envoie via la connexion spécifiée.
    
    mavlink_message_t msg;
    mavlink_msg_command_long_pack(255, 190, &msg,
                                   target_system, target_component,
                                   MAV_CMD_COMPONENT_ARM_DISARM,
                                   0, // confirmation
                                   arm ? 1.0f : 0.0f, // param1: arm/disarm
                                   0, 0, 0, 0, 0, 0); // autres paramètres non utilisés   
     return mav_connection_send_message(conn, &msg);
}

int wait_command_ack(mav_connection_t *conn, uint16_t expected_command,
                      mavlink_command_ack_t *out) {
    // Bloque jusqu'à réception d'un message COMMAND_ACK correspondant à expected_command.
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
                if (msg.msgid == MAVLINK_MSG_ID_COMMAND_ACK) {
                    mavlink_msg_command_ack_decode(&msg, out);
                    if (out->command == expected_command) {
                        return 0; // Succès
                    }
                }
            }
        }
    }
}