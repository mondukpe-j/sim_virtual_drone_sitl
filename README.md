# sim_virtual_drone_sitl
# Client MAVLink / PX4 SITL

Pile de simulation PX4 avec un client MAVLink bas niveau en C et un script équivalent en Python (MAVSDK).

## Construire et lancer le conteneur

```bash
cd volet1/
docker compose build
docker compose run px4-sitl
```

Le dossier `../src` de l'hôte est monté sur `/workspace` dans le conteneur — c'est là que vit le code (persistant entre les sessions).

## Récupérer PX4-Autopilot (une seule fois)

Dans le conteneur, si ce n'est pas déjà fait :

```bash
cd /workspace
git clone --recursive https://github.com/PX4/PX4-Autopilot.git
```

## Lancer le simulateur (SITL)

Dans un premier terminal du conteneur :

```bash
cd /workspace/PX4-Autopilot
HEADLESS=1 make px4_sitl jmavsim
```

Le prompt `pxh>` confirme que PX4 tourne. Le laisser actif dans ce terminal.

## Compiler et lancer le client C

Dans un second terminal (`docker exec -it <nom_du_conteneur> bash`) :

```bash
cd /workspace/client_mavlink
make
./mavlink_client
```

Le programme enchaîne connexion, armement, décollage, mission (16 points de passage) et retour au point de décollage. La progression s'affiche dans le terminal.

## Lancer le script Python (MAVSDK)

```bash
cd /workspace/client_mavlink/src/python
python3 mission.py
```

Même séquence que le client C, avec l'API MAVSDK à la place du protocole MAVLink manuel.

## Remarques

- Le SITL doit tourner *avant* de lancer le client C ou le script Python.
- Le mapping de ports utilisé : le client envoie sur `14580` et écoute sur `14540` (instance MAVLink "Onboard" de PX4).
- Pour relancer un test proprement, il est conseillé de redémarrer le SITL entre deux essais (une mission déjà exécutée reste en mémoire tant que le SITL n'est pas redémarré).
