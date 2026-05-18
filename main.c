#include <stdio.h>
#include <stdbool.h>
// Inclusion de la bibliothèque DGI (le chemin exact dépend de votre école)
#include <dgi.h> 

#include "player.h"
#include "map.h"
#include "network.h"

int main() {
    // 1. Initialisation de la fenêtre graphique avec DGI
    // Note : Les noms des fonctions (InitWindow, UpdateDisplay...) peuvent 
    // légèrement varier selon la version exacte de DGI fournie par votre école.
    if (!dgi_init(800, 600, "CS:GO - Free For All")) {
        printf("Erreur lors de l'initialisation de DGI.\n");
        return 1;
    }

    // 2. Initialisation du joueur (depuis player.h)
    Player p1;
    init_player(&p1, 1, 2.0f, 3.0f);
    printf("Joueur %d pret avec %d PV !\n", p1.id, p1.hp);

    bool running = true;

    // 3. Boucle de jeu principale
    while (running) {
        
        // A. Gestion des événements clavier/souris
        // dgi_update_events() met à jour l'état des touches
        dgi_update_events(); 
        
        // Exemple (à adapter selon les fonctions DGI) : 
        if (dgi_is_key_pressed(KEY_ESCAPE)) {
            running