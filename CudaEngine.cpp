// CudaEngine.cpp (Renommé depuis CudaEngine.cu)
#include "Particle_types.h"
#include "CudaEngine.h"
#include "moc_CudaEngine.cpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>

CudaEngine::CudaEngine(QObject *parent)
    : QObject(parent),
    m_netManager(new QNetworkAccessManager(this)),
    // Définissez ici l'URL de votre serveur Colab/Flask.
    // Cette URL devra être mise à jour avec l'adresse publique du tunnel Colab.
    m_serverUrl("https://inturned-pluteal-santa.ngrok-free.dev/simulate") 
{
    // Connecter le slot de réponse à la fin de la requête
    connect(m_netManager, &QNetworkAccessManager::finished, this, &CudaEngine::onNetworkReply);
}

CudaEngine::~CudaEngine()
{
    // Pas de nettoyage CUDA/GPU ici, c'est le serveur distant qui le fait.
}

// --- Méthode de Sérialisation (Conversion C++/Qt -> JSON) ---
QByteArray CudaEngine::serializeParticles(const QVector<Particle>& particles) const
{
    QJsonArray particleArray;
    for (const Particle& p : particles) {
        QJsonObject obj;
        obj["px"] = p.position.x();
        obj["py"] = p.position.y();
        obj["vx"] = p.velocity.x();
        obj["vy"] = p.velocity.y();
        particleArray.append(obj);
    }
    
    QJsonObject root;
    root["particles"] = particleArray;
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

// --- Fonction Principale d'Envoi ---
void CudaEngine::runSimulation(
    QVector<Particle>& particles,
    const QPointF& mousePos,
    float mouseSpeed,
    float friction,
    float bounciness,
    bool isBlackHoleActive,
    int viewportWidth,
    int viewportHeight
)
{
    // Stocker le pointeur pour mettre à jour les données à la réception
    m_hostParticles = &particles;

    // 1. Création du JSON de données initiales (positions/vitesses)
    QByteArray particlesData = serializeParticles(particles);

    // 2. Ajout des paramètres de contrôle au JSON
    QJsonObject requestObject = QJsonDocument::fromJson(particlesData).object();
    requestObject["num"] = particles.size();
    requestObject["friction"] = friction;
    requestObject["bounciness"] = bounciness;
    requestObject["width"] = viewportWidth;
    requestObject["height"] = viewportHeight;
    requestObject["mouse_x"] = mousePos.x();
    requestObject["mouse_y"] = mousePos.y();
    requestObject["mouse_speed"] = mouseSpeed;
    requestObject["attraction_mode"] = isBlackHoleActive;
    
    QByteArray finalData = QJsonDocument(requestObject).toJson(QJsonDocument::Compact);

    // 3. Envoi de la requête
    QNetworkRequest request(m_serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    m_netManager->post(request, finalData);
    
    // La fonction se termine, le calcul est maintenant ASYNCHRONE sur le serveur.
}


// --- Slot de Réception (Mise à Jour des données hôtes) ---
void CudaEngine::onNetworkReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Network Error:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);

    if (doc.isObject()) {
        QJsonArray particlesJson = doc.object()["particles"].toArray();
        if (particlesJson.size() == m_hostParticles->size()) {
            // Désérialisation et mise à jour des positions/vitesses
            for (int i = 0; i < particlesJson.size(); ++i) {
                QJsonObject obj = particlesJson.at(i).toObject();
                (*m_hostParticles)[i].position.setX(obj["px"].toDouble());
                (*m_hostParticles)[i].position.setY(obj["py"].toDouble());
                (*m_hostParticles)[i].velocity.setX(obj["vx"].toDouble());
                (*m_hostParticles)[i].velocity.setY(obj["vy"].toDouble());
            }
        }
    }
    
    // Émettre le signal pour dire à l'UI de redessiner
    emit simulationFinished();
    reply->deleteLater();
}
// Note: La désérialisation a été intégrée dans onNetworkReply pour des raisons de concision.
