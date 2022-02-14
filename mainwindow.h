#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QByteArray>
#include <QFile>
#include <QFileDialog>
#include <QDebug>
#include <calculator.h>
#include <QMessageBox>
#include <QDesktopServices>
#include <QLabel>
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

typedef struct _nv_item_{
    _nv_item_(int _id,int _offset,QByteArray buff)
    {
        id=_id;
        offset=_offset;
        buffer=buff;
        //        qDebug() <<"Add "<<id<<offset<<buffer.mid(0,30)<<buffer.length()<<endl<<endl;
    }
    uint id;
    int offset;
    QByteArray buffer;
}nvItem;
#define HDR_MSG QString("<a href=\"https://github.com/Muhmmad-Almuhmmah/\" target=\"_blank\">Githup</a> <a href=\"https://www.facebook.com/X.Dev.Ye\" target=\"_blank\">Facebook</a>\t|\t%1")
class MainWindow : public QMainWindow
{
    Q_OBJECT
    QList<nvItem> nvItems;
    QList<uint32_t>ids;
    QString handle="88 00 01 00 ";
    QByteArray buffer;
    QString stringMin,Banner;
    int currentLen=0,rmItems;
    QLabel label;
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void RmOptions(bool show);
    bool ReadFile(const QString &file,QByteArray &buffer);
    bool WriteFile(const QString &file,const QByteArray &buffer);
    QByteArray GetHandle(const uint32_t &item);
    bool ParserBuffer(QByteArray &buffer);
    bool PatchBuffer(QByteArray &buffer);
    void showMsg(QString Msg);
    void UpdateTitle();
private slots:
    void UserBtns();

    void on_ch_SensitiveItems_clicked(bool checked);

private:
    Ui::MainWindow *ui;

};
#endif // MAINWINDOW_H
