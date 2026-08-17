#include "../include/connection.h"
#include "../include/heartbeat.h"
#include <stdio.h>

int main(void) {
    mav_connection_t conn;
    if (connection_init(&conn, "127.0.0.1", 14580, 14540) != 0) {
        printf("Échec de connection_init\n");
        return 1;
    }

    if (heartbeat_send(&conn, 255, 190) < 0) {
        printf("Échec de l'envoi du heartbeat\n");
        return 1;
    }
    printf("Heartbeat envoyé, en attente de la réponse de PX4...\n");

    mavlink_heartbeat_t hb;
    if (heartbeat_wait(&conn, &hb) == 0) {
        printf("Heartbeat reçu ! system_status=%d, base_mode=%d, custom_mode=%u\n",
               hb.system_status, hb.base_mode, hb.custom_mode);
    } else {
        printf("Erreur de réception\n");
    }

    connection_close(&conn);
    return 0;
}