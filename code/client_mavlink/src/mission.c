#include "../include/mission.h"
#include <stdio.h>

static int send_mission_item(mav_connection_t *conn,
                             uint8_t target_system, uint8_t target_component,
                             uint16_t seq,
                             const waypoint_t *waypoint) {
    mavlink_message_t msg;
    const int32_t lat_int = (int32_t)(waypoint->lat * 1e7);
    const int32_t lon_int = (int32_t)(waypoint->lon * 1e7);

    mavlink_msg_mission_item_int_pack(255, 190, &msg,
                                      target_system, target_component,
                                      seq,
                                      MAV_FRAME_GLOBAL_RELATIVE_ALT_INT,
                                      MAV_CMD_NAV_WAYPOINT,
                                      (seq == 0) ? 1 : 0,
                                      1,
                                      0.0f, 0.0f, 0.0f, 0.0f,
                                      lat_int, lon_int,
                                      waypoint->alt,
                                      MAV_MISSION_TYPE_MISSION);

    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const int len = mavlink_msg_to_send_buffer(buf, &msg);
    return connection_send(conn, buf, len);
}

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

    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    int len = mavlink_msg_to_send_buffer(buf, &msg);
    return connection_send(conn, buf, len);
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

int command_takeoff(mav_connection_t *conn,
                     uint8_t target_system, uint8_t target_component,
                     float altitude) {
    // Crée un message MAV_CMD_NAV_TAKEOFF et l'envoie via la connexion spécifiée.
    
    mavlink_message_t msg;
    mavlink_msg_command_long_pack(255, 190, &msg,
                                   target_system, target_component,
                                   MAV_CMD_NAV_TAKEOFF,
                                   0, // confirmation
                                   0, // param1: minimum pitch (not used)
                                   0, 0, 0, 0, 0, altitude); // autres paramètres non utilisés   

    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    int len = mavlink_msg_to_send_buffer(buf, &msg);
    return connection_send(conn, buf, len);
}


int wait_global_position(mav_connection_t *conn, mavlink_global_position_int_t *out) {
    // Bloque jusqu'à réception d'un message GLOBAL_POSITION_INT.
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
                if (msg.msgid == MAVLINK_MSG_ID_GLOBAL_POSITION_INT) {
                    mavlink_msg_global_position_int_decode(&msg, out);
                    return 0; // Succès
                }
            }
        }
    }
}

int mission_upload(mav_connection_t *conn,
                    uint8_t target_system, uint8_t target_component,
                    const waypoint_t *waypoints, int count) {
    if (conn == NULL || waypoints == NULL || count <= 0) {
        return -1;
    }

    mavlink_message_t count_msg;
    mavlink_msg_mission_count_pack(255, 190, &count_msg,
                                   target_system, target_component,
                                   (uint16_t)count,
                                   MAV_MISSION_TYPE_MISSION,
                                   0);

    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const int count_len = mavlink_msg_to_send_buffer(buf, &count_msg);
    if (connection_send(conn, buf, count_len) < 0) {
        return -1;
    }

    for (;;) {
        mavlink_mission_request_int_t req;
        uint8_t req_buf[1024];
        mavlink_message_t req_msg;
        mavlink_status_t req_status;

        int n = connection_recv(conn, req_buf, sizeof(req_buf));
        if (n <= 0) {
            return -1;
        }

        int request_found = 0;
        for (int i = 0; i < n; ++i) {
            if (mavlink_parse_char(MAVLINK_COMM_0, req_buf[i], &req_msg, &req_status)) {
                if (req_msg.msgid == MAVLINK_MSG_ID_MISSION_REQUEST_INT) {
                    mavlink_msg_mission_request_int_decode(&req_msg, &req);
                    request_found = 1;

                    if (req.seq >= (uint16_t)count) {
                        fprintf(stderr, "MISSION_REQUEST_INT seq hors limites: %u/%d\n",
                                req.seq, count);
                        return -1;
                    }

                    if (send_mission_item(conn, target_system, target_component, req.seq,
                                          &waypoints[req.seq]) < 0) {
                        fprintf(stderr, "Échec d'envoi du waypoint %u\n", req.seq);
                        return -1;
                    }

                    if (req.seq == (uint16_t)(count - 1)) {
                        goto wait_ack;
                    }
                }
            }
        }

        if (!request_found) {
            continue;
        }
    }

wait_ack:
    while (1) {
        uint8_t ack_buf[1024];
        mavlink_message_t ack_msg;
        mavlink_status_t ack_status;
        int n = connection_recv(conn, ack_buf, sizeof(ack_buf));
        if (n <= 0) {
            return -1;
        }

        for (int i = 0; i < n; ++i) {
            if (mavlink_parse_char(MAVLINK_COMM_0, ack_buf[i], &ack_msg, &ack_status)) {
                if (ack_msg.msgid == MAVLINK_MSG_ID_MISSION_ACK) {
                    mavlink_mission_ack_t ack;
                    mavlink_msg_mission_ack_decode(&ack_msg, &ack);
                    printf("MISSION_ACK reçu: type=%u target_system=%u target_component=%u\n",
                           ack.type, ack.target_system, ack.target_component);
                    return (ack.type == MAV_MISSION_ACCEPTED) ? 0 : -1;
                }
            }
        }
    }
}

int command_mission_start(mav_connection_t *conn,
                          uint8_t target_system, uint8_t target_component) {
    mavlink_message_t msg;
    // Le mode AUTO.MISSION doit être actif côté PX4 pour que la mission s'exécute réellement.
    mavlink_msg_command_long_pack(255, 190, &msg,
                                   target_system, target_component,
                                   MAV_CMD_MISSION_START,
                                   0,
                                   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const int len = mavlink_msg_to_send_buffer(buf, &msg);
    if (connection_send(conn, buf, len) < 0) {
        return -1;
    }

    mavlink_command_ack_t ack;
    if (wait_command_ack(conn, MAV_CMD_MISSION_START, &ack) != 0) {
        return -1;
    }

    return (ack.result == MAV_RESULT_ACCEPTED) ? 0 : -1;
}

int wait_mission_current(mav_connection_t *conn, mavlink_mission_current_t *out) {
    uint8_t buf[1024];
    mavlink_message_t msg;
    mavlink_status_t status;

    while (1) {
        int n = connection_recv(conn, buf, sizeof(buf));
        if (n <= 0) {
            return -1;
        }

        for (int i = 0; i < n; ++i) {
            if (mavlink_parse_char(MAVLINK_COMM_0, buf[i], &msg, &status)) {
                if (msg.msgid == MAVLINK_MSG_ID_MISSION_CURRENT) {
                    mavlink_msg_mission_current_decode(&msg, out);
                    return 0;
                }
            }
        }
    }
}

int wait_mission_item_reached(mav_connection_t *conn, mavlink_mission_item_reached_t *out) {
    uint8_t buf[1024];
    mavlink_message_t msg;
    mavlink_status_t status;

    while (1) {
        int n = connection_recv(conn, buf, sizeof(buf));
        if (n <= 0) {
            return -1;
        }

        for (int i = 0; i < n; ++i) {
            if (mavlink_parse_char(MAVLINK_COMM_0, buf[i], &msg, &status)) {
                if (msg.msgid == MAVLINK_MSG_ID_MISSION_ITEM_REACHED) {
                    mavlink_msg_mission_item_reached_decode(&msg, out);
                    return 0;
                }
            }
        }
    }
}

int command_rtl(mav_connection_t *conn,
                 uint8_t target_system, uint8_t target_component) {
    mavlink_message_t msg;
    mavlink_msg_command_long_pack(255, 190, &msg,
                                   target_system, target_component,
                                   MAV_CMD_NAV_RETURN_TO_LAUNCH,
                                   0, // confirmation
                                   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f); // autres paramètres non utilisés 

    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const int len = mavlink_msg_to_send_buffer(buf, &msg);
    return connection_send(conn, buf, len);
}