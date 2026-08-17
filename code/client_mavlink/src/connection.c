#include "../include/connection.h"

int connection_init(mav_connection_t *conn,
                     const char *target_ip, uint16_t target_port,
                     uint16_t local_port) {
    // Implémentation de l'initialisation de la connexion UDP
    // Cette fonction crée un socket UDP, le lie à l'adresse locale 
    // et configure l'adresse cible pour l'envoi de données.
    // Retourne 0 si succès, -1 en cas d'erreur.

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (sockfd < 0) {
        return -1; // Erreur lors de la création du socket
    }

    printf("Socket UDP créé avec succès.\n");

    conn->sockfd = sockfd;

    // Initialisation de l'adresse locale
    conn->local_addr.sin_family = AF_INET;
    conn->local_addr.sin_port = htons(local_port);
    conn->local_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind du socket à l'adresse locale
    if (bind(sockfd, (struct sockaddr *)&conn->local_addr, sizeof(conn->local_addr)) < 0) {
        close(sockfd);
        return -1; // Erreur lors du bind
    }

    // Initialisation de l'adresse cible
    conn->target_addr.sin_family = AF_INET;
    conn->target_addr.sin_port = htons(target_port);
    if (inet_pton(AF_INET, target_ip, &conn->target_addr.sin_addr) <= 0) {
        close(sockfd);
        return -1; // Erreur lors de la conversion de l'adresse IP
    }

    printf("Connexion UDP initialisée vers %s:%d sur le port local %d.\n", target_ip, target_port, local_port);
    return 0; // Succès
}

int connection_send(mav_connection_t *conn, const uint8_t *buf, int len) {
    // Implémentation de l'envoi de données via le socket UDP
    // Cette fonction envoie le buffer fourni à l'adresse cible.
    // Retourne le nombre d'octets envoyés, -1 en cas d'erreur.
    return sendto(conn->sockfd, buf, len, 0,
                  (struct sockaddr *)&conn->target_addr, sizeof(conn->target_addr));
}

int connection_recv(mav_connection_t *conn, uint8_t *buf, int buf_size) {
    // Implémentation de la réception de données via le socket UDP
    // Cette fonction reçoit des octets bruts et les stocke dans le buffer fourni.
    // Retourne le nombre d'octets reçus, -1 en cas d'erreur, 0 si la connexion est fermée.
    struct sockaddr_in sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
    return recvfrom(conn->sockfd, buf, buf_size, 0,
        (struct sockaddr *)&sender_addr, &addr_len);
}

void connection_close(mav_connection_t *conn) {
    // Implémentation de la fermeture du socket UDP
    // Cette fonction ferme le socket proprement.
    close(conn->sockfd);
    printf("Socket UDP fermé avec succès.\n");
}