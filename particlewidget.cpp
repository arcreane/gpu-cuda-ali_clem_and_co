#include "ParticleWidget.h"
#include <QPainter>
#include <QRandomGenerator>
#include <QCursor>
#include <QApplication>
#include <cmath>

ParticleWidget::ParticleWidget(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background-color: black;");
    setMouseTracking(true);

    m_bounciness = 0.5f;
    m_friction = 0.98f;

    initParticles(4000);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ParticleWidget::updateParticles);
    m_timer->start(16);

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

void ParticleWidget::setFriction(int value)
{
    // Le slider va de 0 (Glissant) à 100 (Visqueux)
    // On veut un facteur entre 1.0 (pas de frein) et 0.90 (frein fort)
    // Inversion : plus la valeur est haute, plus le chiffre est bas (donc ça freine)
    m_friction = 1.0f - (value / 1000.0f);
}

void ParticleWidget::setBounciness(int value)
{
    m_bounciness = value / 100.0f;
}

void ParticleWidget::explode()
{
    for (Particle &p : m_particles) {
        float vx = QRandomGenerator::global()->bounded(40.0) - 20.0;
        float vy = QRandomGenerator::global()->bounded(40.0) - 20.0;
        p.velocity = QPointF(vx, vy);
    }
}

void ParticleWidget::updateParticles()
{
    // --- 1. Gestion Souris (Trou Noir / Répulsion) ---
    QPointF mousePos = mapFromGlobal(QCursor::pos());
    QPointF mouseVel = mousePos - m_lastMousePos;
    m_lastMousePos = mousePos;
    float mouseSpeed = std::sqrt(std::pow(mouseVel.x(), 2) + std::pow(mouseVel.y(), 2));

    const float friction = 0.98f; // Moins de friction pour bien voir les rebonds
    const float interactionRadius = 150.0f;
    const float forceFactor = 0.6f;
    bool isBlackHoleActive = (QApplication::mouseButtons() & Qt::LeftButton);

    // --- 2. Préparation de la Grille de Collision ---
    // Taille d'une cellule = taille de particule (env 4px) * 2 pour être large
    int cellSize = 6;
    int gridW = (width() / cellSize) + 1;
    int gridH = (height() / cellSize) + 1;

    // Vecteur de vecteurs d'index (stocke qui est dans quelle case)
    // C'est un peu coûteux à réallouer à chaque frame, mais le plus simple à comprendre
    QVector<QVector<int>> grid(gridW * gridH);

    // --- 3. Boucle Principale ---
    for (int i = 0; i < m_particles.size(); ++i) {
        Particle &p = m_particles[i];

        // A. Interaction Souris (Code habituel)
        float dx = p.position.x() - mousePos.x();
        float dy = p.position.y() - mousePos.y();
        float distMouse = std::sqrt(dx*dx + dy*dy);

        if (distMouse < interactionRadius && distMouse > 1.0f) {
            float dirX = dx / distMouse;
            float dirY = dy / distMouse;
            if (isBlackHoleActive) {
                float attraction = (1.0f - (distMouse / interactionRadius)) * 2.0f;
                p.velocity.rx() -= dirX * attraction;
                p.velocity.ry() -= dirY * attraction;
            } else if (mouseSpeed > 0.1f) {
                float repulsion = (1.0f - (distMouse / interactionRadius)) * mouseSpeed * forceFactor;
                p.velocity.rx() += dirX * repulsion;
                p.velocity.ry() += dirY * repulsion;
            }
        }

        // B. Application Mouvement
        p.position += p.velocity;
        p.velocity *= m_friction;

        // C. Rebond Murs
        if (p.position.x() < 0) { p.position.setX(0); p.velocity.setX(-p.velocity.x()); }
        else if (p.position.x() > width()) { p.position.setX(width()); p.velocity.setX(-p.velocity.x()); }
        if (p.position.y() < 0) { p.position.setY(0); p.velocity.setY(-p.velocity.y()); }
        else if (p.position.y() > height()) { p.position.setY(height()); p.velocity.setY(-p.velocity.y()); }

        // D. Enregistrement dans la grille pour collisions
        int gx = static_cast<int>(p.position.x()) / cellSize;
        int gy = static_cast<int>(p.position.y()) / cellSize;

        // Sécurité pour ne pas sortir du tableau
        if (gx >= 0 && gx < gridW && gy >= 0 && gy < gridH) {
            grid[gy * gridW + gx].append(i);
        }
    }

    // --- 4. Résolution des Collisions (Via la Grille) ---
    // Rayon d'une particule (approximatif pour un rectangle de 3px)
    float particleRadius = 1.5f;
    float minDist = particleRadius * 2.0f; // Distance min avant collision (3.0f)
    float minDistSq = minDist * minDist;

    // Pour chaque cellule de la grille
    for (int i = 0; i < grid.size(); ++i) {
        const QVector<int> &cell = grid[i];
        if (cell.isEmpty()) continue;

        // Pour chaque particule dans cette cellule
        for (int j = 0; j < cell.size(); ++j) {
            int p1_idx = cell[j];
            Particle &p1 = m_particles[p1_idx];

            // Vérifier contre les autres particules de la MÊME cellule
            for (int k = j + 1; k < cell.size(); ++k) {
                int p2_idx = cell[k];
                Particle &p2 = m_particles[p2_idx];

                float dx = p1.position.x() - p2.position.x();
                float dy = p1.position.y() - p2.position.y();
                float distSq = dx*dx + dy*dy;

                // COLLISION DÉTECTÉE !
                if (distSq < minDistSq && distSq > 0.001f) {
                    float dist = std::sqrt(distSq);

                    // 1. Repousser les particules pour qu'elles ne se chevauchent pas
                    float overlap = (minDist - dist) * 0.5f; // Chacune recule de moitié
                    float nx = dx / dist; // Vecteur normal normalisé
                    float ny = dy / dist;

                    p1.position.rx() += nx * overlap;
                    p1.position.ry() += ny * overlap;
                    p2.position.rx() -= nx * overlap;
                    p2.position.ry() -= ny * overlap;

                    // 2. Échange d'énergie (Rebond)
                    // Formule simplifiée de collision élastique 1D sur le vecteur normal
                    float vRelX = p1.velocity.x() - p2.velocity.x();
                    float vRelY = p1.velocity.y() - p2.velocity.y();
                    float velAlongNormal = vRelX * nx + vRelY * ny;

                    // Si elles s'éloignent déjà, on ne fait rien
                    if (velAlongNormal > 0) continue;

                    // Application de l'impulsion
                    float j = -(1.0f + m_bounciness) * velAlongNormal;
                    j /= 2.0f; // Car masses égales (1/m1 + 1/m2 = 2)

                    float impulseX = j * nx;
                    float impulseY = j * ny;

                    p1.velocity.rx() += impulseX;
                    p1.velocity.ry() += impulseY;
                    p2.velocity.rx() -= impulseX;
                    p2.velocity.ry() -= impulseY;
                }
            }
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
        painter.drawRect(QRectF(p.position.x(), p.position.y(), 3, 3));
    }
}
