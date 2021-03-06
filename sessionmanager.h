/**
 * \file
 * <!--
 * Copyright 2015 Develer S.r.l. (http://www.develer.com/)
 * -->
 *
 * \brief SessionManager class header
 *
 * \author Aurelien Rainone <aurelien@develer.org>
 */

#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QObject>
#include <QSerialPort>

#include "connectdialog.h"

class OutputManager;

/**
 * \brief manage serial port session
 */
class SessionManager : public QObject
{
    Q_OBJECT

private:

    /// serial port instance
    QSerialPort            *serial;

    /// current session configuration
    QHash<QString, QString> curr_cfg;

    /// indicate that a connection has been initiated or is in progress
    bool        in_progress;

public:

    explicit SessionManager(QObject *parent = 0);
    ~SessionManager();

    /**
     * \brief open a serial port connection
     * \param port_cfg    serial port settings
     */
    void openSession(const QHash<QString, QString> &port_cfg);

    /**
     * \brief return true if the session is active
     */
    bool isSessionOpen() const;

    /**
     * \brief send data to serial port
     * \param data    byte array data
     */
    void sendToSerial(const QByteArray &data);

signals:

    /**
     * \brief signal emitted at the beggining of a new session
     */
    void sessionStarted();

    /**
     * \brief signal emitted when new data has been received from the serial port
     * \param data    byte array data
     */
    void dataReceived(const QByteArray &data);

private:

    /**
     * \brief read data from serial port
     */
    void readData();

    /**
     * \brief save given data to configured dump file
     */
    void saveToFile(const QByteArray &data);

    /**
     * \brief handle serial port error
     */
    void handleError(QSerialPort::SerialPortError serialPortError);
};

#endif // SESSIONMANAGER_H
