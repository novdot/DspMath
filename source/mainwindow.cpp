#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QDateTime>
#include <QMessageBox>

#include <math.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowTitle("DspMath");

    connect(ui->pushButton_choose_dst,SIGNAL(clicked()),this,SLOT(testSin()));
    connect(ui->pushButton_open_src,SIGNAL(clicked()),this,SLOT(openSrcFile()));
    connect(ui->pushButton_run,SIGNAL(clicked()),this,SLOT(runDsp()));

    ui->label_status->setText("Empty");
    ui->label_status_dst_file->setText("Empty");
    ui->label_status_src_file->setText("Empty");
}

MainWindow::~MainWindow()
{
    delete ui;
}
/**************************************************************************/
void MainWindow::openSrcFile()
{
    bool ok;
    int val;
    m_strSrcFileName = QFileDialog::getOpenFileName(0, "Open Dialog", "", "*.*");
    m_src_array.clear();
    ui->label_status_src_file->setText(m_strSrcFileName);
    ui->label_status->setText("openSrcFile");

    // Создаем объект
    QFile file(m_strSrcFileName);

    // С помощью метода open() открываем файл в режиме чтения
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("Cannot open file for reading");
        ui->label_status->setText("openSrcFile..fail");
        return;
    }

    // Создаем входящий поток, из которого будут считываться данные, и связываем его с нашим файлом
    QTextStream in(&file);

    // Считываем файл строка за строкой
    while (!in.atEnd()) {
        QString line = in.readLine();
        val = line.toInt(&ok,10);
        //qDebug()<<"Readed value: "<<val;
        if(ok){
            m_src_array.append(val);
        }
    }

    // Закрываем файл
    file.close();
    ui->label_status->setText("openSrcFile..done");
}
/**************************************************************************/
void MainWindow::testSin()
{
    int it = 0;
    int it_max = 40;
    int it_periods = 0;
    int it_periods_max = 25;
    m_src_array.clear();
    ui->label_status->setText("test Sinus");

    for(it_periods=0;it_periods<it_periods_max;it_periods++){
        for(it=0;it<it_max;it++){
             m_src_array.append((int)(sin((float)it*2.0*3.1415/(float)it_max)*1000)+32000);
        }
    }
}
/**************************************************************************
void MainWindow::openDstFile()
{
}

/**************************************************************************/
/*
#define	X	    0.98 //0.96 //0.86
#define	A0_HP   ((1 + X)/2) * 0x40000000
#define	A1_HP   (-(1 + X)/2) * 0x40000000
#define	B1_HP   (X * 0x80000000)/2
*/
#define PI   (3.1415)

#define L_PLC   (3)
#define L_DUP   (3)
/*
#define DIV_CONST	(768)
#define DIV_CONST2	(384)
*/
#define	HALFINT	(0x40000000) //???

int aPLC[L_PLC], bPLC[L_PLC], aDUP[L_DUP], bDUP[L_DUP];
//int aDUP_2[L_DUP] = {(int)A0_HP, (int)A1_HP, 0}, bDUP_2[L_DUP] = {0, (int)B1_HP, 0};
void dsp_init()
{
    double K, R, Cos_x_2, R_x_R;
    int FiltType = 0;
    double BandWidth = 10.0/(float)(10000 );
    double CenterFreq = 1.0/(float)(40);

    R = 1.0 - 3.0 * BandWidth;
    R_x_R = R * R;
    Cos_x_2 = cos(2.0 * PI * CenterFreq) * 2.0;
    K = (1.0 - R * Cos_x_2 + R_x_R)/(2.0 - Cos_x_2);
    switch (FiltType){
    case 0:
        aPLC[0] = (int)((1.0 - K)*HALFINT);
        aPLC[1] = (int)(((K - R) * Cos_x_2)*HALFINT);
        aPLC[2] = (int)((R_x_R - K)*HALFINT);
        bPLC[0] = 0;
        bPLC[1] = (int)((R * Cos_x_2)*HALFINT);
        bPLC[2] = (int)(( (-1) * R_x_R)*HALFINT);
        break
            ;
    case 1:
        aDUP[0] = (int)((1.0 - K)*HALFINT);
        aDUP[1] = (int)(((K - R) * Cos_x_2)*HALFINT);
        aDUP[2] = (int)((R_x_R - K)*HALFINT);
        bDUP[0] = 0;
        bDUP[1] = (int)((R * Cos_x_2)*HALFINT);
        bDUP[2] = (int)((- R_x_R)*HALFINT);
        break;
    }

    printf("Init reqursion coef\n");
    printf("CenterFreq:%f BandWidth:%f\n",CenterFreq,BandWidth);
    printf("K:%f, R:%f, Cos_x_2:%f, R_x_R:%f\n",K, R, Cos_x_2, R_x_R);
    printf("A(%d,%d,%d)\n",aPLC[0],aPLC[1],aPLC[2]);
    printf("B(%d,%d,%d)\n",bPLC[0],bPLC[1],bPLC[2]);
    /*
    qDebug()<<"-----------------------------------"<<endl<<"Init reqursion coef"<<endl;
    qDebug()<< " f:" << CenterFreq << " BW:"<<BandWidth;
    qDebug()<< " K:" << K << " R:"<<R;
    qDebug()<< " A0:" << aPLC[0];
    qDebug()<< " A1:" << aPLC[1];
    qDebug()<< " A2:" << aPLC[2];
    qDebug()<< " B1:" << bPLC[1];
    qDebug()<< " B2:" << bPLC[2];
    qDebug()<<"-----------------------------------"<<endl;*/
}
int dsp_run(int input)
{
    static unsigned kIn = 0;
    int ind = 0;
    int64_t	temp = 0;
    unsigned i;
    static int BufInPLC_1 [L_PLC] = {0,0,0};
    static int BufInPLC_2 [L_PLC] = {0,0,0};
    static int BufOutPLC [L_PLC] = {0,0,0};

    if (kIn>(L_PLC-1)) kIn = 0;

    printf("------------------------------------\n");
    printf("input:%d(0x%08X) kIn:%d\n", input, input, kIn);
    BufInPLC_1[kIn] = input;
    ind = kIn;
    // BufInPLC_2[kIn] = 0;

    for (i=0; i<L_PLC; i++){
        temp += (int64_t)aPLC[i]*(int64_t)BufInPLC_1[ind];
        temp += (int64_t)bPLC[i]*(int64_t)BufInPLC_2[ind];
        if ((--ind) < 0) ind = L_PLC-1;
    }
    BufInPLC_2[kIn] = (int)(temp>>30);
    printf("temp before convert to BufInPLC_2:%I64d(0x%I64X)\n", temp, temp);
    //2 section
    //  BufOutPLC[kIn] = 0;
    temp = 0;
    for (i=0; i<L_PLC; i++){
        temp += (int64_t)aPLC[i]*(int64_t)BufInPLC_2[ind];
        temp += (int64_t)bPLC[i]*(int64_t)BufOutPLC[ind];
        if ((--ind) < 0) ind = L_PLC-1;
    }
    BufOutPLC[kIn] = (int)(temp>>30);
    printf("temp before convert to BufOutPLC:%I64d(0x%I64X)\n", temp, temp);

    printf("Stage buffer for input:%d(0x%08X)\n", input, input);
    printf(" BufInPLC_1[0]:%d(0x%08X)\n", BufInPLC_1[0], BufInPLC_1[0]);
    printf(" BufInPLC_1[1]:%d(0x%08X)\n", BufInPLC_1[1], BufInPLC_1[1]);
    printf(" BufInPLC_1[2]:%d(0x%08X)\n", BufInPLC_1[2], BufInPLC_1[2]);
    printf(" BufInPLC_2[0]:%d(0x%08X)\n", BufInPLC_2[0], BufInPLC_2[0]);
    printf(" BufInPLC_2[1]:%d(0x%08X)\n", BufInPLC_2[1], BufInPLC_2[1]);
    printf(" BufInPLC_2[2]:%d(0x%08X)\n", BufInPLC_2[2], BufInPLC_2[2]);
    printf(" BufOutPLC[0] :%d(0x%08X)\n" , BufOutPLC[0] , BufOutPLC[0] );
    printf(" BufOutPLC[1] :%d(0x%08X)\n" , BufOutPLC[1] , BufOutPLC[1] );
    printf(" BufOutPLC[2] :%d(0x%08X)\n" , BufOutPLC[2] , BufOutPLC[2] );
    printf("\n" );
    return (BufOutPLC[kIn++]);
}
void MainWindow::runDsp()
{
    int val = 0;
    dsp_init();

    QList<int32_t>::iterator i;
    for (i = m_src_array.begin(); i != m_src_array.end(); ++i){
       val = dsp_run(*i);
       //qDebug()<<"Converted: "<<val;
       m_dst_array.append(val);
    }
    saveDstFile();
}
/**************************************************************************/
void MainWindow::saveDstFile()
{
    m_strDstFileName = QFileDialog::getSaveFileName(0, "Save Dialog", "", "*.txt");
    ui->label_status_dst_file->setText(m_strDstFileName);
    QFile file(m_strDstFileName);

    if (!file.open(QIODevice::WriteOnly)) {
        qWarning("Cannot open file for writing");
        ui->label_status->setText("saveDstFile..fail");
        return;
    }
    QTextStream out(&file);

    QList<int32_t>::iterator i;
    for (i = m_dst_array.begin(); i != m_dst_array.end(); ++i){
        out<< *i <<endl;
    }

    file.close();
}

