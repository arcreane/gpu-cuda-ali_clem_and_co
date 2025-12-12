#ifndef PARTICLEWIDGET_H
#define PARTICLEWIDGET_H

#include "Particle_types.h"
#include <QWidget>
#include <QVector>
#include <QPointF>
#include <QTimer>

class CudaEngine; // Déclaration anticipée de CudaEngine

class ParticleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ParticleWidget(QWidget *parent = nullptr);
    ~ParticleWidget(); // <--- AJOUT de la déclaration du destructeur

public slots:

    void explode();
    void setBounciness(int value);
    // NOUVEAU : Slot pour la friction
    void setFriction(int value);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void requestSimulation();

private:
    QVector<Particle> m_particles;
    QTimer *m_timer;
    QPointF m_lastMousePos;


    float m_bounciness;
    float m_friction;

    void initParticles(int count);
    CudaEngine *m_cudaEngine;
};

#endif // PARTICLEWIDGET_H
