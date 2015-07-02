/**
 * \file
 * <!--
 * Copyright 2015 Develer S.r.l. (http://www.develer.com/)
 * -->
 *
 * \brief main cutecom-ng window
 *
 * \author Aurelien Rainone <aurelien@develer.org>
 */

#include <algorithm>
#include <iterator>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "connectdialog.h"
#include "sessionmanager.h"
#include "outputmanager.h"

/// line ending char appended to the commands sent to the serial port
const QString LINE_ENDING = "\n";

/// maximum count of document blocks for the bootom output
const int MAX_OUTPUT_LINES = 100;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // create session and output managers
    output_mgr = new OutputManager(this);
    session_mgr = new SessionManager(this);
    connect_dlg = new ConnectDialog(this);

    // show connection dialog
    connect(ui->connectButton, &QAbstractButton::clicked, connect_dlg, &ConnectDialog::show);

    // handle reception of new data from serial port
    connect(session_mgr, &SessionManager::dataReceived, this, &MainWindow::handleDataReceived);

    // get data formatted for display and show it in output view
    connect(output_mgr, &OutputManager::dataConverted, this, &MainWindow::addDataToView);

    // get data formatted for display and show it in output view
    connect(ui->inputBox, &HistoryComboBox::lineEntered, this, &MainWindow::handleNewInput);

    // clear output manager buffer at session start
    connect(session_mgr, &SessionManager::sessionStarted, output_mgr, &OutputManager::clear);

    // clear output text
    connect(session_mgr, &SessionManager::sessionStarted, ui->mainOutput, &QPlainTextEdit::clear);

    // call openSession when user accepts/closes connection dialog
    connect(connect_dlg, &ConnectDialog::openDeviceClicked, session_mgr, &SessionManager::openSession);

    connect(ui->splitOutputBtn, &QToolButton::clicked, this, &MainWindow::toggleOutputSplitter);

    // additional configuration for bottom output
    ui->bottomOutput->hide();
    ui->bottomOutput->document()->setMaximumBlockCount(MAX_OUTPUT_LINES);
    ui->bottomOutput->viewport()->installEventFilter(this);

    // connect search related signals
    connect(ui->searchInput, &QLineEdit::returnPressed, this, &MainWindow::handleSearchNext);
    connect(ui->searchInput, &QLineEdit::textChanged, this, &MainWindow::handleSearchTextChanged);
}


MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::handleNewInput(QString entry)
{
    // if session is not open, this also keeps user input
    if (session_mgr->isSessionOpen())
    {
        entry.append(LINE_ENDING);
        session_mgr->sendToSerial(entry.toLocal8Bit());
        ui->inputBox->clearEditText();
    }
}

void MainWindow::addDataToView(const QString & textdata)
{
    // problem : QTextEdit interprets a '\r' as a new line, so if a buffer ending
    //           with '\r\n' happens to be cut in the middle, there will be 1 extra
    //           line jump in the QTextEdit. To prevent we remove ending '\r' and
    //           prepend them to the next received buffer

    // flag indicating that the previously received buffer ended with CR
    static bool prev_ends_with_CR = false;

    QString newdata;
    if (prev_ends_with_CR)
    {
        // CR was removed at the previous buffer, so now we prepend it
        newdata.append('\r');
        prev_ends_with_CR = false;
    }

    if (textdata.length() > 0)
    {
        QString::const_iterator end_cit = textdata.cend();
        if (textdata.endsWith('\r'))
        {
            // if buffer ends with CR, we don't copy it
            end_cit--;
            prev_ends_with_CR = true;
        }
        std::copy(textdata.begin(), end_cit, std::back_inserter(newdata));
    }

    if (ui->bottomOutput->isVisible())
    {
        // append text to the top output and stay at current position
        QTextCursor cursor(ui->mainOutput->document());
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(newdata);
    }
    else
    {
        // append text to the top output and scroll down
        ui->mainOutput->moveCursor(QTextCursor::End);
        ui->mainOutput->insertPlainText(newdata);
    }

    // append text to bottom output and scroll
    ui->bottomOutput->moveCursor(QTextCursor::End);
    ui->bottomOutput->insertPlainText(newdata);
}

void MainWindow::handleDataReceived(const QByteArray &data)
{
    (*output_mgr) << data;
}

void MainWindow::toggleOutputSplitter()
{
    ui->bottomOutput->setVisible(!ui->bottomOutput->isVisible());
}

bool MainWindow::eventFilter(QObject *target, QEvent *event)
{
    if (event->type() == QEvent::Wheel)
        return true;

    // base class behaviour
    return QMainWindow::eventFilter(target, event);
}

#include <QDebug>

void MainWindow::handleSearchNext()
{
    // there is a point having 'find next' feature in 'browsing mode only'
    // indeed in standard mode we are scrolling down automatically when
    // new text is received
    if (ui->bottomOutput->isVisible())
    {
        const QString searched(ui->searchInput->text());

        if (!searched.isEmpty())
        {
            QTextCursor pos = ui->mainOutput->document()->find(searched);
            if (!pos.isNull())
            {
                // CONTINUER ICI, DEFINIR COMME SELECTIONNE LE TEXTE 'searched'

            }
        }

    }
}

void MainWindow::handleSearchTextChanged(const QString & text)
{
    // un-highlight all text previously highlighted
    foreach(const TextSelection & sel, prev_formats.keys())
    {
        const QTextCharFormat & org_format = prev_formats.value(sel);
        qDebug() << "original block format:" << org_format;

        QTextCursor htext_cursor(ui->mainOutput->document());
        htext_cursor.setPosition(sel.pos, QTextCursor::KeepAnchor);
        htext_cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, sel.len);

        htext_cursor.setCharFormat(org_format);
        qDebug() << "current highlighted format:" << htext_cursor.charFormat();
    }
    prev_formats.clear();

    qDebug() << "-------------------";
    qDebug() << "search text changed: " << text;

    QTextCursor cursor(ui->mainOutput->document());
    cursor.movePosition(QTextCursor::Start);
    cursor = ui->mainOutput->document()->find(text, cursor);

    while (!cursor.isNull())
    {
        if (cursor.hasSelection())
        {
            qDebug() << "\tselection: " << cursor.selectionStart() << ':' << cursor.selectionEnd();
            ui->mainOutput->setTextCursor(cursor);

            // save current text format before highlighting it
            QTextCharFormat ch_fmt = cursor.charFormat();
            QTextCharFormat bck_fmt = cursor.blockCharFormat();

            prev_formats.insert(TextSelection(cursor.position(), text.length()), ch_fmt);

            ch_fmt.setBackground(QBrush(Qt::yellow));
            cursor.setCharFormat(ch_fmt);

            QTextCharFormat dbg_fmt = cursor.charFormat();

            cursor.setPosition(cursor.position() + 1);
        }
        cursor = ui->mainOutput->document()->find(text, cursor);
    }
}

