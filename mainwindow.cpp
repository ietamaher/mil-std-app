#include "mainwindow.h"
#include "ui_MainWindow.h" // This is the header generated from your .ui file

#include "systemcontroller.h"
#include "systemdatamodel.h"

MainWindow::MainWindow(SystemController* controller, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_controller(controller)
{
    Q_ASSERT(m_controller != nullptr); // A MainWindow without a controller is useless

    ui->setupUi(this);
    setWindowTitle("Defense System Control");

    configureTargetTable();
    setupConnections();

    // Populate the UI with the initial state
    onTargetsUpdated();
    onServoStateUpdated();
    onLrfDataUpdated();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::configureTargetTable()
{
    ui->targetsTableWidget->setColumnCount(4);
    ui->targetsTableWidget->setHorizontalHeaderLabels({"ID", "Range (m)", "Azimuth (°)", "Speed (m/s)"});
    ui->targetsTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->targetsTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->targetsTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void MainWindow::setupConnections()
{
    SystemDataModel* model = m_controller->model();
    Q_ASSERT(model != nullptr);

    // When the model's data changes, update our UI
    connect(model, &SystemDataModel::targetsUpdated, this, &MainWindow::onTargetsUpdated);

    // --- THIS IS THE FIX ---
    // Connect to the SIGNAL 'azimuthServoStateUpdated', NOT the slot 'on...'.
    connect(model, &SystemDataModel::azimuthServoStateUpdated, this, &MainWindow::onServoStateUpdated);
    // You'll need a new slot for elevation if you want to display it.
    // connect(model, &SystemDataModel::elevationServoStateUpdated, this, &MainWindow::onElevationServoStateUpdated);

    connect(model, &SystemDataModel::lrfDataChangedForUI, this, &MainWindow::onLrfDataUpdated);

    // When the controller wants to log something, display it
    connect(m_controller, &SystemController::logMessage, this, &MainWindow::onLogMessage);
}

// --- SLOTS for receiving data ---

void MainWindow::onTargetsUpdated()
{
    // FIX: Call getRadarData() and then access the .trackedTargets member.
    auto radarData = m_controller->model()->getRadarData();
    const auto& targets = radarData.trackedTargets; // Get a reference to the hash

    ui->targetsTableWidget->setRowCount(0);

    // QHash can be iterated just like QList for this purpose.
    for (const auto& target : targets) {
        int row = ui->targetsTableWidget->rowCount();
        ui->targetsTableWidget->insertRow(row);

        ui->targetsTableWidget->setItem(row, 0, new QTableWidgetItem(QString::number(target.id)));
        ui->targetsTableWidget->setItem(row, 1, new QTableWidgetItem(QString::number(target.rangeMeters, 'f', 1)));
        ui->targetsTableWidget->setItem(row, 2, new QTableWidgetItem(QString::number(target.azimuthDegrees, 'f', 2)));
        ui->targetsTableWidget->setItem(row, 3, new QTableWidgetItem(QString::number(target.relativeSpeedMPS, 'f', 2)));
    }
}

void MainWindow::onServoStateUpdated()
{
    auto servoData = m_controller->model()->getAzimuthServoData();
    ui->azimuthLineEdit->setText(QString::number(servoData.position, 'f', 2));
    //ui->elevationLineEdit->setText(QString::number(servoData.currentElevation, 'f', 2));
    // You could add a status label as well
}

void MainWindow::on_getDistanceButton_clicked()
{
    m_controller->lrfGetSingleDistance();
}

void MainWindow::on_getPulseCountButton_clicked()
{
    m_controller->lrfGetPulseCount();
}

void MainWindow::onLrfDataUpdated()
{

        // Get the latest data from the model
        LrfData data = m_controller->model()->getLrfData();

        // Update your UI widgets (assuming you have QLineEdits named
        // distanceLineEdit, pulseCountLineEdit, etc. in your .ui file)
        ui->distanceLineEdit->setText(data.isLastRangingValid ? QString::number(data.lastDistance) : "Invalid");
        ui->pulseCountLineEdit->setText(QString::number(data.laserCount));
        ui->lrfStatusLineEdit->setText(data.isFault ? "FAULT" : (data.isConnected ? "Connected" : "Disconnected"));
        ui->temperatureLineEdit->setText(data.isTempValid ? QString::number(data.temperature) + " °C" : "N/A");

}

void MainWindow::onLogMessage(const QString& message, QColor color)
{
    ui->logTextEdit->setTextColor(color);
    ui->logTextEdit->append(message);
}

// --- SLOTS for sending commands ---

void MainWindow::on_trackButton_clicked()
{
    bool ok;
    quint32 targetId = ui->targetIdLineEdit->text().toUInt(&ok);
    if (ok) {
        m_controller->trackTarget(targetId);
    } else {
        onLogMessage("Invalid Target ID entered.", Qt::red);
    }
}

void MainWindow::on_stopTrackButton_clicked()
{
    m_controller->stopTracking();
}
