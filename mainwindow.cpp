#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QTime>

#define TOTAL_STEPS 16
#define VIDEO_DURATION 30000.0

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->stopBtn->hide();
}


void MainWindow::on_startBtn_clicked()
{
    ui->startBtn->hide();
    ui->stopBtn->show();
    start();
}

void MainWindow::on_stopBtn_clicked()
{
    ui->startBtn->show();
    ui->stopBtn->hide();

    QHashIterator<QProcess*, qreal> i(mProcesses);
    while (i.hasNext())
    {
        i.next();
        QProcess *prc = i.key();
        prc->terminate();
        prc->kill();
        prc->waitForFinished();
        prc->deleteLater();
    }

    mFinisheds.clear();
    mProcesses.clear();
}

void MainWindow::start(int count)
{
    if (ui->startBtn->isVisible())
        return;
    if (count == TOTAL_STEPS)
    {
        ui->startBtn->show();
        ui->stopBtn->hide();
        return;
    }

    mStartTime = QDateTime::currentDateTime();

    count++;

    ui->tableWidget->setRowCount(count);
    ui->tableWidget->setItem(count-1, 0, new QTableWidgetItem(QString::number(count)));
    ui->tableWidget->setItem(count-1, 1, new QTableWidgetItem(QString::number(count)));
    ui->tableWidget->setItem(count-1, 2, new QTableWidgetItem("30s"));
    ui->tableWidget->setItem(count-1, 3, new QTableWidgetItem(QString("%1s").arg(count * 30)));
    ui->tableWidget->setItem(count-1, 4, new QTableWidgetItem("Calculating"));
    ui->tableWidget->setItem(count-1, 5, new QTableWidgetItem("Calculating"));

    ui->statusLabel->setText( tr("Start with %1 threads").arg(count) );
    ui->progressBar->setValue( 100.0 * (count - 1) / TOTAL_STEPS );

    for (int i=0; i<count; i++)
    {
        QProcess *prc = new QProcess(this);
        prc->setReadChannelMode(QProcess::MergedChannels);

        connect(prc, &QProcess::readyReadStandardOutput, this, [this, prc, count, i](){
            QString output = prc->readAllStandardOutput();
            qDebug() << i << ": " << output.trimmed().toStdString().c_str();

            QRegExp rx("time\\=(\\d\\d\\:\\d\\d\\:\\d\\d\\.\\d\\d)");
            if (rx.indexIn(output) < 0)
                return;

            qint64 msecs = QTime(0,0,0).msecsTo( QTime::fromString(rx.cap(1), "hh:mm:ss.z") );
            if (msecs > VIDEO_DURATION)
                msecs = VIDEO_DURATION;

            mProcesses[prc] = (msecs / VIDEO_DURATION);
            calculateProgress(count);
        });

        connect(prc, static_cast<void(QProcess::*)(int)>(&QProcess::finished), this, [this, prc, count](){
            mFinisheds.insert(prc);
            mProcesses[prc] = 1;
            calculateProgress(count);

            if (mFinisheds.count() != count)
                return;

            for (QProcess *prc: mFinisheds)
                prc->deleteLater();

            mFinisheds.clear();
            mProcesses.clear();

            ui->tableWidget->item(count-1, 4)->setText( QString("%1s").arg( mStartTime.secsTo(QDateTime::currentDateTime()) ));
            ui->tableWidget->item(count-1, 5)->setText( QString("%1s").arg( mStartTime.secsTo(QDateTime::currentDateTime()) / count ));

            start(count);
        });

        mProcesses[prc] = 0;

        prc->start(
            #ifdef Q_OS_LINUX
                    "ffmpeg"
            #else
                    QCoreApplication::applicationDirPath() + "/ffmpeg"
            #endif
                   , {
                       "-i", QCoreApplication::applicationDirPath() + "/libx264.dll",
                       "-preset", "ultrafast", "-threads", "1", "-c:v", "libx264",
                       "-f", "null", "-"
                   });
    }
}

void MainWindow::calculateProgress(int step)
{
    qreal progress = 100.0 * (step - 1) / TOTAL_STEPS;
    QHashIterator<QProcess*, qreal> i(mProcesses);
    while (i.hasNext())
    {
        i.next();
        progress += (i.value() / step) * (100.0 / TOTAL_STEPS);
    }

    ui->progressBar->setValue(progress);
}

MainWindow::~MainWindow()
{
    QHashIterator<QProcess*, qreal> i(mProcesses);
    while (i.hasNext())
    {
        i.next();
        QProcess *prc = i.key();
        prc->terminate();
        prc->waitForFinished();
    }

    mFinisheds.clear();
    mProcesses.clear();

    delete ui;
}
