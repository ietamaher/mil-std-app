#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QColor>

// Forward declare the classes it interacts with
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class SystemController;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // The constructor now takes a pointer to the SystemController
    explicit MainWindow(SystemController* controller, QWidget *parent = nullptr);
    ~MainWindow();
    // Slot to receive log messages from the controller
    void onLogMessage(const QString& message, QColor color);
private slots:
    // Slots to receive data updates from the model
    void onTargetsUpdated();
    void onServoStateUpdated();
    void onLrfDataUpdated();
    void on_getDistanceButton_clicked();
    void on_getPulseCountButton_clicked();


    // Slots automatically connected by name to buttons in the .ui file
    void on_trackButton_clicked();
    void on_stopTrackButton_clicked();

private:
    void setupConnections();
    void configureTargetTable();

    Ui::MainWindow *ui;
    SystemController* m_controller; // A pointer to the application's brain
};
#endif // MAINWINDOW_H
