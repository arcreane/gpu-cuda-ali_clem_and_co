#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QVBoxLayout> // Pour empiler les widgets
#include <QPushButton> // Pour créer le bouton

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 1. Widget qui servira de conteneur pour tout le reste
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget); // On l'attache à la fenêtre principale

    // 2. Layout Vertical
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 3. Ecran de particules
    ParticleWidget *particleDisplay = new ParticleWidget(this);
    layout->addWidget(particleDisplay); // Il prendra toute la place disponible par défaut

    // 4. Bouton Chaos
    QPushButton *chaosButton = new QPushButton("CHAOS", this);
    chaosButton->setFixedHeight(60); // Hauteur du bouton

    chaosButton->setStyleSheet(
        "QPushButton { "
        "  background-color: #222; "
        "  color: white; "
        "  font-weight: bold; "
        "  font-size: 18px; "
        "  border: none; "
        "  border-top: 1px solid #444;"
        "} "
        "QPushButton:hover { background-color: #333; } "
        "QPushButton:pressed { background-color: #800; color: #FFAA00; }"
        );

    layout->addWidget(chaosButton); // On l'ajoute en bas du layout

    // 5. CONNECTER LE BOUTON A L'ACTION
    // Quand on clique sur chaosButton -> on lance particleDisplay->explode()
    connect(chaosButton, &QPushButton::clicked, particleDisplay, &ParticleWidget::explode);

    // Taille de démarrage
    resize(1000, 800);
}

MainWindow::~MainWindow()
{
    delete ui;
}
