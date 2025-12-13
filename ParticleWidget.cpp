#include "ParticleWidget.h"
#include "CudaEngine.h"
#include <QPainter>
#include <QRandomGenerator>
#include <QCursor>
#include <QApplication>
#include <QObject>
#include <cmath>

ParticleWidget::ParticleWidget(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background-color: black;");
    setMouseTracking(true);

    m_bounciness = 0.5f;
    m_friction = 0.98f;

    initParticles(5000);
    m_cudaEngine = new CudaEngine(this); // Initialisation de l'Engine et connexion du signal

    // Connexion : Quand le calcul distant est fait, redessine l'UI
    connect(m_cudaEngine, &CudaEngine::simulationFinished, this, QOverload<>::of(&QWidget::update));

    m_timer = new QTimer(this);

    // Le timer appelle une version simplifiée de la fonction
    connect(m_timer, &QTimer::timeout, this, &ParticleWidget::requestSimulation);
    m_timer->start(16);

    m_lastMousePos = QPointF(0, 0);
}

void ParticleWidget::requestSimulation()
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

    // --- 2. Délégation du calcul au moteur GPU (CUDA) ---
    m_cudaEngine->runSimulation(
        m_particles,
        mousePos,
        mouseSpeed,
        m_friction,
        m_bounciness,
        isBlackHoleActive,
        width(),
        height()
        );

        // L'état de m_particles est maintenant mis à jour avec les nouvelles
        // positions / vitesses rapatriées par CudaEngine::runSimulation.
}


// AJOUT d'une fonction de nettoyage
ParticleWidget::~ParticleWidget()
{

}

void ParticleWidget::initParticles(int count)
{
    m_particles.resize(count);

    // a peu près la taille de la fenêtre.
    float initialWidth = 1200.0f;
    float initialHeight = 900.0f;

    for (int i = 0; i < count; ++i) {
        float x = QRandomGenerator::global()->bounded(initialWidth);
        float y = QRandomGenerator::global()->bounded(initialHeight);
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


void ParticleWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 200));

    for (const Particle &p : std::as_const(m_particles)) {
        painter.drawRect(QRectF(p.position.x(), p.position.y(), 3, 3));
    }
}
