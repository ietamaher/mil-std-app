QT       += core gui serialbus serialport  dbus statemachine

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    communication/modbustransport.cpp \
    devices/lrfdevice.cpp \
    devices/servodriverdevice.cpp \
    main.cpp \
    mainwindow.cpp \
    protocols/lrfprotocolparser.cpp \
    protocols/modbusprotocolparser.cpp \
    protocols/nmeaparser.cpp \
    devices/radardevice.cpp \
    communication/serialporttransport.cpp \
    systemcontroller.cpp \
    systemdatamodel.cpp \
    devices/plc42device.cpp \
    protocols/plc42protocolparser.cpp \
    devices/cameravideostreamdevice.cpp \
    devices/gyrodevice.cpp \
    protocols/gyroprotocolparser.cpp

HEADERS += \
    communication/modbustransport.h \
    devices/TemplatedDevice.h \
    data/DataTypes.h \
    ICommunicationInterface.h \
    devices/lrfdevice.h \
    devices/servodriverdevice.h \
    interfaces/IDevice.h \
    interfaces/Message.h \
    interfaces/ProtocolParser.h \
    interfaces/Transport.h \
    mainwindow.h \
    protocols/LrfMessage.h \
    protocols/ServoMessage.h \
    protocols/lrfprotocolparser.h \
    protocols/modbusprotocolparser.h \
    protocols/nmeaparser.h \
    devices/radardevice.h \
    communication/serialporttransport.h \
    protocols/radarplotmessage.h \
    systemcontroller.h \
    systemdatamodel.h \
    devices/plc42device.h \
    protocols/Plc42Message.h \
    protocols/plc42protocolparser.h \
    devices/cameravideostreamdevice.h \
    osd/osdrenderer.h \
    utils/inference.h \
    models/systemstatemodel.h \
    vpi_helpers.h \
    devices/gyrodevice.h \
    protocols/GyroMessage.h \
    protocols/gyroprotocolparser.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
