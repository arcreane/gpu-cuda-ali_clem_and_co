#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QFontDatabase>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // --- 1. CONFIGURATION GÉNÉRALE ET STYLE ---

    QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    font.setPointSize(10);
    QApplication::setFont(font);

    // LA FEUILLE DE STYLE CORRIGÉE (Sans box-shadow)
    this->setStyleSheet(R"(
        QMainWindow {
            background-color: #121212;
        }
        QWidget#CentralWidget {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1a1a2a, stop:1 #0a0a12);
        }
        /* Zone de contrôle en bas */
        QWidget#ControlPanel {
            background-color: #1e1e24;
            border-top: 2px solid #333;
            border-top-left-radius: 15px;
            border-top-right-radius: 15px;
        }
        /* Le Label */
        QLabel {
            color: #a0a0b0;
            font-weight: bold;
            font-size: 14px;
        }
        /* SLIDER MODERNE */
        QSlider::groove:horizontal {
            border: 1px solid #333;
            height: 8px;
            background: #0a0a0a;
            margin: 2px 0;
            border-radius: 4px;
        }
        QSlider::handle:horizontal {
            background: qradialgradient(cx:0.5, cy:0.5, radius:0.7, fx:0.5, fy:0.5, stop:0.4 #00d2ff, stop:1 #005f7f);
            border: 2px solid #00d2ff;
            width: 20px;
            margin: -8px 0;
            border-radius: 11px;
        }
        QSlider::sub-page:horizontal {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #005f7f, stop:1 #00d2ff);
            border: 1px solid #333;
            height: 8px;
            border-radius: 4px;
        }
        /* BOUTON CHAOS */
        QPushButton#ChaosButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ff4e00, stop:1 #c21800);
            color: white;
            font-weight: bold;
            font-size: 18px;
            border: 1px solid #c21800; /* Bordure subtile pour remplacer l'ombre */
            border-radius: 10px;
            padding: 15px;
            margin-top: 15px;
        }
        QPushButton#ChaosButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ff6a00, stop:1 #e02000);
            border: 2px solid #ffaa00; /* Effet lumineux au survol */
        }
        QPushButton#ChaosButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #9e1200, stop:1 #c21800);
            padding-top: 17px;
            padding-bottom: 13px;
            border: 1px solid #700000;
        }
    )");

    // --- 2. CONSTRUCTION DE L'INTERFACE ---

    QWidget *centralWidget = new QWidget(this);
    centralWidget->setObjectName("CentralWidget");
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // A. L'écran de particules
    ParticleWidget *particleDisplay = new ParticleWidget(this);
    // On retire box-shadow ici aussi, on garde juste la bordure bleue
    particleDisplay->setStyleSheet("border-bottom: 2px solid #00d2ff;");
    mainLayout->addWidget(particleDisplay, 1);

    // B. Le panneau de contrôle
    QWidget *controlPanel = new QWidget(this);
    controlPanel->setObjectName("ControlPanel");
    mainLayout->addWidget(controlPanel);

    QVBoxLayout *controlLayout = new QVBoxLayout(controlPanel);
    controlLayout->setContentsMargins(20, 20, 20, 20);
    controlLayout->setSpacing(15);

    // 1. Le Label
    QLabel *label = new QLabel("INTENSITÉ DU REBOND (Élasticité)", this);
    label->setAlignment(Qt::AlignCenter);
    controlLayout->addWidget(label);

    // 2. Le Slider
    QSlider *bounceSlider = new QSlider(Qt::Horizontal, this);
    bounceSlider->setRange(0, 150);
    bounceSlider->setValue(50);
    bounceSlider->setCursor(Qt::PointingHandCursor);
    controlLayout->addWidget(bounceSlider);

    // 3. Le Bouton Chaos
    QPushButton *chaosButton = new QPushButton("CHAOS 💥", this);
    chaosButton->setObjectName("ChaosButton");
    chaosButton->setCursor(Qt::PointingHandCursor);
    controlLayout->addWidget(chaosButton);

    // --- 3. CONNEXIONS ---
    connect(chaosButton, &QPushButton::clicked, particleDisplay, &ParticleWidget::explode);
    connect(bounceSlider, &QSlider::valueChanged, particleDisplay, &ParticleWidget::setBounciness);

    resize(1100, 850);
}

MainWindow::~MainWindow()
{
    delete ui;
}
