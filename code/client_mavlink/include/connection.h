#ifndef CONNECTION_H
#define CONNECTION_H

#include <stdio.h>
#include <stdint.h>
#include <netinet/in.h>
//#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

typedef struct {
    int sockfd;
    struct sockaddr_in target_addr;   // adresse de PX4 (pour l'envoi)
    struct sockaddr_in local_addr;    // notre propre adresse (pour bind)
} mav_connection_t;

// Ouvre le socket UDP, bind sur local_port, prépare target_addr vers target_ip:target_port
// Retourne 0 si succès, -1 si erreur
int connection_init(mav_connection_t *conn,
                     const char *target_ip, uint16_t target_port,
                     uint16_t local_port);

// Envoie un buffer MAVLink déjà sérialisé vers target_addr
// Retourne le nombre d'octets envoyés, -1 si erreur
int connection_send(mav_connection_t *conn, const uint8_t *buf, int len);

// Reçoit des octets bruts (bloquant), à passer ensuite à mavlink_parse_char()
// Retourne le nombre d'octets reçus, -1 si erreur, 0 si connexion fermée
int connection_recv(mav_connection_t *conn, uint8_t *buf, int buf_size);

// Ferme le socket proprement
void connection_close(mav_connection_t *conn);

#endif