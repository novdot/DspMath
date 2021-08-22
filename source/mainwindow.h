#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QObject>
#include <QTimer>
#include <QList>
#include <QProcess>
#include <QFileDialog>
#include <QDebug>

#include <iostream>
#include <Windows.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void openSrcFile();
    void openDstFile();
    void runDsp();
private:
    Ui::MainWindow *ui;
    QList<int32_t> m_src_array;
    QString m_strSrcFileName;
    QList<int32_t> m_dst_array;
    QString m_strDstFileName;

    void saveDstFile();
};

#endif // MAINWINDOW_H
