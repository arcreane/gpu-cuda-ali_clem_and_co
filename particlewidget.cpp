#include "ParticleWidget.h"
#include <QPainter>
#include <QRandomGenerator>
#include <QCursor>
#include <cmath>
#include <QApplication>

ParticleWidget::ParticleWidget(QWidget *parent)
    : QWidget(parent)
{
    // Fond noir
    setStyleSheet("background-color: black;");

    // Activer le suivi de la souris pour l'interaction
    setMouseTracking(true);

    // Initialisation
    initParticles(5000);

    // Timer ~60 FPS
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ParticleWidget::updateParticles);
    m_timer->start(16);

    // Initialiser la dernière position de souris
    m_lastMousePos = QPointF(0, 0);
}

void ParticleWidget::initParticles(int count)
{
    m_particles.resize(count);
    for (int i = 0; i < count; ++i) {
        float x = QRandomGenerator::global()->bounded(800.0);
        float y = QRandomGenerator::global()->bounded(600.0);
        m_particles[i].position = QPointF(x, y);
        m_particles[i].velocity = QPointF(0, 0);
    }
}

void ParticleWidget::updateParticles()
{
    // 1. Gestion de la position souris
    QPointF mousePos = mapFromGlobal(QCursor::pos());
    QPointF mouseVel = mousePos - m_lastMousePos;
    m_lastMousePos = mousePos; // Mise à jour pour la prochaine frame

    // Calcul de la vitesse de la souris
    float mouseSpeed = std::sqrt(std::pow(mouseVel.x(), 2) + std::pow(mouseVel.y(), 2));

    // --- PARAMÈTRES PHYSIQUES ---
    const float friction = 0.96f;
    const float interactionRadius = 150.0f;
    const float forceFactor = 0.6f;

    // Est-ce que le clic gauche est maintenu ?
    bool isBlackHoleActive = (QApplication::mouseButtons() & Qt::LeftButton);

    for (Particle &p : m_particles) {

        // Calcul du vecteur entre la particule et la souris
        float dx = p.position.x() - mousePos.x();
        float dy = p.position.y() - mousePos.y();
        float distance = std::sqrt(dx*dx + dy*dy);

        // On interagit si la particule est proche (et pas collée à 0 pour éviter la division par zéro)
        if (distance < interactionRadius && distance > 1.0f) {

            // Direction normalisée (vecteur unitaire)
            float dirX = dx / distance;
            float dirY = dy / distance;

            // --- LOGIQUE D'INTERACTION ---
            if (isBlackHoleActive) {
                // 1. La force est plus forte quand on est proche (1.0 - ratio)
                // 2. On multiplie par 2.0 pour que l'aspiration soit puissante
                float attractionStrength = (1.0f - (distance / interactionRadius)) * 2.0f;

                // On utilise "-=" pour attirer vers la souris
                p.velocity.rx() -= dirX * attractionStrength;
                p.velocity.ry() -= dirY * attractionStrength;

            } else if (mouseSpeed > 0.1f) {
                // MODE RÉPULSION CLASSIQUE (Souris en mouvement uniquement)
                // Force proportionnelle à la vitesse de la souris
                float repulsionStrength = (1.0f - (distance / interactionRadius)) * mouseSpeed * forceFactor;

                // On utilise "+=" pour repousser
                p.velocity.rx() += dirX * repulsionStrength;
                p.velocity.ry() += dirY * repulsionStrength;
            }
        }

        // --- APPLIQUER LA PHYSIQUE ---
        p.position += p.velocity;
        p.velocity *= friction;

        // Rebond sur les murs
        if (p.position.x() < 0) {
            p.position.setX(0);
            p.velocity.setX(-p.velocity.x());
        } else if (p.position.x() > width()) {
            p.position.setX(width());
            p.velocity.setX(-p.velocity.x());
        }

        if (p.position.y() < 0) {
            p.position.setY(0);
            p.velocity.setY(-p.velocity.y());
        } else if (p.position.y() > height()) {
            p.position.setY(height());
            p.velocity.setY(-p.velocity.y());
        }
    }

    update();
}

void ParticleWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setPen(Qt::NoPen);

    painter.setBrush(QColor(255, 255, 255, 200));

    for (const Particle &p : m_particles) {
        painter.drawRect(QRectF(p.position.x(), p.position.y(), 2, 2));
    }
}

void ParticleWidget::explode()
{
    for (Particle &p : m_particles) {
        // On génère une vitesse aléatoire brutale (entre -20 et +20)
        // Cela va "casser" l'inertie actuelle et propulser la particule
        float vx = QRandomGenerator::global()->bounded(40.0) - 20.0;
        float vy = QRandomGenerator::global()->bounded(40.0) - 20.0;

        p.velocity = QPointF(vx, vy);
    }
}
