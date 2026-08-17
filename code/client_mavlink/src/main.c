#include "../include/connection.h"
#include "../include/heartbeat.h"
#include "../include/mission.h"
#include <stdio.h>
#include <unistd.h>

int main(void) {
    static const waypoint_t mission_waypoints[] = {
        // Ligne 1 (Sud -> Nord)
        { 47.3970000, 8.5440000, 10.0f },
        { 47.3980000, 8.5440000, 10.0f },
        { 47.3990000, 8.5440000, 10.0f },
        { 47.4000000, 8.5440000, 10.0f },

        // Ligne 2 (Nord -> Sud)
        { 47.4000000, 8.5450000, 12.0f },
        { 47.3990000, 8.5450000, 12.0f },
        { 47.3980000, 8.5450000, 12.0f },
        { 47.3970000, 8.5450000, 12.0f },

        // Ligne 3 (Sud -> Nord)
        { 47.3970000, 8.5460000, 15.0f },
        { 47.3980000, 8.5460000, 15.0f },
        { 47.3990000, 8.5460000, 15.0f },
        { 47.4000000, 8.5460000, 15.0f },

        // Ligne 4 (Nord -> Sud)
        { 47.4000000, 8.5470000, 10.0f },
        { 47.3990000, 8.5470000, 10.0f },
        { 47.3980000, 8.5470000, 10.0f },
        { 47.3970000, 8.5470000, 10.0f }
    };

    mav_connection_t conn;
    if (connection_init(&conn, "127.0.0.1", 14580, 14540) != 0) {
        printf("Échec de connection_init\n");
        return 1;
    }

    // --- Heartbeat ---
    if (heartbeat_send(&conn, 255, 190) < 0) {
        printf("Échec de l'envoi du heartbeat\n");
        return 1;
    }
    printf("Heartbeat envoyé, en attente de la réponse de PX4...\n");

    mavlink_heartbeat_t hb;
    uint8_t target_sysid, target_compid;
    if (heartbeat_wait(&conn, &hb, &target_sysid, &target_compid) != 0) {
        printf("Erreur de réception du heartbeat\n");
        return 1;
    }
    printf("Heartbeat reçu ! sysid=%d compid=%d base_mode=%d\n",
           target_sysid, target_compid, hb.base_mode);

    // --- Arming ---
    if (command_arm(&conn, target_sysid, target_compid, 1) < 0) {
        printf("Échec de l'envoi de la commande ARM\n");
        return 1;
    }
    mavlink_command_ack_t arm_ack;
    if (wait_command_ack(&conn, MAV_CMD_COMPONENT_ARM_DISARM, &arm_ack) != 0) {
        printf("Erreur de réception de l'ACK d'arming\n");
        return 1;
    }
    printf("ARM ACK reçu ! result=%d\n", arm_ack.result);
    if (arm_ack.result != MAV_RESULT_ACCEPTED) {
        printf("Arming refusé, abandon\n");
        return 1;
    }

    // --- Calcul de l'altitude cible (AMSL = altitude actuelle + montée voulue) ---
    mavlink_global_position_int_t current_pos;
    if (wait_global_position(&conn, &current_pos) != 0) {
        printf("Impossible de lire la position actuelle\n");
        return 1;
    }
    float current_alt_amsl = current_pos.alt / 1000.0f;
    float target_alt_amsl = current_alt_amsl + 10.0f;
    printf("Altitude actuelle AMSL: %.2f m, cible AMSL: %.2f m\n",
           current_alt_amsl, target_alt_amsl);

    // --- Takeoff ---
    if (command_takeoff(&conn, target_sysid, target_compid, target_alt_amsl) < 0) {
        printf("Échec de l'envoi du TAKEOFF\n");
        return 1;
    }
    mavlink_command_ack_t takeoff_ack;
    if (wait_command_ack(&conn, MAV_CMD_NAV_TAKEOFF, &takeoff_ack) != 0) {
        printf("Erreur de réception de l'ACK takeoff\n");
        return 1;
    }
    printf("TAKEOFF ACK reçu ! result=%d\n", takeoff_ack.result);
    if (takeoff_ack.result != MAV_RESULT_ACCEPTED) {
        printf("Takeoff refusé, abandon\n");
        return 1;
    }

    // --- Mission upload ---
    printf("Upload de la mission (%zu waypoints)...\n", sizeof(mission_waypoints) / sizeof(mission_waypoints[0]));
    if (mission_upload(&conn, target_sysid, target_compid,
                       mission_waypoints,
                       (int)(sizeof(mission_waypoints) / sizeof(mission_waypoints[0]))) != 0) {
        printf("Erreur d'upload de la mission\n");
        connection_close(&conn);
        return 1;
    }
    printf("Mission uploadée et acceptée par PX4\n");
    sleep(3);

    // --- Démarrage de la mission ---
    if (command_mission_start(&conn, target_sysid, target_compid) != 0) {
        printf("Erreur au démarrage de la mission\n");
        connection_close(&conn);
        return 1;
    }
    printf("Mission démarrée\n");

    // --- Suivi de la progression ---
    int mission_count = (int)(sizeof(mission_waypoints) / sizeof(mission_waypoints[0]));
    printf("Surveillance de la progression de la mission...\n");

    int mission_completed = 0;
    while (!mission_completed) {
        mavlink_mission_item_reached_t reached;
        if (wait_mission_item_reached(&conn, &reached) == 0) {
            printf("  MISSION_ITEM_REACHED: seq=%u\n", reached.seq);
            if (reached.seq == (uint16_t)(mission_count - 1)) {
                mission_completed = 1;
            }
        }
    }

    printf("Dernier waypoint atteint, déclenchement du RTL...\n");
    if (command_rtl(&conn, target_sysid, target_compid) < 0) {
        printf("Échec de l'envoi du RTL\n");
        return 1;
    }
    mavlink_command_ack_t rtl_ack;
    if (wait_command_ack(&conn, MAV_CMD_NAV_RETURN_TO_LAUNCH, &rtl_ack) == 0) {
        printf("RTL ACK reçu ! result=%d\n", rtl_ack.result);
    }


    printf("Attente de l'atterrissage...\n");
    int landed = 0;
    while (!landed) {
        mavlink_heartbeat_t final_hb;
        uint8_t s, c;
        if (heartbeat_wait(&conn, &final_hb, &s, &c) == 0) {
            if (!(final_hb.base_mode & MAV_MODE_FLAG_SAFETY_ARMED)) {
                printf("Drone désarmé — atterrissage confirmé\n");
                landed = 1;
            }
        }
    }

    connection_close(&conn);
    return 0;
}