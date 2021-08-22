#include "mainwindow.h"

#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    //float_dbg();
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
