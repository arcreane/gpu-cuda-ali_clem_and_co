#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QFontDatabase>
#include <QSpinBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // --- 1. CONFIGURATION GÉNÉRALE ET STYLE ---

    QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    font.setPointSize(10);
    QApplication::setFont(font);

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

    ParticleWidget *particleDisplay = new ParticleWidget(this);
    particleDisplay->setStyleSheet("border-bottom: 2px solid #00d2ff;");
    mainLayout->addWidget(particleDisplay, 1);

    // Panneau de contrôle
    QWidget *controlPanel = new QWidget(this);
    controlPanel->setObjectName("ControlPanel");
    mainLayout->addWidget(controlPanel);

    QVBoxLayout *controlLayout = new QVBoxLayout(controlPanel);
    controlLayout->setContentsMargins(20, 20, 20, 20);
    controlLayout->setSpacing(15);

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
    frictionSlider->setRange(0, 100); // 0 = Space, 100 = Mud
    frictionSlider->setValue(20);     // Valeur par défaut (0.98)
    frictionSlider->setCursor(Qt::PointingHandCursor);
    controlLayout->addWidget(frictionSlider);

    // ... (Après tes sliders Rebond et Friction) ...

    // --- LIGNE DE SÉPARATION (Optionnel pour faire propre) ---
    QFrame *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setStyleSheet("background-color: #333;");
    controlLayout->addWidget(line);

    // --- CONTRÔLE NOMBRE DE PARTICULES ---
    QHBoxLayout *countLayout = new QHBoxLayout();

    QLabel *labelCount = new QLabel("NOMBRE DE PARTICULES :", this);
    labelCount->setStyleSheet("color: #a0a0b0; font-weight: bold;");
    countLayout->addWidget(labelCount);

    QSpinBox *particleSpinBox = new QSpinBox(this);
    particleSpinBox->setRange(100, 100000);
    particleSpinBox->setSingleStep(1000);
    particleSpinBox->setValue(5000);

    particleSpinBox->setStyleSheet(
        "QSpinBox { background: #0a0a0a; color: #00d2ff; border: 1px solid #333; padding: 5px; font-weight: bold; }"
        "QSpinBox::up-button, QSpinBox::down-button { background: #222; }"
        );
    countLayout->addWidget(particleSpinBox);

    QPushButton *applyBtn = new QPushButton("APPLY", this);
    applyBtn->setCursor(Qt::PointingHandCursor);
    applyBtn->setFixedWidth(100);

    applyBtn->setStyleSheet(
        "QPushButton { background-color: #006644; color: white; font-weight: bold; border: none; padding: 5px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #008855; }"
        "QPushButton:pressed { background-color: #004422; }"
        );
    countLayout->addWidget(applyBtn);
    controlLayout->addLayout(countLayout);

    // --- BOUTON CHAOS ---
    QPushButton *chaosButton = new QPushButton("CHAOS 💥", this);
    chaosButton->setObjectName("ChaosButton");
    chaosButton->setCursor(Qt::PointingHandCursor);
    controlLayout->addWidget(chaosButton);

    // --- 3. CONNEXIONS ---
    connect(chaosButton, &QPushButton::clicked, particleDisplay, &ParticleWidget::explode);
    connect(bounceSlider, &QSlider::valueChanged, particleDisplay, &ParticleWidget::setBounciness);

    // Connexion du nouveau slider
    connect(frictionSlider, &QSlider::valueChanged, particleDisplay, &ParticleWidget::setFriction);
    connect(applyBtn, &QPushButton::clicked, [=]() {
        particleDisplay->changeParticleCount(particleSpinBox->value());
    });

    resize(1100, 900);
}

MainWindow::~MainWindow()
{
    delete ui;
}
