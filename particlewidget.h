#ifndef PARTICLEWIDGET_H
#define PARTICLEWIDGET_H

#include <QWidget>
#include <QVector>
#include <QPointF>
#include <QTimer>

struct Particle {
    QPointF position;
    QPointF velocity;
};

class ParticleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ParticleWidget(QWidget *parent = nullptr);

public slots:  // <--- Vérifie que tu as bien cette section
    void updateParticles();
    void explode();

protected:
    void paintEvent(QPaintEvent *event) override;
private:
    QVector<Particle> m_particles;
    QTimer *m_timer;

    // --- NOUVEAU : Pour suivre la souris ---
    QPointF m_lastMousePos;

    void initParticles(int count);
};

#endif
