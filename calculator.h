#ifndef QC_CALCULATOR_H
#define QC_CALCULATOR_H

#include <QString>
#include <windows.h>
#include <QCryptographicHash>
#include <QDebug>
class QC_Calculator
{
public:

    static bool WritableMEID(const QString &strMeid, QByteArray &Buff)
    {
        if(strMeid.length()!=14)
            return false;
        char		szMEID[32]={0};
        __int64		i64Cvt;
        char *nMeidStr=(char*)malloc(strMeid.length());
        strcpy(nMeidStr,strMeid.toStdString().c_str());
        strcpy(szMEID,nMeidStr);
        sscanf(szMEID,"%I64X",&i64Cvt);
        Buff=QByteArray::fromRawData((const char *)&i64Cvt,sizeof(i64Cvt));
        free(nMeidStr);
        return true;
    }

    static bool ReadableMEID(QString &StrMeid, BYTE *ItemData)
    {
        char szTmp[1024]={0},szData[512]={0};
        int bStart=6,SizeBuff=7;
        if(ItemData[4]==0x5D)
        {
            bStart=7;
            SizeBuff=8;
        }
        sprintf(szData,"%02X",ItemData[SizeBuff]);
        for(int	i=bStart;i>=0;i--)
        {
            //check and skip 0x5D if exist
            if(ItemData[i]==0x5d)
                continue;
            sprintf(szTmp,"%02X",ItemData[i]);
            strcat(szData,szTmp);
        }
        StrMeid=QByteArray((char*)&szData[2],14);
        return StrMeid.length()==14;
    }

    static bool WritableIMEI(const QString &strImei,QByteArray &ItemData, bool Preappend_08)
    {
        if(strImei.length()!=15)
            return false;
        byte bTh,bTl;
        int i=0;
        if(Preappend_08==true)
            ItemData[0]=0x08;
        char *nImeiStr=NULL;
        nImeiStr=(char*)malloc(strImei.length());
        if(nImeiStr==NULL)
            return false;
        strcpy(&nImeiStr[0],strImei.toStdString().c_str());
        ItemData[1]=(((nImeiStr[0]-0x30)<<4) & 0xF0 ) | 0x0a;
        for(i=2; i<9 ; i++)
        {
            bTh=nImeiStr[2*i-2]-0x30;
            bTl=nImeiStr[2*i-3]-0x30;
            ItemData[i]= (((bTh&0x0F)<<4) & 0xF0) | (bTl & 0x0F);
        }
        free(nImeiStr);
        return true;
    }

    static bool ReadableIMEI(QString &StrImei, const QByteArray& ItemData)
    {
        char szTmp[1024]={0},szData[512]={0},szIMEI[32]={0};
        __int64	i64Tmp;
        uint32_t uiImeiLen=15;
        int iItemDataLen=(uiImeiLen+1)/2+1;
        memcpy(&i64Tmp,ItemData.mid(1,iItemDataLen-1).data(),iItemDataLen-1);
        sprintf(szData,"%016I64X",i64Tmp);
        memset(szTmp,0,sizeof(szTmp));
        for(int i=0;i<(int)uiImeiLen+1;i++)
        {
            szTmp[(uiImeiLen+1)-1-i]=szData[i];
        }
        strcpy(szIMEI,&szTmp[1]);
        StrImei=QString(szIMEI);
        return StrImei.length()==15;
    }


    static bool WritebleESN(const QString &esn,QByteArray &patch)
    {
        if(esn.length()!=8) return false;
        uint32_t data=0;
        sscanf(esn.toStdString().c_str(),"%I32X",&data);
        patch=QByteArray::fromRawData((const char*)&data,sizeof(data));
        return 1;
    }

    static bool ReadableESN(const QByteArray &value, QString &EsnVal)
    {
        if(value.count()<7) return false;
        EsnVal=QString(QString(value[6])+QString(value[7])+QString(value[4])+QString(value[5])+QString(value[2])+QString(value[3])+QString(value[0])+QString(value[1])).toUpper();
        return true;
    }

    static uint32_t ConvertMin1(BYTE *buff){
        QByteArray a,v;
        for(int i=7;i>3;i--)
        {
            v=QByteArray((const char*)&buff[i],1).toHex();
            //        QD(v);
            if(i==7 and v=="00")
                continue;
            if(v.at(0)=='0' and v!="00")
                v.remove(0,1);
            a.append(v);
        }
        unsigned int newUint = std::stoul(a.toStdString(), nullptr, 16);
        return newUint;
    }

    static uint16_t ConvertMin2(BYTE *buff){
        QByteArray a,v;
        for(int i=7;i>5;i--)
        {
            v=QByteArray((const char*)&buff[i],1).toHex();
            //        QD(v);
            if(i==7 and v=="00")
                continue;
            if(v.at(0)=='0' and v!="00")
                v.remove(0,1);
            a.append(v);
        }
        unsigned int newUint = std::stoul(a.toStdString(), nullptr, 16);
        return newUint;
    }


    static QString DecodeMIN(uint min1, uint min2){
        uint min1a=0,min1b=0,min1c=0,min1c_5b=0,min1c_5c=0,min1c_5d=0;
        min2 = (min2 + 1) % 10 + (((((min2 % 100)/10) + 1) % 10) * 10) + ((((min2 / 100) + 1) % 10) * 100);
        min1a = (min1 & 0xFFC000) >> 14;
        min1a = (min1a + 1) % 10 + (((((min1a % 100) / 10) + 1) % 10) * 10) + ((((min1a / 100) + 1) % 10) * 100);
        min1b = ((min1 & 0x3C00) >> 10) % 10;
        min1c = (min1 & 0x3FF);
        min1c_5b = (min1c + 1) % 10;
        min1c_5c = (((((min1c % 100) / 10) + 1) % 10) * 10);
        min1c_5d = ((((min1c / 100) + 1) % 10) * 100);
        min1c = min1c_5b + min1c_5c + min1c_5d;
        return QString::number(min2)+QString::number(min1a)+QString::number(min1b)+QString::number(min1c);
    }

    bool EncodeMIN(const QString &MinString, QString &Min1, QString &Min2){
        if(MinString.isEmpty())
            return false;
        uint16_t min2=(((byte)_strtoui64(MinString.mid(0,1).toLatin1().data(),NULL,16)+9)%10)*100+(((byte)_strtoui64(MinString.mid(1,1).toLatin1().data(),NULL,16)+9)%10)*10+(((byte)_strtoui64(MinString.mid(2,1).toLatin1().data(),NULL,16)+9) %10 );

        DWORD min1a=(((byte)_strtoui64(MinString.mid(3,1).toLatin1().data(),NULL,16)+9)%10)*100+(((byte)_strtoui64(MinString.mid(4,1).toLatin1().data(),NULL,16)+9)%10)*10+(((byte)_strtoui64(MinString.mid(5,1).toLatin1().data(),NULL,16)+9)%10);

        DWORD min1b=(byte)_strtoui64(MinString.mid(6,1).toLatin1().data(),NULL,16);

        if(min1b==0)
            min1b=1;

        DWORD min1c=(((byte)_strtoui64(MinString.mid(7,1).toLatin1().data(),NULL,16)+9)%10)*100+(((byte)_strtoui64(MinString.mid(8,1).toLatin1().data(),NULL,16)+9)%10)*10+(((byte)_strtoui64(MinString.mid(9,1).toLatin1().data(),NULL,16)+9) % 10);

        uint32_t min1=min1c+(min1b<<10)+(min1a<<14);

        Min1=QByteArray((const char*)&min1,sizeof(min1)).toHex();
        Min2=QByteArray((const char*)&min2,sizeof(min2)).toHex();
        return Min2.length()>0 and Min2.length()>0;
    }

};

#endif // QC_CALCULATOR_H
