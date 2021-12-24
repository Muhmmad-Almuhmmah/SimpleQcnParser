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
        showMsg("Fail Read File",_read.errorString(),true);
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
        showMsg("Fail Write File",_write.errorString(),true);
    }
    return false;
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
        qcHandle=QByteArray::fromRawData(reinterpret_cast<const char*>(&idNV),sizeof(idNV));
        qcHandle.prepend(QByteArray::fromHex(handle.toLatin1()));
        //qDebug() <<idNV<<qcHandle<<qcHandle.toHex();
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
        showMsg("Error Parser","Fail Parser QCN File!",true);
    }else{
        showMsg("Parser","Parser QCN Success");
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
        QByteArray patch;
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
            patch.append(QByteArray().fill(0x00,(128-patch.length())));
            qDebug() <<"patch"<<patch.toHex();
            buffer.replace(item.buffer,patch);
            state=true;
        }
    }
    if(!error.isEmpty())
    {
        showMsg("Inupt Error",error);
        return false;
    }
    if(!state){
        showMsg("Error","Fail Patch data");
        return false;
    }
    return true;
}

void MainWindow::showMsg(QString title, QString Msg, bool error)
{
    QApplication::beep();
    label.setText(HDR_MSG.arg(Msg));
    //    QMessageBox box(this);
    //    box.setWindowTitle(title);
    //    box.setIcon(error?QMessageBox::Warning:QMessageBox::Information);
    //    box.setText(Msg);
    //    box.exec();
    //    ui->statusbar->showMessage(QString("%1").arg(Msg),5000);
}

void MainWindow::on_btn_LoadFile_clicked()
{
    QString file=QFileDialog::getOpenFileName(this,"Select QCN File","","Qcn File(*.qcn)");
    if(file.isEmpty())
        return;
    if(!ReadFile(file,buffer))
        return;
    if(ParserBuffer(buffer))
        ui->lineQcFile->setText(file);

}

void MainWindow::on_btnWriteFile_clicked()
{
    QString file=QFileDialog::getSaveFileName(this,"Select Save File Name","","Qcn File(*.qcn)");
    if(file.isEmpty())
        return;
    if(buffer.isEmpty()){
        showMsg("","Load QCN File First !",true);
        return;
    }
    if(!PatchBuffer(buffer))
    {
        return;
    }
    if(currentLen!=buffer.length()){
        showMsg("Error","Fail Detect Current Length File");
        return;
    }
    if(WriteFile(file,buffer))
        showMsg("","Succecc Write Patch Data");
}
