#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QSpinBox>
#include <QFrame>
#include <QFontDatabase>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // --- 1. CONFIGURATION GÉNÉRALE ET STYLE ---

    // Police système propre
    QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    font.setPointSize(10);
    QApplication::setFont(font);

    // FEUILLE DE STYLE GLOBALE (Thème Dark Sci-Fi corrigé)
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
        /* Les Labels */
        QLabel {
            color: #a0a0b0;
            font-weight: bold;
            font-size: 14px;
        }
        /* SLIDERS MODERNES */
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
            border: 1px solid #c21800;
            border-radius: 10px;
            padding: 15px;
            margin-top: 15px;
        }
        QPushButton#ChaosButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ff6a00, stop:1 #e02000);
            border: 2px solid #ffaa00;
        }
        QPushButton#ChaosButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #9e1200, stop:1 #c21800);
            padding-top: 17px;
            padding-bottom: 13px;
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
    particleDisplay->setStyleSheet("border-bottom: 2px solid #00d2ff;");
    mainLayout->addWidget(particleDisplay, 1); // Prend toute la place disponible

    // B. Le panneau de contrôle
    QWidget *controlPanel = new QWidget(this);
    controlPanel->setObjectName("ControlPanel");
    mainLayout->addWidget(controlPanel);

    QVBoxLayout *controlLayout = new QVBoxLayout(controlPanel);
    controlLayout->setContentsMargins(20, 20, 20, 20);
    controlLayout->setSpacing(15);

    // --- EN-TÊTE : TITRE + FPS ---
    QHBoxLayout *headerLayout = new QHBoxLayout();

    QLabel *titleLabel = new QLabel("PANNEAU DE CONTRÔLE", this);
    titleLabel->setStyleSheet("color: #00d2ff; font-weight: bold; font-size: 16px;");
    headerLayout->addWidget(titleLabel);

    // Label FPS
    QLabel *fpsLabel = new QLabel("FPS: --", this);
    fpsLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    fpsLabel->setStyleSheet(
        "color: #00ff00; font-family: 'Courier New', monospace; font-weight: bold; font-size: 18px; "
        "border: 1px solid #00ff00; padding: 4px 10px; border-radius: 4px; background-color: #002200;"
        );
    headerLayout->addWidget(fpsLabel);

    controlLayout->addLayout(headerLayout);

    // --- CONTROLE 1 : REBOND ---
    QLabel *labelBounce = new QLabel("INTENSITÉ DU REBOND (Élasticité)", this);
    labelBounce->setAlignment(Qt::AlignCenter);
    controlLayout->addWidget(labelBounce);

    QSlider *bounceSlider = new QSlider(Qt::Horizontal, this);
    bounceSlider->setRange(0, 150);
    bounceSlider->setValue(50);
    bounceSlider->setCursor(Qt::PointingHandCursor);
    controlLayout->addWidget(bounceSlider);

    // --- CONTROLE 2 : FRICTION ---
    QLabel *labelFriction = new QLabel("DENSITÉ DU FLUIDE (Friction)", this);
    labelFriction->setAlignment(Qt::AlignCenter);
    controlLayout->addWidget(labelFriction);

    QSlider *frictionSlider = new QSlider(Qt::Horizontal, this);
    frictionSlider->setRange(0, 100);
    frictionSlider->setValue(20);
    frictionSlider->setCursor(Qt::PointingHandCursor);
    controlLayout->addWidget(frictionSlider);

    // --- SÉPARATEUR ---
    QFrame *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setStyleSheet("background-color: #333; margin-top: 10px; margin-bottom: 10px;");
    controlLayout->addWidget(line);

    // --- LIGNE CONFIGURATION : NOMBRE + RAYON + APPLY ---
    QHBoxLayout *configLayout = new QHBoxLayout();

    // 1. Nombre de particules
    QLabel *labelCount = new QLabel("NB PARTICULES :", this);
    labelCount->setStyleSheet("color: #a0a0b0; font-weight: bold;");
    configLayout->addWidget(labelCount);

    QSpinBox *particleSpinBox = new QSpinBox(this);
    particleSpinBox->setRange(100, 100000); // Max 100k
    particleSpinBox->setSingleStep(1000);
    particleSpinBox->setValue(4000);        // Valeur par défaut
    particleSpinBox->setStyleSheet(
        "QSpinBox { background: #0a0a0a; color: #00d2ff; border: 1px solid #333; padding: 5px; font-weight: bold; min-width: 80px; }"
        "QSpinBox::up-button, QSpinBox::down-button { background: #222; }"
        );
    configLayout->addWidget(particleSpinBox);

    // 2. Taille (Rayon)
    QLabel *labelRadius = new QLabel("TAILLE :", this);
    labelRadius->setStyleSheet("color: #a0a0b0; font-weight: bold; margin-left: 15px;");
    configLayout->addWidget(labelRadius);

    QSlider *radiusSlider = new QSlider(Qt::Horizontal, this);
    radiusSlider->setRange(10, 50); // 1.0 à 5.0
    radiusSlider->setValue(15);     // Défaut 1.5
    radiusSlider->setFixedWidth(100);
    radiusSlider->setCursor(Qt::PointingHandCursor);
    configLayout->addWidget(radiusSlider);

    // 3. Bouton Apply
    QPushButton *applyBtn = new QPushButton("APPLY", this);
    applyBtn->setCursor(Qt::PointingHandCursor);
    applyBtn->setFixedWidth(100);
    applyBtn->setStyleSheet(
        "QPushButton { background-color: #006644; color: white; font-weight: bold; border: none; padding: 8px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #008855; }"
        "QPushButton:pressed { background-color: #004422; }"
        );
    configLayout->addWidget(applyBtn);

    controlLayout->addLayout(configLayout);

    // --- BOUTON CHAOS ---
    QPushButton *chaosButton = new QPushButton("🚀 LANCER LE CHAOS 💥", this);
    chaosButton->setObjectName("ChaosButton");
    chaosButton->setCursor(Qt::PointingHandCursor);
    controlLayout->addWidget(chaosButton);

    // --- 3. CONNEXIONS (SIGNALS & SLOTS) ---

    // Sliders physiques
    connect(bounceSlider, &QSlider::valueChanged, particleDisplay, &ParticleWidget::setBounciness);
    connect(frictionSlider, &QSlider::valueChanged, particleDisplay, &ParticleWidget::setFriction);

    // Bouton Chaos
    connect(chaosButton, &QPushButton::clicked, particleDisplay, &ParticleWidget::explode);

    // Bouton Apply (Configuration)
    connect(applyBtn, &QPushButton::clicked, [=]() {
        // Ordre important : d'abord le rayon, ensuite on régénère
        float r = radiusSlider->value() / 10.0f;
        particleDisplay->changeParticleCount(particleSpinBox->value());
    });

    // Mise à jour des FPS (Reçu depuis ParticleWidget)
    connect(particleDisplay, &ParticleWidget::fpsChanged, [=](int fps) {
        fpsLabel->setText(QString("FPS: %1").arg(fps));
        // Changement de couleur si ça rame (< 30 FPS)
        if (fps < 30) {
            fpsLabel->setStyleSheet("color: #ff3333; border: 1px solid #ff3333; font-weight: bold; font-size: 18px; padding: 4px 10px; border-radius: 4px; background-color: #220000;");
        } else {
            fpsLabel->setStyleSheet("color: #00ff00; border: 1px solid #00ff00; font-weight: bold; font-size: 18px; padding: 4px 10px; border-radius: 4px; background-color: #002200;");
        }
    });

    // Taille initiale de la fenêtre
    resize(1100, 900);
}

MainWindow::~MainWindow()
{
    delete ui;
}
