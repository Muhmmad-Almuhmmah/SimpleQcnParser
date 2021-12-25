#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ids<<0;   //ESN
    ids<<85;  //SPC
    ids<<550; //IMEI
    ids<<1943;//MEID
    ui->statusbar->addWidget(&label);
    label.setText(HDR_MSG.arg("Simple Qcn Parser"));
    label.show();
    label.setOpenExternalLinks(true);
    connect(ui->btn_LoadFile,SIGNAL(clicked()),this,SLOT(UserBtns()));
    connect(ui->btnWriteFile,SIGNAL(clicked()),this,SLOT(UserBtns()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::ReadFile(const QString &file, QByteArray &buffer)
{
    QFile _read(file);
    if(_read.open(QIODevice::ReadOnly))
        buffer=_read.readAll();
    else{
        showMsg(_read.errorString());
    }
    return !buffer.isEmpty();
}

bool MainWindow::WriteFile(const QString &file, const QByteArray &buffer)
{
    QFile _write(file);
    if(_write.open(QIODevice::WriteOnly)){
        _write.write(buffer);
        _write.close();
        return true;
    }else{
        showMsg(_write.errorString());
    }
    return false;
}

QByteArray MainWindow::GetHandle(const uint32_t &item)
{
    QByteArray qcHandle;
    qcHandle=QByteArray::fromRawData(reinterpret_cast<const char*>(&item),sizeof(item));
    qcHandle.prepend(QByteArray::fromHex(handle.toLatin1()));
    return qcHandle;
}

bool MainWindow::ParserBuffer(QByteArray &buffer)
{
    nvItems.clear();
    currentLen=0;
    ui->lineESN->clear();
    ui->lineSPC->clear();
    ui->lineIMEI->clear();
    ui->lineMEID->clear();
    ui->btnWriteFile->setEnabled(0);
    QByteArray qcHandle,nvData;
    int32_t offset;
    foreach(uint32_t idNV,ids){
        //        qcHandle=QByteArray::fromRawData(reinterpret_cast<const char*>(&idNV),sizeof(idNV));
        //        qcHandle.prepend(QByteArray::fromHex(handle.toLatin1()));
        qcHandle=GetHandle(idNV);
        qDebug() <<idNV<<qcHandle<<qcHandle.toHex();
        offset=buffer.indexOf(qcHandle);
        if(offset){
            nvItems.append(nvItem(idNV,offset,buffer.mid(offset+8,128)));
        }
    }
    if(nvItems.isEmpty())
        return false;
    foreach(nvItem item,nvItems){
        QString value;
        qDebug() <<item.id<<item.buffer.toHex();
        if(!item.id and QC_Calculator::ReadableESN(item.buffer.toHex(),value))
            ui->lineESN->setText(value);
        else if(item.id==85){
            ui->lineSPC->setText(item.buffer.mid(0,6));
        }else if(item.id==550 and QC_Calculator::ReadableIMEI(value,item.buffer))
            ui->lineIMEI->setText(value);
        else if(item.id==1943 and QC_Calculator::ReadableMEID(value,reinterpret_cast<BYTE*>(item.buffer.mid(0,7).data())))
            ui->lineMEID->setText(value);
    }

    if(ui->lineESN->text().isEmpty() && ui->lineIMEI->text().isEmpty() &&
            ui->lineMEID->text().isEmpty() && ui->lineSPC->text().isEmpty())
    {
        showMsg("Fail Parser QCN File!");
    }else{
        showMsg("Parser QCN Success");
        ui->btnWriteFile->setEnabled(1);
        currentLen=buffer.length();
    }
    return currentLen;
}

bool MainWindow::PatchBuffer(QByteArray &buffer)
{
    bool state=false;
    QString error;
    foreach(nvItem item,nvItems){
        QByteArray patch,currentItem;
        QString value;
        qDebug() <<item.id<<item.buffer.toHex();
        if(!item.id){
            value=ui->lineESN->text().trimmed();
            if(!value.isEmpty() && value.length()!=8)
            {
                error="Length ESN Must be 8";
                break;
            }
            QC_Calculator::WritebleESN(value,patch);
        }else if(item.id==85){
            value=ui->lineSPC->text().trimmed();
            if(!value.isEmpty() && value.length()!=6)
            {
                error="Length SPC Must be 6";
                break;
            }
            if(value.length()==6)
                patch=value.toLatin1();
        }else if(item.id==550 and QC_Calculator::ReadableIMEI(value,item.buffer)){
            value=ui->lineIMEI->text().trimmed();
            if(!value.isEmpty() && value.length()!=15)
            {
                error="Length IMEI Must Be 15";
                break;
            }
            QC_Calculator::WritableIMEI(value,patch,item.buffer.startsWith(0x08));
        }else if(item.id==1943 and QC_Calculator::ReadableMEID(value,reinterpret_cast<BYTE*>(item.buffer.mid(0,7).data()))){
            value=ui->lineMEID->text().trimmed();
            if(!value.isEmpty() && value.length()!=14)
            {
                error="Length MEID Must be 14";
                break;
            }
            QC_Calculator::WritableMEID(value,patch);
        }
        if(patch.length()){
            patch.prepend(GetHandle(item.id));
            patch.append(QByteArray().fill(0x00,(136-patch.length())));
            currentItem=GetHandle(item.id);
            currentItem.append(item.buffer);
            qDebug() <<patch.length()<<"patch"<<patch.toHex();
            qDebug() <<currentItem.length()<<"currentItem"<<currentItem.toHex();
            buffer.replace(currentItem,patch);
            state=true;
        }
    }
    if(!error.isEmpty())
    {
        showMsg(error);
        return false;
    }
    if(!state){
        showMsg("Fail Patch data");
        return false;
    }
    return true;
}

void MainWindow::showMsg(QString Msg)
{
    QApplication::beep();
    label.setText(HDR_MSG.arg(Msg));
}

void MainWindow::UserBtns()
{
    setEnabled(false);
    QPushButton *pButton = qobject_cast<QPushButton *>(sender());
    qDebug() <<"sender"<<sender();
    if(pButton==ui->btn_LoadFile){
        QString file=QFileDialog::getOpenFileName(this,"Select QCN File","","Qcn File(*.qcn)");
        if(!file.isEmpty()){
            if(ReadFile(file,buffer)){
                if(ParserBuffer(buffer))
                    ui->lineQcFile->setText(file);
            }
        }

    }else if(pButton==ui->btnWriteFile){
        QString file=QFileDialog::getSaveFileName(this,"Select Save File Name","","Qcn File(*.qcn)");
        if(!file.isEmpty()){
            if(!buffer.isEmpty()){
                if(PatchBuffer(buffer))
                {
                    if(currentLen==buffer.length()){
                        if(WriteFile(file,buffer))
                            showMsg("Success Write Patch Data");
                    }else{
                        qDebug() <<currentLen<<buffer.length();
                        showMsg("Fail Detect Current Length File");
                    }
                }
            }else{
                showMsg("Load QCN File First !");
            }
        }
    }
    setEnabled(true);
}
