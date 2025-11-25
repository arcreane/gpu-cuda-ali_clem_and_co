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
    // NOUVEAU : Pour gérer l'intensité du rebond via le slider
    void setBounciness(int value);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<Particle> m_particles;
    QTimer *m_timer;
    QPointF m_lastMousePos;

    // NOUVEAU : Facteur de rebond (0.0 = mou, 1.0 = rebond parfait, >1.0 = énergie cinétique ajoutée)
    float m_bounciness;

    void initParticles(int count);
};

#endif // PARTICLEWIDGET_H
