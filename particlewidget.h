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

public slots:
    void updateParticles();
    void explode();
    void setBounciness(int value);
    void setFriction(int value);
    void changeParticleCount(int count);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<Particle> m_particles;
    QTimer *m_timer;
    QPointF m_lastMousePos;

    float m_bounciness;
    float m_friction;

    void initParticles(int count);
};

#endif // PARTICLEWIDGET_H
