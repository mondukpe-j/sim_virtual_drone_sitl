# Compilateur utilisé
CC := gcc

# Options de compilation
# -std=c11    : Utilise la norme C11
# -Wall -Wextra : Active l'ensemble des avertissements utiles
# -Iinclude   : Indique au compilateur de chercher les headers dans include/
CFLAGS := -std=c11 -Wall -Wextra -Iinclude

# Options de l'éditeur de liens
LDFLAGS :=

# Définition des dossiers et de l'exécutable
SRC_DIR := src
OBJ_DIR := build
BIN     := mavlink_client

# Détection automatique des fichiers sources (.c) et génération des noms d'objets (.o)
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Cibles "virtuelles" (qui ne représentent pas des fichiers)
.PHONY: all clean

# Cible principale par défaut
all: $(BIN)

# Édition de liens : combine les fichiers .o pour créer l'exécutable
$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Compilation : transforme chaque fichier .c en fichier .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Création automatique du dossier build/ s'il n'existe pas encore
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Nettoyage : supprime le dossier d'objets et l'exécutable
clean:
	rm -rf $(OBJ_DIR) $(BIN)