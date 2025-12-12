// Particle_types.h
#ifndef PARTICLE_TYPES_H
#define PARTICLE_TYPES_H


#include <QPointF> // utilisation de la structure QPointF pour la partie CPU (Qt)

// ******************************************************
// NOUVEAU : STRUCTURE PARTICLE (Utilisée par le client Qt)
// ******************************************************
struct Particle {
    QPointF position;
    QPointF velocity;

};
// ******************************************************


// Déf de la structure QPointF pour la partie GPU (C++ pur)
struct Particle_GPU{
    // 4 floats pour la position (x,y) et la vitesse (vx, vy)
    float pos_x;
    float pos_y;
    float vel_x;
    float vel_y;

    // Si on a besoin ajoutera ici les champs (masse, rayon, etc.)
};


#endif // PARTICLE_TYPES_H
