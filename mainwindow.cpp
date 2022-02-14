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
    ids<<32;  //MIN1
    ids<<33;  //MIN2
    ids<<85;  //SPC
    ids<<550; //IMEI
    ids<<1943;//MEID
    ids<<71;  //banner
    ui->statusbar->addWidget(&label);
    label.setText(HDR_MSG.arg("Simple Qcn Parser"));
    label.show();
    label.setOpenExternalLinks(true);
    connect(ui->btn_LoadFile,SIGNAL(clicked()),this,SLOT(UserBtns()));
    connect(ui->btnWriteFile,SIGNAL(clicked()),this,SLOT(UserBtns()));
    RmOptions(0);
    UpdateTitle();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::RmOptions(bool show)
{
    ui->ch_ESN->setVisible(show);
    ui->ch_MEID->setVisible(show);
    ui->ch_SPC->setVisible(show);
    ui->ch_IMEI->setVisible(show);
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
    ui->lineQcFile->clear();
    stringMin.clear();
    Banner.clear();
    label.setText(HDR_MSG.arg("Simple Qcn Parser"));
    ui->btnWriteFile->setEnabled(0);
    QByteArray qcHandle,nvData;
    uint32_t min1,min2;
    int offset=0;
    foreach(uint32_t idNV,ids){
        //        qcHandle=QByteArray::fromRawData(reinterpret_cast<const char*>(&idNV),sizeof(idNV));
        //        qcHandle.prepend(QByteArray::fromHex(handle.toLatin1()));
        qcHandle=GetHandle(idNV);
        qDebug() <<idNV<<qcHandle<<qcHandle.toHex();
        offset=buffer.indexOf(qcHandle);
        qDebug() <<offset;
        if(offset>0){
            nvItems.append(nvItem(idNV,offset,buffer.mid(offset+8,128)));
        }
        offset=0;
    }
    if(nvItems.isEmpty()){
        showMsg("Can't Find Items ESN/MEID/IMEI/SPC");
        return false;
    }
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
        else if(item.id==32)
            min1=QC_Calculator::ConvertMin1((BYTE*)item.buffer.data());
        else if(item.id==32)
            min2=QC_Calculator::ConvertMin2((BYTE*)item.buffer.data());
        else if(item.id==71)
            Banner=item.buffer;
    }
    stringMin=QC_Calculator::DecodeMIN(min1,min2);
    qDebug() <<"stringMin"<<min1<<Banner;
    if(ui->lineESN->text().isEmpty() && ui->lineIMEI->text().isEmpty() &&
            ui->lineMEID->text().isEmpty() && ui->lineSPC->text().isEmpty())
    {
        showMsg("Fail Parser QCN File!");
    }else{
        QString msg="Parser QCN Success";
        if(stringMin.length() and Banner.length())
            msg+=QString(" | MIN:%1 | BANNER:%2").arg(stringMin).arg(Banner);
        else if(stringMin.length())
            msg+=QString(" | MIN:%1").arg(stringMin);
        else if(Banner.length())
            msg+=QString(" | BANNER:%1").arg(Banner);
        showMsg(msg);
        ui->btnWriteFile->setEnabled(1);
        currentLen=buffer.length();
    }
    UpdateTitle();
    return currentLen;
}

bool MainWindow::PatchBuffer(QByteArray &buffer)
{
    bool state=false;
    rmItems=0;
    qDebug() <<"bufferLen"<<buffer.length();
    QString error;
    QList<int> sensitiveItems=QList<int>() <<0<<85<<550<<1943;
    foreach(nvItem item,nvItems){
        QByteArray patch,currentItem;
        QString value;
        qDebug() <<item.id<<item.buffer.toHex();
        if(ui->ch_SensitiveItems->isChecked()){
            if(!ui->ch_ESN->isChecked() && item.id==0)          // ESN
                continue;
            else if(!ui->ch_IMEI->isChecked() && item.id==550)  // IMEI
                continue;
            else if(!ui->ch_MEID->isChecked() && item.id==1943)  // MEID
                continue;
            else if(!ui->ch_SPC->isChecked() && item.id==85){    // SPC
                continue;
            }
            if(!sensitiveItems.contains(item.id))
                continue;
            qDebug() <<"Remove Item"<<item.id;
            patch.prepend(GetHandle((60000)+item.id));
            patch.append(QByteArray().fill(0x00,(136-patch.length())));
            currentItem=GetHandle(item.id);
            currentItem.append(item.buffer);
            if(patch==currentItem){
                qDebug() <<"Skip "<<item.id;
                continue;
            }
            qDebug() <<patch.length()<<"patch"<<patch.toHex();
            qDebug() <<currentItem.length()<<"currentItem"<<currentItem.toHex();
            buffer.replace(currentItem,patch);
            rmItems+=1;
            continue;
        }
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
            if(patch==currentItem){
                qDebug() <<"Skip "<<item.id;
                continue;
            }
            qDebug() <<patch.length()<<"patch"<<patch.toHex();
            qDebug() <<currentItem.length()<<"currentItem"<<currentItem.toHex();
            buffer.replace(currentItem,patch);
            state=true;
        }
    }
    qDebug() <<"bufferLen"<<buffer.length();
    if(!error.isEmpty())
    {
        showMsg(error);
        return false;
    }
    if(!state and !rmItems){
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

void MainWindow::UpdateTitle()
{
    QString title="Simple QCN Patcher v1.2";
    setWindowTitle(title);
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
        QString file;
        if(ui->ch_SensitiveItems->isChecked()){
            if(!(ui->ch_ESN->isChecked() or ui->ch_SPC->isChecked() or ui->ch_IMEI->isChecked() or ui->ch_MEID->isChecked())){
                showMsg("! Please Select Item(s) to remove");
                setEnabled(true);
                return;
            }
        }
        file=QFileDialog::getSaveFileName(this,"Select Save File Name","","Qcn File(*.qcn)");
        if(!file.isEmpty()){
            if(!buffer.isEmpty()){
                if(PatchBuffer(buffer))
                {
                    if(currentLen!=buffer.length()/* and !ui->ch_SensitiveItems->isChecked()*/){
                        qDebug() <<currentLen<<buffer.length();
                        showMsg("Fail Detect Current Length File");
                    }else if(WriteFile(file,buffer)){
                        QString msg1="Success Write Patch Data";
                        if(rmItems){
                            msg1.append(QString(" && Remove Items:"+QString::number(rmItems)));
                        }
                        showMsg(msg1);
                    }
                }
            }else{
                showMsg("Load QCN File First !");
            }
        }
    }
    setEnabled(true);
}

void MainWindow::on_ch_SensitiveItems_clicked(bool checked)
{
    RmOptions(checked);
}
