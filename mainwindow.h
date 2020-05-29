#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QDateTime>
#include <QList>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_startBtn_clicked();
    void start(int count = 0);
    void calculateProgress(int step);

    void on_stopBtn_clicked();

private:
    Ui::MainWindow *ui;
    QHash<QProcess*, qreal> mProcesses;
    QSet<QProcess*> mFinisheds;
    QDateTime mStartTime;
};
#endif // MAINWINDOW_H
