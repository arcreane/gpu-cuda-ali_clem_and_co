// CudaEngine.cuh
#ifndef CUDA_ENGINE_H
#define CUDA_ENGINE_H

#include <QVector>
#include <QPointF>
#include <QObject>
#include <QNetworkAccessManager> // Nécessaire pour les appels HTTP
#include <QNetworkReply>
#include "Particle_types.h"

// Note: CudaEngine devient QObject pour gérer les signaux/slots réseau
class CudaEngine : public QObject {
    Q_OBJECT

public:
    explicit CudaEngine(QObject *parent = nullptr);
    ~CudaEngine();
    
    // Fonction principale appelée par ParticleWidget
    void runSimulation(
        QVector<Particle>& particles,
        const QPointF& mousePos,
        float mouseSpeed,
        float friction,
        float bounciness,
        bool isBlackHoleActive,
        int viewportWidth,
        int viewportHeight
    );

signals:
    // Signal émis lorsque le calcul distant est terminé
    void simulationFinished();

private slots:
    // Slot pour traiter la réponse du serveur Colab
    void onNetworkReply(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_netManager;
    QUrl m_serverUrl;
    
    // Stocker le pointeur vers les particules de l'hôte
    // pour les mettre à jour lors de la réception de la réponse asynchrone
    QVector<Particle>* m_hostParticles; 

    // Méthodes de sérialisation
    QByteArray serializeParticles(const QVector<Particle>& particles) const;
    void deserializeParticles(const QByteArray& data, QVector<Particle>& particles);
};

#endif // CUDA_ENGINE_H