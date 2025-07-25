#include <string.h>
#include "test.h"

#include "alog_printf.h"


// 北京欢迎您
CModbus modbus;
int chanFreshTime = 300;
void getNameByPid(int pid, char *task_name) {
    char proc_pid_path[256];
    char buf[256];

    sprintf(proc_pid_path, "/proc/%d/status", pid);
    FILE *fp = fopen(proc_pid_path, "r");
    if (NULL != fp) {
        if (fgets(buf, 255, fp) == NULL) {
            fclose(fp);
        }
        fclose(fp);
        sscanf(buf, "%*s %s", task_name);
    }
}

int getProcNo(char *m_ProcName) {
    int ret = 0;
    for (int i = 0; i < 20; i++) {
        if (m_ProcName[i] >= '0' && m_ProcName[i] <= '9') {
            ret = (int) (m_ProcName[i] - 0x30);
            break;
        }
    }
    return ret;
}

#ifdef WIN32
UINT WINAPI SetBaseAddr(HWND Addr) {
    modbus.pid = Addr;
    return 0;
}
UINT WINAPI RxPro(int tdNo, void *fertdata) { return (modbus.rxPro(tdNo, (FERTDATA *) fertdata)); }
UINT WINAPI TxPro(int tdNo, void *fertdata) { return (modbus.txPro(tdNo, (FERTDATA *) fertdata)); }
#else
extern "C" {
int SetBaseAddr(int Addr, void *fertdata) {

    char m_ProcName[20];
    getNameByPid(Addr, m_ProcName);
    modbus.m_ProcNo = getProcNo(m_ProcName);
    modbus.getModbusConfig();
    modbus.pid = Addr;
    modbus.SetBaseAddr((FERTDATA *) fertdata);
    // modbus.InitRTUaddr();
    return 0;
}

int RxPro(int tdNo, void *fertdata) {
    modbus.SetBaseAddr((FERTDATA *) fertdata);
    return (modbus.rxPro(tdNo));
}

int TxPro(int tdNo, void *fertdata) {
    modbus.SetBaseAddr((FERTDATA *) fertdata);
    return (modbus.txPro(tdNo));
}
}
#endif

int CModbus::rxPro(int tdNo) {
    short rtuno = tdNo;
    RTUPARA *rtupara = GetRtuPara(rtuno);
    tdNo = rtupara->chanNo;
    if (dsd[tdNo].commFlag == 0)
        return 0;
    rtuno = GetCurrentRtuNo(tdNo);
    int ret = procdata(tdNo, rtuno, &dsd[tdNo]);
    if (ret == 1)
        CallscirxOvertime(&dsd[tdNo], tdNo, rtuno);
    else {
        dsd[tdNo].rxOvertime = 0;
        usleep(40); // 珠海沃顿PCS反应不及时
        dsd[tdNo].commFlag = 0;
        dsd[tdNo].txCmd = 0;
        ClearRtuRecFailnum(rtuno);
        ClearRec(tdNo);
        ChangeRtuNo(tdNo);
    }

    return 0;
}

int CModbus::getBcufile(int filetype, unsigned short verno) {
    char filename[100];
    char *m_pHomeDir = getenv("RUNDIR");
    if (m_pHomeDir == NULL)
        return 0;
    if (filetype == 1) // bcu file
    {
        sprintf(filename, "%s/bin/BCU_V%d.%d.%d.bin", m_pHomeDir, (verno >> 12) & 0x0f, (verno >> 8) & 0x0f,
                verno & 0xff);
    } else // bmu file
    {
        sprintf(filename, "%s/bin/BMU_V%d.%d.%d.bin", m_pHomeDir, (verno >> 12) & 0x0f, (verno >> 8) & 0x0f,
                verno & 0xff);
    }
    int file = open(filename, 0, O_RDWR);
    if (file < 0) {
        printf("PROTOCOL Err:File %s not exist\n", filename);
        return 0;
    }
    filebuf = (unsigned char *) malloc(MAX_FILE_BUF);
    int num = read(file, filebuf, MAX_FILE_BUF);
    close(file);
    if (num <= 0) {
        alog_printf(ALOG_LVL_INFO, true, "BCU升级文件%s读取失败\n", filename); // 参数文件读取失败
        return 0;
    }
    if (num != ((*(filebuf + 4)) * 256 * 256 * 256 + (*(filebuf + 5)) * 256 * 256 + (*(filebuf + 6)) * 256 +
                (*(filebuf + 7)))) {
        alog_printf(ALOG_LVL_INFO, true, "BCU升级文件%s校验失败1\n", filename); // 参数文件校验失败
        return 0;
    }
    // 需要校验版本号
    return num;
}

void CModbus::getModbusConfig() {
    char filename[100];
    char *m_pHomeDir = getenv("RUNDIR");
    if (m_pHomeDir == NULL)
        return;
    // FILE	*fp;
    sprintf(filename, "%s/dat/ModbusCall.dat", m_pHomeDir);
    int file = open(filename, 0, O_RDWR);
    if (file < 0) {
        printf("PROTOCOL Err:File %s not exist\n", filename);
        return;
    }
    int num = read(file, pmod, sizeof(MODBUS) * RTU_MAX_NUM);
    close(file);
    if (num > 0)
        printf("modbusCall参数配置文件%s读取成功\n", filename); // 参数文件读取成功
}

void CModbus::CallscirxOvertime(MODBUSDATA *sci, int tdNo, short rtuno) {
    CHANPARA *chanp = GetChanPara(tdNo);
    if (chanp == NULL)
        return;
    if (sci->rxOvertime > 6000) {
        // alog_printf(ALOG_LVL_DEBUG,true,"接收超时\n");
        _TRACE(tdNo, "接收超时", 1);
        sci->commFlag = 0;
        sci->rxOvertime = 0;
        ClearRec(tdNo);
        rtudata[rtuno].txPtr++;
        AddRtuRecFailnum(rtuno, 1);
        /// ChangeRtuNo(tdNo);
    } else {
        sci->rxOvertime++;
    }
}
int CModbus::procdata(int tdNo, int rtuno, MODBUSDATA *modbusdata) {
    short len = 0, i;
    LenRecQ(tdNo, &len);

    if (rtuno == -1)
        return 1;
    RTUPARA *rtupara = GetRtuPara(rtuno);
    int modbusrcnt = 0;
    // MODBUS *mp=GetModbusPara();
    if (pmod)
        modbusrcnt = pmod->ordernum;
    else
        return 1;
    while (len >= 3) {

        for (i = 0; i < 3; i++)
            GetRecVal(tdNo, &dsd[tdNo].shmbuf[i]);
        BYTE rturaddr = dsd[tdNo].shmbuf[0];
        BYTE CmdType = dsd[tdNo].shmbuf[1];
        BYTE datalen = dsd[tdNo].shmbuf[2];
        if (rturaddr != rtupara->rtuAddr) {
            for (i = 0; i < 3; i++)
                BackRecVal(tdNo);
            return 1;
        }

        if (CmdType > 4 && CmdType < 17)
            datalen = 3;

        if ((datalen + 5) > len) {
            for (i = 0; i < 3; i++)
                BackRecVal(tdNo);
            return 1;
        }
        for (i = 0; i < datalen + 2; i++)
            GetRecVal(tdNo, dsd[tdNo].shmbuf + 3 + i);

        unsigned short crc = CRC16(dsd[tdNo].shmbuf, datalen + 3);
        if (crc == (dsd[tdNo].shmbuf[datalen + 3] + dsd[tdNo].shmbuf[datalen + 4] * 256)) {
            ShowData(tdNo, 0 /*recv*/, dsd[tdNo].shmbuf, datalen + 5, 0 /*data*/);
            len -= datalen + 5;
        } else {
            len -= datalen + 5;
            continue;
        }

        int i;
        int num = 0, ycnum;
        Self64 utctime = GetCurMSTime();
        switch (CmdType) {
            case 0x01:
            case 0x02: {
                int j;
                unsigned char yxvalue = 0x00;
                num = getoffset(rtudata[rtuno].txPtr, 0, rtupara->type);

                for (j = 0; j < (pmod + rtudata[rtuno].txPtr)->length; j++) {
                    yxvalue = dsd[tdNo].shmbuf[3 + j / 8] >> (j % 8) & 0x01;
                    SetSingleYxValue((unsigned short) rtuno, num + j, yxvalue, utctime, 0);
                    // alog_printf(ALOG_LVL_DEBUG,true,"yxstartnum = %d,yxno = %d,yxvalue = %d,shutbuf =
                    // %d\n",num,num+j,yxvalue,dsd[tdNo].shmbuf[3+j/8]);
                }
                //            int pModeNo = -1;
                //            for(i=0;i<8;i++)
                //            {
                //                if((pmode+i)->dataModeNo != rtupara->type) continue;
                //                else
                //                {
                //                    pModeNo = i;
                //                    break;
                //                }
                //            }
                //            short startAddr = (pmod+rtudata[rtuno].txPtr)->startaddr;
                //            short endAddr = startAddr + MIN(datalen,(pmod+rtudata[rtuno].txPtr)->length);
                //            int i,j;unsigned char yxvalue = 0x00;
                //            for(i=0;i<(pmode+pModeNo)->procNum;i++)
                //            {
                //                if((pmode+pModeNo)->modbusproc[i].regAddr < endAddr)
                //                {
                //                    if((pmode+pModeNo)->modbusproc[i].regAddr >= startAddr)
                //                    {
                //                        for(j=0;j<(pmod+rtudata[rtuno].txPtr)->length;j++)
                //                        {
                //                            yxvalue = dsd[tdNo].shmbuf[3+j/8]>>(j%8) & 0x01;
                //                            SetSingleYxValue((unsigned short)rtuno,num+j,yxvalue,0);
                //                        }
                //                    }
                //                    else
                //                    {
                //                        num += (pmode+pModeNo)->modbusproc[i].regNum;
                //                    }
                //                }
                //            }
                rtudata[rtuno].txPtr++;
                if (rtudata[rtupara->rtuNo].txPtr >= modbusrcnt) {
                    rtudata[rtupara->rtuNo].txPtr = 0;
                    // chanloop[tdNo]++;
                    rtuloop[rtuno]++;
                }
                return 0;
            } break;
            case 0x04:
            case 0x03: {
                switch (dsd[tdNo].txCmd) {
                    case 0: // 遥信0..7-8..15
                        num = getoffset(rtudata[rtuno].txPtr, 0, rtupara->type);
                        SetSingleYxValue(rtuno, num, dsd[tdNo].shmbuf[3], utctime, 0);
                        break;
                    case 2: // 遥信8..15-0..7
                        num = getoffset(rtudata[rtuno].txPtr, 0, rtupara->type);
                        SetSingleYxValue(rtuno, num, dsd[tdNo].shmbuf[4], utctime, 0);
                        break;
                    case 1: // 遥测四字节整数(低字前1032)
                        ycnum = rtupara->ycnum;
                        num = getoffset(rtudata[rtuno].txPtr, 1, rtupara->type);
                        for (i = 0; i < MIN(datalen / 4, ycnum - num); i++) {
                            int ycval = 0;
                            // 低字在前，高字在后
                            ycval = (dsd[tdNo].shmbuf[i * 4 + 3] * 256 + dsd[tdNo].shmbuf[i * 4 + 4]) +
                                    (dsd[tdNo].shmbuf[i * 4 + 5] * 256 + dsd[tdNo].shmbuf[i * 4 + 6]) * 65536;
                            SetYcValue_int(rtuno, num + i, (BYTE *) &ycval, 1, utctime);
                        };
                        break;
                    case 3: // 二字节有符号遥测帧(10)
                        ycnum = rtupara->ycnum;
                        num = getoffset(rtudata[rtuno].txPtr, 1, rtupara->type);
                        for (i = 0; i < MIN(datalen / 2, ycnum - num); i++) {
                            short ycval = dsd[tdNo].shmbuf[i * 2 + 3] * 256 + dsd[tdNo].shmbuf[i * 2 + 4];
                            SetYcValue_short(rtuno, num + i, (BYTE *) &ycval, 1, utctime);
                        }
                        break;

                    case 4: // 二字节有符号遥测帧(10)高位符号其余绝对值
                        ycnum = rtupara->ycnum;
                        num = getoffset(rtudata[rtuno].txPtr, 1, rtupara->type);
                        for (i = 0; i < MIN(datalen / 2, ycnum - num); i++) {
                            short ycval;
                            int v = dsd[tdNo].shmbuf[i * 2 + 3] * 256 + dsd[tdNo].shmbuf[i * 2 + 4];
                            if (v & 0x8000)
                                ycval = (-1) * (v & 0x7fff);
                            else
                                ycval = v;
                            SetYcValue_short(rtuno, num + i, (BYTE *) &ycval, 1, utctime);
                        }
                        break;
                    case 5: // 二字节无符号遥测帧(10)
                        ycnum = rtupara->ycnum;
                        num = getoffset(rtudata[rtuno].txPtr, 1, rtupara->type);
                        for (i = 0; i < MIN(datalen / 2, ycnum - num); i++) {
                            int ycval = dsd[tdNo].shmbuf[i * 2 + 3] * 256 + dsd[tdNo].shmbuf[i * 2 + 4];
                            SetYcValue_int(rtuno, num + i, (BYTE *) &ycval, 1, utctime);
                        }
                        break;
                    case 6: // 遥测四字节整数(高字前3210)反序
                        ycnum = rtupara->ycnum;
                        num = getoffset(rtudata[rtuno].txPtr, 1, rtupara->type);
                        for (i = 0; i < MIN(datalen / 4, ycnum - num); i++) {
                            int ycval = (dsd[tdNo].shmbuf[i * 4 + 3] * 256 + dsd[tdNo].shmbuf[i * 4 + 4]) * 65536 +
                                        (dsd[tdNo].shmbuf[i * 4 + 5] * 256 + dsd[tdNo].shmbuf[i * 4 + 6]);
                            SetYcValue_int(rtuno, num + i, (BYTE *) &ycval, 1, utctime);
                        }
                        break;

                    case 7: // 遥测四字节浮点(高字前2301)反序
                        ycnum = rtupara->ycnum;
                        num = getoffset(rtudata[rtuno].txPtr, 1, rtupara->type);
                        for (i = 0; i < MIN(datalen / 4, ycnum - num); i++) {
                            int ycval =
                                    (dsd[tdNo].shmbuf[3 + i * 4] * 256 + dsd[tdNo].shmbuf[3 + i * 4 + 1]) +
                                    (dsd[tdNo].shmbuf[3 + i * 4 + 2] * 256 + dsd[tdNo].shmbuf[3 + i * 4 + 3]) * 65536;
                            float y = 0;
                            y = *((float *) &ycval);
                            y *= 100;
                            SetYcValue_float(rtuno, num + i, (BYTE *) &y, 1, utctime);
                        }
                        break;
                    case 8: // 遥测四字节整数(低字前0123)正序
                        ycnum = rtupara->ycnum;
                        num = getoffset(rtudata[rtuno].txPtr, 1, rtupara->type);
                        for (i = 0; i < MIN(datalen / 4, ycnum - num); i++) {
                            int ycval = dsd[tdNo].shmbuf[i * 4 + 3] + dsd[tdNo].shmbuf[i * 4 + 4] * 256 +
                                        (dsd[tdNo].shmbuf[i * 4 + 5] + dsd[tdNo].shmbuf[i * 4 + 6] * 256) * 65536;
                            SetYcValue_int(rtuno, num + i, (BYTE *) &ycval, 1, utctime);
                        }
                        break;
                    case 9: // 遥测四字节浮点(低字前0123)正序
                        ycnum = rtupara->ycnum;
                        num = getoffset(rtudata[rtuno].txPtr, 1, rtupara->type);
                        for (i = 0; i < MIN(datalen / 4, ycnum - num); i++) {
                            int ycval = dsd[tdNo].shmbuf[i * 4 + 3] + dsd[tdNo].shmbuf[i * 4 + 4] * 256 +
                                        (dsd[tdNo].shmbuf[i * 4 + 5] + dsd[tdNo].shmbuf[i * 4 + 6] * 256) * 65536;
                            float y = 0;
                            y = *((float *) &ycval);
                            y *= 100;
                            SetYcValue_float(rtuno, num + i, (BYTE *) &y, 1, utctime);
                        }
                        break;
                    case 10: // 电度帧整数(0123正序)
                        ycnum = rtupara->kwhnum;
                        num = getoffset(rtudata[rtuno].txPtr, 2, rtupara->type);
                        for (i = 0; i < MIN(datalen / 4, ycnum - num); i++) {
                            DWORD ycval = dsd[tdNo].shmbuf[i * 4 + 3] + dsd[tdNo].shmbuf[i * 4 + 4] * 256 +
                                          (dsd[tdNo].shmbuf[i * 4 + 5] + dsd[tdNo].shmbuf[i * 4 + 6] * 256) * 65536;
                            SetKwhValue(rtuno, num + i, ycval, utctime);
                        }
                        break;
                    case 11: // 电度帧整数(3210反序)
                        ycnum = rtupara->kwhnum;
                        num = getoffset(rtudata[rtuno].txPtr, 2, rtupara->type);
                        for (i = 0; i < MIN(datalen / 4, ycnum - num); i++) {
                            DWORD ycval = (dsd[tdNo].shmbuf[i * 4 + 3] * 256 + dsd[tdNo].shmbuf[i * 4 + 4]) * 65536 +
                                          (dsd[tdNo].shmbuf[i * 4 + 5] * 256 + dsd[tdNo].shmbuf[i * 4 + 6]);
                            SetKwhValue(rtuno, num + i, ycval, utctime);
                        }
                        break;
                    case 12: // 电度帧整数(低字前1032)
                        ycnum = rtupara->kwhnum;
                        num = getoffset(rtudata[rtuno].txPtr, 2, rtupara->type);
                        for (i = 0; i < MIN(datalen / 4, ycnum - num); i++) {
                            DWORD ycval = (dsd[tdNo].shmbuf[i * 4 + 3] * 256 + dsd[tdNo].shmbuf[i * 4 + 4]) +
                                          (dsd[tdNo].shmbuf[i * 4 + 5] * 256 + dsd[tdNo].shmbuf[i * 4 + 6]) * 65536;
                            SetKwhValue(rtuno, num + i, ycval, utctime);
                        }
                        break;
                    case 13: // 电度帧浮点(0123正序)
                        ycnum = rtupara->kwhnum;
                        num = getoffset(rtudata[rtuno].txPtr, 2, rtupara->type);
                        for (i = 0; i < MIN(datalen / 4, ycnum - num); i++) {
                            DWORD ycval = dsd[tdNo].shmbuf[i * 4 + 3] + dsd[tdNo].shmbuf[i * 4 + 4] * 256 +
                                          (dsd[tdNo].shmbuf[i * 4 + 5] + dsd[tdNo].shmbuf[i * 4 + 6] * 256) * 65536;
                            float y = 0;
                            y = *((float *) &ycval);
                            ycval = y * 100;
                            SetKwhValue(rtuno, num + i, ycval, utctime);
                        }
                        break;
                    case 14: // 电度帧浮点(3210反序)
                        ycnum = rtupara->kwhnum;
                        num = getoffset(rtudata[rtuno].txPtr, 2, rtupara->type);
                        for (i = 0; i < MIN(datalen / 4, ycnum - num); i++) {
                            DWORD ycval = (dsd[tdNo].shmbuf[i * 4 + 3] * 256 + dsd[tdNo].shmbuf[i * 4 + 4]) * 65536 +
                                          (dsd[tdNo].shmbuf[i * 4 + 5] * 256 + dsd[tdNo].shmbuf[i * 4 + 6]);
                            float y = 0;
                            y = *((float *) &ycval);
                            ycval = y * 100;
                            SetKwhValue(rtuno, num + i, ycval, utctime);
                        }
                        break;

                    case 15: // 电度帧浮点(1032低字前)
                        ycnum = rtupara->kwhnum;
                        num = getoffset(rtudata[rtuno].txPtr, 2, rtupara->type);
                        for (i = 0; i < MIN(datalen / 4, ycnum - num); i++) {
                            DWORD ycval = (dsd[tdNo].shmbuf[i * 4 + 3] * 256 + dsd[tdNo].shmbuf[i * 4 + 4]) +
                                          (dsd[tdNo].shmbuf[i * 4 + 5] * 256 + dsd[tdNo].shmbuf[i * 4 + 6]) * 65536;
                            float y = 0;
                            y = *((float *) &ycval);
                            ycval = y * 100;
                            SetKwhValue(rtuno, num + i, ycval, utctime);
                        }
                        break;
                    case 16: // 遥测浮点（3210）
                        ycnum = rtupara->ycnum;

                        num = getoffset(rtudata[rtuno].txPtr, 1, rtupara->type);
                        for (i = 0; i < MIN(datalen / 4, ycnum - num); i++) {
                            int ycval =
                                    (dsd[tdNo].shmbuf[3 + i * 4] * 256 + dsd[tdNo].shmbuf[3 + i * 4 + 1]) * 256 * 256 +
                                    (dsd[tdNo].shmbuf[3 + i * 4 + 2] * 256 + dsd[tdNo].shmbuf[3 + i * 4 + 3]);
                            float y = 0;
                            y = *((float *) &ycval);
                            y *= 100;
                            SetYcValue_float(rtuno, num + i, (BYTE *) &y, 1, utctime);
                        }
                        break;
                    case 17: // 二字节有符号遥测帧(01)
                        ycnum = rtupara->ycnum;
                        num = getoffset(rtudata[rtuno].txPtr, 1, rtupara->type);
                        for (i = 0; i < MIN(datalen / 2, ycnum - num); i++) {
                            short ycval = dsd[tdNo].shmbuf[i * 2 + 3] + dsd[tdNo].shmbuf[i * 2 + 4] * 256;
                            SetYcValue_short(rtuno, num + i, (BYTE *) &ycval, 1, utctime);
                        }
                        break;

                    case 18: // 二字节有符号遥测帧(01)高位符号其余绝对值
                        ycnum = rtupara->ycnum;
                        num = getoffset(rtudata[rtuno].txPtr, 1, rtupara->type);
                        for (i = 0; i < MIN(datalen / 2, ycnum - num); i++) {
                            short ycval;
                            int v = dsd[tdNo].shmbuf[i * 2 + 3] + dsd[tdNo].shmbuf[i * 2 + 4] * 256;
                            if (v & 0x8000)
                                ycval = (-1) * (v & 0x7fff);
                            else
                                ycval = v;

                            SetYcValue_short(rtuno, num + i, (BYTE *) &ycval, 1, utctime);
                        }
                        break;
                    case 19: // 二字节无符号遥测帧(01)
                        ycnum = rtupara->ycnum;
                        num = getoffset(rtudata[rtuno].txPtr, 1, rtupara->type);
                        for (i = 0; i < MIN(datalen / 2, ycnum - num); i++) {
                            int ycval = dsd[tdNo].shmbuf[i * 2 + 3] + dsd[tdNo].shmbuf[i * 2 + 4] * 256;
                            SetYcValue_int(rtuno, num + i, (BYTE *) &ycval, 1, utctime);
                        }
                        break;
                    case 20: // 江苏电科院光伏项目-阳光逆变器
                        procdata_20(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 21: // 怀柔光伏项目-阳光逆变器
                        procdata_21(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 22: // 怀柔光伏项目-固特威36kW逆变器
                        procdata_22(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 26: // 江苏电科院光伏项目-nari逆变器
                        procdata_26(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 27: // 北京经研院-nari逆变器
                        procdata_27(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 28: // 北京经研院-气象站
                        procdata_28(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 29: // 江苏电科院储能项目PCS
                        procdata_29(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 30: // 江苏电科院储能项目BMS
                        procdata_30(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 31: // 清川院水光项目PCS
                        procdata_31(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 32: // 北菜配电室
                        procdata_32(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 33: // 怀柔光伏项目-固特威HT逆变器
                        procdata_33(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 34: // 怀柔光伏项目-华为逆变器
                        procdata_34(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 35: // 怀柔光伏项目-阳光逆变器
                        procdata_35(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 36: // 怀柔光伏项目-安科瑞电表
                        procdata_36(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 37: // 热失控传感器
                        procdata_37(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 38: // 超声波传感器
                        procdata_38(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 39: // 温度检测
                        procdata_39(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 40: // 电源
                        procdata_40(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 44: // 电信园-锦浪光伏逆变器-V19
                        procdata_44(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 47: // BCU
                        procdata_47(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 49: // PCS
                        procdata_49(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 52: // BMS-PLC液冷机
                        procdata_52(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 53: // BMS-电表
                        procdata_53(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 54: // BMS-消防
                        procdata_54(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                        break;
                }

                rtudata[rtuno].txPtr++;
                if (rtudata[rtupara->rtuNo].txPtr >= modbusrcnt) {
                    rtudata[rtupara->rtuNo].txPtr = 0;
                    rtuloop[rtuno]++;
                }

                return 0;
            } break;
            case 0x05: // 遥控
            {
                YKRETSTRUCT *ykret = GetYkRetPara(rtuno);
                RXCOMMAND Mail;
                if (GetCommandPara(rtuno, Mail) == FALSE)
                    return FALSE;
                RXCOMMAND *rxcommand = &Mail;
                ykret->rtuno = rtuno;
                ykret->ykno = rxcommand->data.yk[4] + rxcommand->data.yk[5] * 256;
                ykret->ret = 1;
                ykret->ykprocflag = 1;

                return 0;
            } break;
            case 0x06: // 单点遥调
            {
                YKRETSTRUCT *ykret = GetYkRetPara(rtuno);
                RXCOMMAND Mail;
                if (GetCommandPara(rtuno, Mail) == FALSE)
                    return FALSE;
                RXCOMMAND *rxcommand = &Mail;
                ykret->rtuno = rtuno;
                ykret->ykno = rxcommand->data.yk[4] + rxcommand->data.yk[5] * 256;
                ykret->ret = 1;
                ykret->ykprocflag = 1;
                //            if(rtupara->type == 1)
                //            {
                //                YKRETSTRUCT *ykret = GetYkRetPara(rtuno);
                //                RXCOMMAND Mail ;
                //                if(GetCommandPara(rtuno,Mail) == FALSE) return FALSE;
                //                RXCOMMAND *rxcommand = &Mail;
                //                ykret->rtuno = rtuno;
                //                ykret->ykno = rxcommand->data.yk[4]+rxcommand->data.yk[5]*256;
                //                ykret->ret = 1;
                //                ykret->ykprocflag = 1;
                //            }
                //            else
                //            {
                //                RXCOMMAND *rxcommand = GetRxcommand(rtuno);
                //                if(rxcommand->data.buf[0] == 0x01)
                //                {
                //                    rxcommand->data.buf[0] = rxcommand->data.buf[0] + 0x01;
                //                }
                //                else
                //                {
                //                    YKRETSTRUCT *ykret = GetYkRetPara(rtuno);
                //                    RXCOMMAND Mail ;
                //                    if(GetCommandPara(rtuno,Mail) == FALSE) return FALSE;
                //                    RXCOMMAND *rxcommand = &Mail;
                //                    ykret->rtuno = rtuno;
                //                    ykret->ykno = rxcommand->data.yk[4]+rxcommand->data.yk[5]*256;
                //                    ykret->ret = 1;
                //                    ykret->ykprocflag = 1;
                //                }

                //            }
                return 0;
            } break;
            case 0x10: // 多点遥调
            {
                YKRETSTRUCT *ykret = GetYkRetPara(rtuno);
                RXCOMMAND Mail;
                if (GetCommandPara(rtuno, Mail) == FALSE)
                    return FALSE;
                RXCOMMAND *rxcommand = &Mail;
                ykret->rtuno = rtuno;
                ykret->ykno = rxcommand->data.yk[4] + rxcommand->data.yk[5] * 256;
                ykret->ret = 1;
                ykret->ykprocflag = 1;
                rtudata[rtuno].txPtr++;
                if (rtudata[rtupara->rtuNo].txPtr >= modbusrcnt) {
                    rtudata[rtupara->rtuNo].txPtr = 0;
                    // chanloop[tdNo]++;
                    rtuloop[rtuno]++;
                }
                return 0;
            } break;
            case 0x95: // 写文件
            {
                fileFlag = 0;
                free(filebuf);
                filebuf = NULL;
                fileFlag = 0;
                fileType = 0;
                fileGroupNo = 0;
                fileGroupNum = 0;
                fileReadNum = 0;
                return 0;
            } break;
        }
    }
    return 1;
}

int CModbus::procdata_29(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    int yxno, no = 0;
    if (startaddr == 900) {
        yxno = 0;
        procdata_yc_word(rtuno, 0, 19, buf + no, byteorder, 0x01);
        no += 38;
        procdata_yx(rtuno, yxno, 4, 1, buf + no, byteorder);
        no += 1;
        yxno += 4;
        procdata_yx(rtuno, yxno, 5, 2, buf + no, byteorder);
        no += 1;
        yxno += 5;
        procdata_yx(rtuno, yxno, 4, 1, buf + no, byteorder);
        no += 1;
        yxno += 4;
        procdata_yx(rtuno, yxno, 2, 6, buf + no, byteorder);
        no += 1;
        yxno += 2;
        procdata_yx(rtuno, yxno, 8, 0, buf + no, byteorder);
        no += 1;
        yxno += 8;
        procdata_yx(rtuno, yxno, 8, 0, buf + no, byteorder);
        no += 1;
        yxno += 8;
        procdata_yx(rtuno, yxno, 8, 0, buf + no, byteorder);
        no += 1;
        yxno += 8;

    } else if (startaddr == 957) {
        procdata_yc_word(rtuno, 19, 3, buf, byteorder, 0x01);
    }
    return 1;
}

int CModbus::procdata_32(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x01; // 字序，高字在前
    int ycno, no;

    if (startaddr == 256) {
        ycno = 0;
        no = 0;
        procdata_yc_word(rtuno, ycno, 9, buf + no, byteorder, 0x00);
        ycno += 9;
        no += 18;
        procdata_yc_word(rtuno, ycno, 12, buf + no, byteorder, 0x01);
        ycno += 12;
        no += 24;
        procdata_yc_word(rtuno, ycno, 5, buf + no, byteorder, 0x01);
        ycno += 5;
        no += 10;
    } else if (startaddr == 63) {
        ycno = 26;
        procdata_yc_dword(rtuno, ycno, 4, buf, byteorder, wordorder, 0x00);
    }

    return 1;
}

int CModbus::procdata_31(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x00; // 字序，低字在前
    int ycno, yxno, no;

    if (startaddr == 15205) {
        ycno = 0;
        procdata_yc_word(rtuno, ycno, 4, buf, byteorder, 0x00);
    } else if (startaddr == 20102) {
        ycno = 4;
        procdata_yc_word(rtuno, ycno, 2, buf, byteorder, 0x00);
    } else if (startaddr == 25201) {
        ycno = 6, yxno = 0, no = 0;
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
        ycno += 2;
        no += 4;
        no += 4;
        procdata_yc_word(rtuno, ycno, 12, buf + no, byteorder, 0x00);
        ycno += 12;
        no += 24;
        no += 16;
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
        ycno += 2;
        no += 4;
        no += 12;
        procdata_yc_word(rtuno, ycno, 3, buf + no, byteorder, 0x00);
        ycno += 3;
        no += 6;
        no += 2;
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno++;
        no += 2;
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno++;
        no += 2;
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno++;
        no += 2;
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno++;
        no += 2;
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno++;
        no += 2;
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno++;
        no += 2;

    } else if (startaddr == 25261) {
        yxno = 6;
        no = 0;
        procdata_yx(rtuno, yxno, 16, 0, buf + no, byteorder);
        yxno += 16;
        no += 2;
        procdata_yx(rtuno, yxno, 15, 0, buf + no, byteorder);
        yxno += 15;
        no += 2;
        no += 4;
        procdata_yx(rtuno, yxno, 11, 0, buf + no, byteorder);
        yxno += 11;
        no += 2;
    }

    return 1;
}

int CModbus::procdata_30(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x00; // 字序，低字在前
    int ycno, no = 0;
    if (startaddr == 10000) {
        ycno = 0;
        procdata_yc_word(rtuno, ycno, 15, buf + no, byteorder, 0x00);
        ycno += 15;
        no += 30;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
        ycno += 1;
        no += 2;
        procdata_yc_word(rtuno, ycno, 11, buf + no, byteorder, 0x00);
        ycno += 11;
        no += 22;
        procdata_yc_dword(rtuno, ycno, 10, buf + no, byteorder, wordorder, 0x00);
        ycno += 10;
        no += 40;
        procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x00);
    } else if (startaddr == 10064) {
        ycno = 41;
        procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x00);
    } else if (startaddr == 10128) {
        ycno = 63;
        procdata_yc_word(rtuno, ycno, 100, buf + no, byteorder, 0x00);

    } else if (startaddr == 10228) {
        ycno = 163;
        procdata_yc_word(rtuno, ycno, 100, buf + no, byteorder, 0x00);

    } else if (startaddr == 10328) {
        ycno = 263;
        procdata_yc_word(rtuno, ycno, 100, buf + no, byteorder, 0x00);
    } else if (startaddr == 10428) {
        ycno = 363;
        procdata_yc_word(rtuno, ycno, 100, buf + no, byteorder, 0x00);
    } else if (startaddr == 11152) {
        ycno = 463;
        procdata_yc_word(rtuno, ycno, 120, buf + no, byteorder, 0x00);
    }

    return 1;
}

int CModbus::procdata_28(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    procdata_yc_word(rtuno, 0, 13, buf, byteorder, 0x01);
    return 1;
}

int CModbus::procdata_27(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x00; // 字序，低字在前
    int ycno = 0, no = 0;
    if (startaddr == 0) {
        procdata_yc_word(rtuno, ycno, 25, buf, byteorder, 0x01);
    } else if (startaddr == 26) {
        ycno = 25;
        procdata_yc_dword(rtuno, ycno, 4, buf, byteorder, wordorder, 0x00);
    }

    return 1;
}


int CModbus::procdata_26(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x00; // 字序，低字在前
    int ycno = 0, i, no = 0;
    procdata_yc_word(rtuno, ycno, 10, buf + no, byteorder, 0x01);
    ycno += 10;
    no += 20;
    no += 6;
    procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
    ycno += 1;
    no += 2;
    no += 6;
    procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x01);
    ycno += 2;
    no += 4;
    no += 2;
    procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
    ycno += 1;
    no += 2;
    no += 4;
    procdata_yc_dword(rtuno, ycno, 6, buf + no, byteorder, wordorder, 0x00);
    return 1;
}

int CModbus::procdata_21(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x01; // 字序，低字在前
    int yxno, ycno, i, no;
    switch (startaddr) {
        case 5002:
            ycno = 0;
            no = 0;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            procdata_yc_dword(rtuno, ycno, 2, buf + no, byteorder, wordorder, 0x00);
            ycno += 2;
            no += 8;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            procdata_yc_word(rtuno, ycno, 6, buf + no, byteorder, 0x00);
            ycno += 6;
            no += 12;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            procdata_yc_word(rtuno, ycno, 6, buf + no, byteorder, 0x00);
            ycno += 6;
            no += 12;
            no += 12;
            procdata_yc_dword(rtuno, ycno, 2, buf + no, byteorder, wordorder, 0x00);
            ycno += 2;
            no += 8;

            break;
        case 5034:
            ycno = 20;
            no = 0;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            no += 2;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2; // 设备工作状态
            no += 12;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2; // 告警码
            no += 6;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            break;
        case 5070:
            ycno = 25;
            no = 0;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            no += 10;
            procdata_yc_dword(rtuno, ycno, 3, buf + no, byteorder, wordorder, 0x00);
            no += 8;
            yxno = 1;
            procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder); // 停机
            yxno = 2;
            procdata_yx(rtuno, yxno, 1, 4, buf + no, byteorder); // 待机
            yxno = 4;
            procdata_yx(rtuno, yxno, 5, 9, buf + no, byteorder); // 其他
            no += 2;
            yxno = 0;
            procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder); // 运行
            yxno = 3;
            procdata_yx(rtuno, yxno, 1, 2, buf + no, byteorder); // 故障

            break;
        case 5112:
            ycno = 29;
            no = 0;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            no += 2;
            procdata_yc_word(rtuno, ycno, 10, buf + no, byteorder, 0x00);
            ycno += 10;
            no += 20;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            no += 4;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            procdata_yc_word(rtuno, ycno, 8, buf + no, byteorder, 0x00);
            ycno += 8;
            no += 16;
            no += 4;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4; // 逆变器状态
            no += 4;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            break;
        case 5145:
            ycno = 53;
            no = 0;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4;
            break;
        case 7012:
            ycno = 56;
            no = 0;
            procdata_yc_word(rtuno, ycno, 24, buf + no, byteorder, 0x00);
            ycno += 24;
            no += 48;
            break;
        case 5005:
            yxno = 8;
            no = 0;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno += 1;
            no += 2;
            procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
            yxno += 1;
            no += 2;
            break;
        case 5035:
            yxno = 10;
            ycno = 80;
            no = 0;
            procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
            yxno += 1;
            no += 2;
            no += 4;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            break;
    }
    return 1;
}

int CModbus::procdata_34(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x01; // 字序，高字在前
    int yxno, ycno, i, no;
    switch (startaddr) {
        case 30071:
            ycno = 0;
            no = 0;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4;
            procdata_yc_dword(rtuno, ycno, 5, buf + no, byteorder, wordorder, 0x00);
            break;
        case 32000:
            yxno = 0;
            ycno = 7;
            no = 0;
            procdata_yx(rtuno, yxno, 9, 0, buf + no, byteorder);
            yxno += 9;
            no += 2;
            procdata_yx(rtuno, yxno, 3, 0, buf + no, byteorder);
            yxno += 3;
            no += 2;
            procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
            yxno += 2;
            no += 2;
            procdata_yx(rtuno, yxno, 16, 0, buf + no, byteorder);
            yxno += 16;
            no += 2;
            procdata_yx(rtuno, yxno, 16, 0, buf + no, byteorder);
            yxno += 16;
            no += 2;
            procdata_yx(rtuno, yxno, 16, 0, buf + no, byteorder);
            yxno += 16;
            no += 2;
            procdata_yx(rtuno, yxno, 16, 0, buf + no, byteorder);
            yxno += 16;
            no += 2;
            procdata_yx(rtuno, yxno, 4, 0, buf + no, byteorder);
            yxno += 4;
            no += 2;
            procdata_yc_word(rtuno, ycno, 48, buf + no, byteorder, 0x01);
            break;
        case 32064:
            ycno = 63;
            no = 0;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            procdata_yc_word(rtuno, ycno, 6, buf + no, byteorder, 0x00);
            ycno += 6;
            no += 12;
            procdata_yc_dword(rtuno, ycno, 6, buf + no, byteorder, wordorder, 0x00);
            ycno += 6;
            no += 24;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            no += 12;
            procdata_yc_dword(rtuno, ycno, 2, buf + no, byteorder, wordorder, 0x00);
            ycno += 2;
            no += 8;
            break;
        case 32344:
            ycno = 55;
            no = 0;
            procdata_yc_word(rtuno, ycno, 8, buf + no, byteorder, 0x01);
            ycno += 8;
            no += 16;
            break;
        case 35300:
            ycno = 83;
            no = 0;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            break;
        case 40120:
            ycno = 89;
            no = 0;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            no += 4;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            break;
    }
    return 1;
}

int CModbus::procdata_33(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x01; // 字序，高字在前
    int yxno, ycno, i, no;
    Self64 utctime = GetCurMSTime();
    switch (startaddr) {
        case 32016:
            ycno = 0;
            no = 0;
            procdata_yc_word(rtuno, ycno, 48, buf + no, byteorder, 0x01);
            break;
        case 32066:
            ycno = 48;
            no = 0;
            procdata_yc_word(rtuno, ycno, 6, buf + no, byteorder, 0x00);
            ycno += 6;
            no += 12;
            procdata_yc_dword(rtuno, ycno, 3, buf + no, byteorder, wordorder, 0x00);
            ycno += 3;
            no += 12;
            no += 4;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            no += 6;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            no += 2;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            no += 36;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            no += 1;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            break;
        case 35710:
            ycno = 62;
            yxno = 0;
            no = 0;
            procdata_yx(rtuno, yxno, 1, 15, buf, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 14, buf, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 13, buf, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 9, buf, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 8, buf, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 4, buf, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 3, buf, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 2, buf, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 1, buf, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 0, buf, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 15, buf + 2, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 13, buf + 2, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 12, buf + 2, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 11, buf + 2, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 10, buf + 2, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 9, buf + 2, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 8, buf + 2, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 7, buf + 2, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 6, buf + 2, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 5, buf + 2, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 2, buf + 2, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 1, buf + 2, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 0, buf + 2, byteorder);
            yxno++;
            no += 4;
            no += 12;
            procdata_yx(rtuno, yxno, 1, 14, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 13, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 12, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 6, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 5, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 2, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            procdata_yx(rtuno, yxno, 1, 15, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 13, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 12, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 11, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 10, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 9, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 8, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 7, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 6, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 5, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 4, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 3, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 2, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            no += 60;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            no += 14;
            yxno = 46;
            SetSingleYxValue(rtuno, yxno, 0x00, utctime, 0);
            SetSingleYxValue(rtuno, yxno + 1, 0x00, utctime, 0);
            SetSingleYxValue(rtuno, yxno + 2, 0x00, utctime, 0);
            SetSingleYxValue(rtuno, yxno + 3, 0x00, utctime, 0);
            SetSingleYxValue(rtuno, yxno + *(buf + no + 1), 0x01, utctime, 0);
            break;
        case 41322:
            ycno = 63;
            no = 0;
            yxno = 50;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            no += 4;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            no += 6;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            break;
        case 41480:
            ycno = 66;
            no = 0;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x01);
            break;
    }
    return 1;
}

int CModbus::procdata_22(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x01; // 字序，高字在前
    int yxno, ycno, no;
    Self64 utctime = GetCurMSTime();
    switch (startaddr) {
        case 0:
            ycno = 0;
            no = 0;
            procdata_yc_word(rtuno, ycno, 6, buf + no, byteorder, 0x00);
        case 256: // yt
            ycno = 6;
            no = 0;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4;
            procdata_yc_dword(rtuno, ycno, 2, buf + no, byteorder, wordorder, 0x00);
            ycno += 2;
            no += 8;
            break;
        case 768:
            yxno = 27;
            ycno = 10;
            no = 0;
            procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x01);
            ycno += 4;
            no += 8;
            procdata_yc_word(rtuno, ycno, 9, buf + no, byteorder, 0x00);
            ycno += 9;
            no += 18;
            no += 2;
            SetSingleYxValue(rtuno, yxno, 0x00, utctime, 0);
            SetSingleYxValue(rtuno, yxno + 1, 0x00, utctime, 0);
            SetSingleYxValue(rtuno, yxno + 2, 0x00, utctime, 0);
            SetSingleYxValue(rtuno, yxno + 3, 0x00, utctime, 0);
            SetSingleYxValue(rtuno, *(buf + no + 1), 0x01, utctime, 0);
            no += 2;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;

            procdata_yx(rtuno, yxno, 1, 15, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 14, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 13, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 9, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 7, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 6, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 5, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 4, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 3, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 2, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            procdata_yx(rtuno, yxno, 1, 15, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 14, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 13, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 12, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 11, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 10, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 9, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 8, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 7, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 6, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 4, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 3, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 2, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            procdata_yc_dword(rtuno, ycno, 2, buf + no, byteorder, wordorder, 0x00);
            ycno += 2;
            no += 8;
            no += 6;
            yxno = 31;
            procdata_yx(rtuno, yxno, 1, 13, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 7, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 3, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 2, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            no += 12;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            break;
        case 850:
            ycno = 27;
            no = 0;
            yxno = 37;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            no += 6;
            procdata_yc_word(rtuno, ycno, 20, buf + no, byteorder, 0x01);
            ycno += 20;
            no += 40;
            procdata_yx(rtuno, yxno, 15, 0, buf + no, byteorder);
            break;
    }
    return 1;
}

int CModbus::procdata_37(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    int ycno, yxno, no, i;
    unsigned short tempdata;
    switch (startaddr) {
        case 1:
            ycno = 0;
            yxno = 0;
            no = 0;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            procdata_yc_word(rtuno, ycno, 27, buf + no, byteorder, 0x00);
            ycno += 27;
            no += 54;
            break;
        case 0:
            ycno = 27;
            yxno = 1;
            no = 0;
            procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x00);
            ycno += 4;
            no += 8;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            for (i = 0; i < 3; i++) {
                tempdata = (*(buf + no)) * 256 + (*(buf + no + 1));
                if (tempdata != 0) {
                    SetSingleYxValue(rtuno, tempdata - 1 + yxno, 0x01);
                }
                no += 2;
                yxno += 7;
            }
            break;
    }
    return 1;
}

int CModbus::procdata_38(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    int ycno, yxno, no, i;
    unsigned short tempdata;
    switch (startaddr) {
        case 1:
            ycno = 0;
            no = 0;
            procdata_yc_word(rtuno, ycno, 11, buf + no, byteorder, 0x00);
            break;
        case 0:
            ycno = 11;
            yxno = 0;
            no = 0;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            tempdata = (*(buf + no)) * 256 + (*(buf + no + 1));
            if (tempdata != 0) {
                SetSingleYxValue(rtuno, tempdata - 1 + yxno, 0x01);
            }
            break;
    }
    return 1;
}

int CModbus::procdata_39(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x00; // 字序，高字在前
    int ycno, no;
    switch (startaddr) {
        case 0:
            ycno = 0;
            no = 0;
            procdata_yc_dword(rtuno, ycno, 32, buf + no, byteorder, wordorder, 0x00);
            break;
        case 64:
            ycno = 32;
            no = 0;
            procdata_yc_dword(rtuno, ycno, 32, buf + no, byteorder, wordorder, 0x00);
            break;
        case 160:
            ycno = 64;
            no = 0;
            procdata_yc_word(rtuno, ycno, 3, buf + no, byteorder, 0x00);
            break;
    }
    return 1;
}

int CModbus::procdata_40(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x00; // 字序，高字在前
    int ycno, yxno, no;

    switch (startaddr) {
        case 1:
            yxno = 0;
            no = 0;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;

            break;
        case 16:
            ycno = 0;
            no = 0;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            break;
        case 32:
            ycno = 5;
            no = 0;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            break;
        case 48:
            ycno = 3;
            no = 0;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4;
    }
    return 1;
}

int CModbus::procdata_36(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x00; // 字序，高字在前
    int ycno, no;
    switch (startaddr) {
        case 0:
            ycno = 0;
            no = 0;
            procdata_yc_dword(rtuno, ycno, 15, buf + no, byteorder, wordorder, 0x00);
            break;
        case 97:
            ycno = 15;
            no = 0;
            procdata_yc_word(rtuno, ycno, 6, buf + no, byteorder, 0x00);
            ycno += 6;
            no += 12;
            no += 32;
            procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x00);
            break;
        case 356:
            ycno = 25;
            no = 0;
            procdata_yc_dword(rtuno, ycno, 12, buf + no, byteorder, wordorder, 0x00);
            ycno += 12;
            no += 48;
            procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x01);
            break;
    }
    return 1;
}

int CModbus::procdata_54(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    int ycno = 0, yxno = 0, no = 0;
    int temp;
    unsigned short data;

    temp = *(buf + no); // VOC 状态
    SetYcValue_int(rtuno, ycno, (BYTE *) &temp, 1);
    ycno += 1;
    if (temp == 0 || temp == 0xff)
        data = 0;
    else if (temp == 2)
        data = 1;
    else if (temp == 4)
        data = 2;
    else if (temp == 5)
        data = 4;
    else if (temp == 8)
        data = 8;
    else if (temp == 11)
        data = 16;
    else if (temp == 10)
        data = 32;
    procdata_yx(rtuno, yxno, 6, 0, (BYTE *) &data, byteorder);
    yxno += 6;
    no += 1;

    temp = *(buf + no); // 系统状态
    SetYcValue_int(rtuno, ycno, (BYTE *) &temp, 1);
    ycno += 1;
    if (temp == 0 || temp == 0xff)
        data = 0;
    else if (temp == 2)
        data = 1;
    else if (temp == 4)
        data = 2;
    else if (temp == 5)
        data = 4;
    else if (temp == 8)
        data = 8;
    else if (temp == 11)
        data = 16;
    else if (temp == 10)
        data = 32;
    procdata_yx(rtuno, yxno, 6, 0, (BYTE *) &data, byteorder);
    yxno += 6;
    no += 1;

    temp = *(buf + no); // 当前温度
    SetYcValue_short(rtuno, ycno, (BYTE *) &temp, 1);
    ycno += 1;
    no += 1;

    temp = *(buf + no); // 温度报警状态
    SetYcValue_int(rtuno, ycno, (BYTE *) &temp, 1);
    ycno += 1;
    if (temp == 0 || temp == 0xff)
        data = 0;
    else if (temp == 2)
        data = 1;
    else if (temp == 4)
        data = 2;
    else if (temp == 5)
        data = 4;
    else if (temp == 8)
        data = 8;
    else if (temp == 11)
        data = 16;
    else if (temp == 10)
        data = 32;
    procdata_yx(rtuno, yxno, 6, 0, (BYTE *) &data, byteorder);
    yxno += 6;
    no += 1;

    procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
    ycno += 2;
    no += 4; // CO 浓度H2 浓度

    temp = *(buf + no + 1); // 烟雾报警状态
    SetYcValue_int(rtuno, ycno, (BYTE *) &temp, 1);
    ycno += 1;
    if (temp == 0 || temp == 0xff)
        data = 0;
    else if (temp == 2)
        data = 1;
    else if (temp == 4)
        data = 2;
    else if (temp == 5)
        data = 4;
    else if (temp == 8)
        data = 8;
    else if (temp == 11)
        data = 16;
    else if (temp == 10)
        data = 32;
    procdata_yx(rtuno, yxno, 6, 0, (BYTE *) &data, byteorder);
    yxno += 6;
    no += 2;

    procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
    yxno += 1;
    no += 2;
    procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
    yxno += 1;
    no += 2;
}

int CModbus::procdata_53(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x01; // 字序，低字在前
    int ycno = 0, no = 0;
    if (startaddr == 0 && datalen == 128) {
        procdata_yc_dword(rtuno, ycno, 15, buf + no, byteorder, wordorder, 0x00);
        ycno += 15;
        no += 60;
        no += 60;
        procdata_yc_word(rtuno, ycno, 3, buf + no, byteorder, 0x00);
        ycno += 3;
        no += 6;
    } else if (startaddr == 97 && datalen == 92) {
        ycno = 21;
        procdata_yc_word(rtuno, ycno, 6, buf + no, byteorder, 0x00);
        ycno += 6;
        no += 12;
        procdata_yc_word(rtuno, ycno, 16, buf + no, byteorder, 0x01);
        ycno += 16;
        no += 32;
        procdata_yc_word(rtuno, ycno, 16, buf + no, byteorder, 0x00);
        ycno += 16;
        no += 32;
        procdata_yc_dword(rtuno, ycno, 3, buf + no, byteorder, wordorder, 0x00);
        ycno += 3;
        no += 12;
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
        ycno += 2;
        no += 4;
    } else if (startaddr == 289 && datalen == 4) {
        ycno = 64;
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
        ycno += 2;
        no += 4;
    } else if (startaddr == 4353 && datalen == 120) {
        ycno = 66;
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
        ycno += 2;
        no += 4;
        procdata_yc_dword(rtuno, ycno, 23, buf + no, byteorder, wordorder, 0x00);
        ycno += 23;
        no += 92;
        procdata_yc_word(rtuno, ycno, 12, buf + no, byteorder, 0x00);
        ycno += 12;
        no += 24;
    }
}

int CModbus::procdata_52(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x01; // 字序，低字在前
    int ycno = 0, yxno = 0, no = 0;
    if (startaddr == 1024 && datalen == 6) {
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2;
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
    } else if (startaddr == 41739 && datalen == 8) {
        yxno = 1;
        ycno = 2;
        procdata_yc_word(rtuno, ycno, 3, buf + no, byteorder, 0x00);
        ycno += 3;
        no += 6;
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
    } else if (startaddr == 58880 && datalen == 46) {
        ycno = 5;
        yxno = 2;
        procdata_yc_word(rtuno, ycno, 21, buf + no, byteorder, 0x00);
        ycno += 21;
        no += 42;
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
    } else if (startaddr == 59136 && datalen == 98) {
        ycno = 28;
        yxno = 3;
        procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x01);
        ycno += 4;
        no += 8;
        procdata_yc_word(rtuno, ycno, 23, buf + no, byteorder, 0x00);
        ycno += 2;
        no += 4;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1;
        no += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1;
        no += 2;

        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2;
        ycno += 1;
        no += 4;
        procdata_yc_dword(rtuno, ycno, 8, buf + no, byteorder, wordorder, 0x00);
        ycno += 8;
        no += 32;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
    }

    return 1;
}

int CModbus::procdata_49(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    int ycno = 0, yxno = 0, no = 0;
    if (startaddr == 3 && datalen == 124) {
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x01);
        ycno += 2;
        no += 4; // 3,4
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x01);
        ycno += 2;
        no += 4; // 5,6
        procdata_yc_word(rtuno, ycno, 3, buf + no, byteorder, 0x00);
        ycno += 3;
        no += 6; // 7,8,9
        procdata_yc_word(rtuno, ycno, 7, buf + no, byteorder, 0x01);
        ycno += 7;
        no += 14; // 10-16
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1;
        no += 2; // 17
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x01);
        ycno += 2;
        no += 2; // 18
        no += 2;
        procdata_yc_word(rtuno, ycno, 3, buf + no, byteorder, 0x01);
        ycno += 3;
        no += 2; // 20
        no += 4;
        procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x01);
        ycno += 4;
        no += 2; // 23
        no += 6;
        procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x00);
        ycno += 4; // 27,28,29,30
        procdata_yx(rtuno, yxno, 7, 0, buf + no, byteorder);
        yxno += 7;
        no += 2; // 27
        no += 2;
        procdata_yx(rtuno, yxno, 16, 0, buf + no, byteorder);
        yxno += 16;
        no += 2; // 29
        procdata_yx(rtuno, yxno, 16, 0, buf + no, byteorder);
        yxno += 16;
        no += 2; // 30
        procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x00);
        ycno += 4;
        no += 8;
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; //
        procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x00);
        ycno += 4;
        no += 8;
        no += 12;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
        ycno += 1;
        no += 2; // 46
        no += 2;
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
        ycno += 2;
        no += 4; // 48,49
        no += 2;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
        ycno += 1;
        no += 2; // 51
        no += 6;
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
        ycno += 2;
        no += 4; // 55,56
        procdata_yc_word(rtuno, ycno, 8, buf + no, byteorder, 0x00);
        ycno += 8;
        no += 16; // 57
    }

    return 1;
}

int CModbus::procdata_47(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    int ycno, yxno, no = 0;
    if (startaddr == 1 && datalen == 86) {
        ycno = 0;
        yxno = 0;
        procdata_yc_word(rtuno, ycno, 6, buf + no, byteorder, 0x00);
        ycno += 6;
        no += 12;
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; // Pack均衡控制模式
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1; // Pack均衡启停使能
        procdata_yx(rtuno, yxno, 16, 0, buf + no, byteorder);
        yxno += 16;
        no += 2; // PackX均衡启停使能
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; // Pack风冷控制模式
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1; // Pack风冷启停使能
        procdata_yx(rtuno, yxno, 16, 0, buf + no, byteorder);
        yxno += 16;
        no += 2; // PackX风冷启停使能
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; // 簇液冷控制模式
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; // 簇液冷控制使能
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; // 系统保护状态复位
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; // 分断路器
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; // 接触器控制模式
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; // 接触器自动分合使能
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; // 预充电接触器分合使能
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; // 正极接触器分合使能
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; // 负极接触器分合使能

        procdata_yc_word(rtuno, ycno, 15, buf + no, byteorder, 0x00);
        ycno += 15;
        no += 30; // 电芯电压充电截止值
        procdata_yc_word(rtuno, ycno, 3, buf + no, byteorder, 0x01);
        ycno += 3;
        no += 6; // 电芯低温液冷启动值
        procdata_yc_word(rtuno, ycno, 6, buf + no, byteorder, 0x00);
        ycno += 6;
        no += 12; // 簇内电芯温差风扇/液冷启动值

    } else if (startaddr == 1 && datalen == 220) {
        ycno = 32;
        yxno = 43;
        unsigned short status = 0, ret = 0;

        if (byteorder)
            status = (*buf) * 256 + (*(buf + 1));
        else
            status = (*buf) + (*(buf + 1)) * 256;

        if (status == 0x00)
            ret = 0x01;
        else if (status == 0x01)
            ret = 0x02;
        else if (status == 0x10)
            ret = 0x04;
        else if (status == 0x11)
            ret = 0x08;
        else if (status == 0x12)
            ret = 0x10;
        else if (status == 0xf0)
            ret = 0x20;
        else
            ret = 0x00;

        procdata_yx(rtuno, yxno, 6, 0, (unsigned char *) &ret, 0x00);
        yxno += 6; // 电池系统运行状态
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
        ycno += 2;
        no += 4; // 电池系统运行状态
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x01);
        ycno += 2;
        no += 4; // 簇电流
        procdata_yc_word(rtuno, ycno, 9, buf + no, byteorder, 0x01);
        ycno += 9;
        no += 18; // 簇SOC
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; // 簇液冷机状态
        procdata_yx(rtuno, yxno, 3, 0, buf + no, byteorder);
        yxno += 3;
        no += 2; // 预充、正极、负极接触器
        procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x00);
        ycno += 4;
        no += 8; // 最大电芯电压
        procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x01);
        ycno += 4;
        no += 8; // 最高温度
        ycno = 1222;
        yxno = 73;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1;
        no += 2; // Pack通讯状态
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; // 故障状态
        no += 8; // 预留
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
        ycno += 2;
        no += 4; // 软件版本
        ycno = 53;
        procdata_yc_word(rtuno, ycno, 79, buf + no, byteorder, 0x00); // 电压-电芯1-79

    } else if (startaddr == 111 && datalen == 220) {
        ycno = 132;
        procdata_yc_word(rtuno, ycno, 110, buf + no, byteorder, 0x00); // 电压-电芯80-189
    } else if (startaddr == 221 && datalen == 220) {
        ycno = 242;
        procdata_yc_word(rtuno, ycno, 99, buf + no, byteorder, 0x00);
        ycno += 99;
        no += 198; // 电压-电芯190-288
        procdata_yc_word(rtuno, ycno, 11, buf + no, byteorder, 0x01); // 温度1-11;

    } else if (startaddr == 331 && datalen == 220) {
        ycno = 352;
        procdata_yc_word(rtuno, ycno, 110, buf + no, byteorder, 0x01); // 温度12-121
    } else if (startaddr == 441 && datalen == 220) {
        ycno = 462;
        procdata_yc_word(rtuno, ycno, 110, buf + no, byteorder, 0x01); // 温度122-231
    } else if (startaddr == 551 && datalen == 220) {
        ycno = 572;
        procdata_yc_word(rtuno, ycno, 57, buf + no, byteorder, 0x01);
        ycno += 57;
        no += 114; // 温度232-288
        procdata_yc_word(rtuno, ycno, 53, buf + no, byteorder, 0x00); // 状态-电芯1-53
    } else if (startaddr == 661 && datalen == 220) {
        ycno = 682;
        procdata_yc_word(rtuno, ycno, 110, buf + no, byteorder, 0x00); // 状态-电芯54-163
    } else if (startaddr == 771 && datalen == 220) {
        ycno = 792;
        procdata_yc_word(rtuno, ycno, 110, buf + no, byteorder, 0x00); // 状态-电芯164-273
    } else if (startaddr == 881 && datalen == 220) {
        ycno = 902;
        yxno = 53;
        unsigned short status = 0, ret = 0;
        procdata_yc_word(rtuno, ycno, 32, buf + no, byteorder, 0x00);
        ycno += 32;
        no += 62; // 状态-电芯274-288,pack状态1-16,状态-簇1
        if (byteorder)
            status = (*(buf + no)) * 256 + (*(buf + no + 1));
        else
            status = (*(buf + no)) + (*(buf + no + 1)) * 256;
        ret = (0x01 << (status & 0x0f)) / 2;
        procdata_yx(rtuno, yxno, 6, 0, (unsigned char *) &ret, 0x00);
        yxno += 6;

        ret = (0x01 << ((status & 0xf0) >> 4)) / 2;
        if (!byteorder)
            ret = ret * 256;
        procdata_yx(rtuno, yxno, 6, 0, (unsigned char *) &ret, 0x00);
        yxno += 6;

        ret = (0x01 << ((status & 0x0f00) >> 8)) / 2;
        if (!byteorder)
            ret = ret * 256;
        procdata_yx(rtuno, yxno, 5, 0, (unsigned char *) &ret, 0x00);
        yxno += 5;

        ret = (0x01 << ((status & 0xf000) >> 12)) / 2;
        if (!byteorder)
            ret = ret * 256;
        procdata_yx(rtuno, yxno, 3, 0, (unsigned char *) &ret, 0x00);
        yxno += 3;
        no += 2;

        ycno = 1227;
        yxno = 74;
        if (byteorder)
            status = (*(buf + no)) * 256 + (*(buf + no + 1));
        else
            status = (*(buf + no)) + (*(buf + no + 1)) * 256;
        ret = (0x01 << (status & 0x0f)) / 2;
        procdata_yx(rtuno, yxno, 3, 0, (unsigned char *) &ret, 0x00);
        yxno += 3;

        ret = (0x01 << ((status & 0xf0) >> 4)) / 2;
        procdata_yx(rtuno, yxno, 3, 0, (unsigned char *) &ret, 0x00);
        yxno += 3;
        no += 2;

        no += 14;
        procdata_yc_word(rtuno, ycno, 70, buf + no, byteorder, 0x00); // 直流内阻-电芯1-70
    } else if (startaddr == 991 && datalen == 220) {
        ycno = 1004;
        procdata_yc_word(rtuno, ycno, 110, buf + no, byteorder, 0x00); // 直流内阻-电芯71-180
    } else if (startaddr == 1101 && datalen == 216) {
        ycno = 1114;
        procdata_yc_word(rtuno, ycno, 108, buf + no, byteorder, 0x00); // 直流内阻-电芯181-288
    }

    return 1;
}

int CModbus::procdata_44(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x00; // 字序，高字在前
    int ycno, yxno, no;
    switch (startaddr) {
        case 3004:
            ycno = 0, no = 0;
            procdata_yc_dword(rtuno, ycno, 4, buf + no, byteorder, wordorder, 0x00);
            ycno += 4;
            no += 16;
            no += 4;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            no += 2;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            break;
        case 3029:
            ycno = 6, no = 0;
            no += 2;
            procdata_yc_word(rtuno, ycno, 11, buf + no, byteorder, 0x00);
            ycno += 11;
            no += 22;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4;
            procdata_yc_dword(rtuno, ycno, 2, buf + no, byteorder, wordorder, 0x00);
            ycno += 2;
            no += 8;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4;
            procdata_yc_word(rtuno, ycno, 3, buf + no, byteorder, 0x01);
            ycno += 3;
            no += 6;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4;
            procdata_yc_dword(rtuno, ycno, 2, buf + no, byteorder, wordorder, 0x00);
            ycno += 2;
            no += 8;
            break;
        case 3298:
            ycno = 32, no = 0;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            procdata_yc_word(rtuno, ycno, 21, buf + no, byteorder, 0x00);
            ycno += 21;
            no += 42;
            break;
        case 3499:
            ycno = 54, no = 0;
            procdata_yc_word(rtuno, ycno, 10, buf + no, byteorder, 0x00);
            ycno += 10;
            no += 20;
        case 3529:
            ycno = 64, no = 0;
            procdata_yc_word(rtuno, ycno, 10, buf + no, byteorder, 0x01);
            ycno += 10;
            no += 20;
            break;
        case 3006:
            ycno = 74, yxno = 0, no = 0;
            procdata_yx(rtuno, yxno, 1, 5, buf + no, byteorder);
            yxno += 1;
            no += 2;
            no += 86;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            no += 34;
            procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
            yxno += 1;
            no += 2;
            procdata_yx(rtuno, yxno, 1, 5, buf + no, byteorder);
            yxno += 1;
            no += 2;
            break;
    }
    return 1;
}


int CModbus::procdata_35(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x01; // 字序，低字在前
    int ycno, yxno, i, j, no;
    switch (startaddr) {
        case 4999:
            ycno = 0, yxno = 0, no = 0;
            procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x00);
            ycno += 4;
            no += 8;
            procdata_yc_dword(rtuno, ycno, 2, buf + no, byteorder, wordorder, 0x00);
            ycno += 2;
            no += 8;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            procdata_yc_word(rtuno, ycno, 6, buf + no, byteorder, 0x00);
            ycno += 6;
            no += 12;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            procdata_yc_word(rtuno, ycno, 6, buf + no, byteorder, 0x00);
            ycno += 6;
            no += 12;
            no += 12;
            procdata_yc_dword(rtuno, ycno, 2, buf + no, byteorder, wordorder, 0x00);
            ycno += 2;
            no += 8;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            procdata_yc_word(rtuno, ycno, 3, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            no += 12;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            no += 6;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            no += 42;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            no += 10;
            procdata_yc_dword(rtuno, ycno, 3, buf + no, byteorder, wordorder, 0x00);
            ycno += 3;
            no += 12;
            no += 2;
            procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
            yxno++;
            no -= 2;
            procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
            yxno++;
            no += 2;
            procdata_yx(rtuno, yxno, 1, 2, buf + no, byteorder);
            yxno++;
            no -= 2;
            procdata_yx(rtuno, yxno, 5, 9, buf + no, byteorder);
            yxno += 5;
            break;
        case 5112:
            ycno = 33, no = 0;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            no += 2;
            procdata_yc_word(rtuno, ycno, 10, buf + no, byteorder, 0x00);
            ycno += 10;
            no += 20;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            no += 4;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4;
            no += 28;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            break;
        case 7012:
            ycno = 50, no = 0;
            procdata_yc_word(rtuno, ycno, 18, buf + no, byteorder, 0x00);
            ycno += 18;
            no += 36;
            break;
        case 5006:
            ycno = 68, yxno = 8, no = 0;
            procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
            yxno++;
            no += 58;
            no += 6;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            break;
    }
    return 1;
}

int CModbus::procdata_20(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x01; // 字序，低字在前
    int ycno, yxno, i, j, no;
    if (startaddr == 4999 && datalen == 166) {
        ycno = 0, yxno = 0, no = 0;
        procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x00);
        ycno += 4;
        no += 8;
        procdata_yc_dword(rtuno, ycno, 2, buf + no, byteorder, wordorder, 0x00);
        ycno += 2;
        no += 8;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
        ycno += 1;
        no += 2;
        procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
        ycno += 1;
        no += 4;
        procdata_yc_word(rtuno, ycno, 6, buf + no, byteorder, 0x00);
        ycno += 6;
        no += 12;
        procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
        ycno += 1;
        no += 4;
        procdata_yc_word(rtuno, ycno, 6, buf + no, byteorder, 0x00);
        ycno += 6;
        no += 12;
        no += 12;
        procdata_yc_dword(rtuno, ycno, 2, buf + no, byteorder, wordorder, 0x00);
        ycno += 2;
        no += 8;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
        ycno += 1;
        no += 2;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1;
        no += 2;
        no += 2;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1;
        no += 2;
        no += 12;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1;
        no += 2;
        no += 6;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1;
        no += 2;
        no += 42;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1;
        no += 2;
        no += 10;
        procdata_yc_dword(rtuno, ycno, 3, buf + no, byteorder, wordorder, 0x00);
        ycno += 3;
        no += 8;
        no += 2;
        procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
        yxno++;
        no -= 2;
        procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
        yxno++;
        no += 2;
        procdata_yx(rtuno, yxno, 1, 2, buf + no, byteorder);
        yxno++;
        no -= 2;
        procdata_yx(rtuno, yxno, 5, 9, buf + no, byteorder);
        yxno += 5;
    } else if (startaddr == 5112 && datalen == 70) {
        ycno = 32, no = 0;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1;
        no += 2;
        no += 2;
        procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x00);
        ycno += 4;
        no += 8;
        no += 12;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
        ycno += 1;
        no += 2;
        no += 4;
        procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
        ycno += 1;
        no += 4;
        no += 32;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
        ycno += 1;
        no += 2;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1;
        no += 2;
    } else if (startaddr == 7012 && datalen == 20) {
        ycno = 41, no = 0;
        procdata_yc_word(rtuno, ycno, 10, buf + no, byteorder, 0x00);
        ycno += 10;
        no += 20;
    } else if (startaddr == 5006 && datalen == 68) {
        ycno = 51, yxno = 8, no = 0;

        procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
        yxno++;
        no += 58;
        procdata_yx(rtuno, yxno, 1, 7, buf + no, byteorder);
        yxno++;
        procdata_yx(rtuno, yxno, 1, 7, buf + no - 2, byteorder);
        no += 6;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1;
        no += 2;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
        ycno += 1;
        no += 2;
    }
    //    switch (startaddr)
    //    {
    //    case 4999:
    //        ycno = 0,yxno = 0,no=0;
    //        procdata_yc_word(rtuno,ycno,4,buf+no,byteorder,0x00);ycno+=4;no+=8;
    //        procdata_yc_dword(rtuno,ycno,2,buf+no,byteorder,wordorder,0x00);ycno+=2;no+=8;
    //        procdata_yc_word(rtuno,ycno,1,buf+no,byteorder,0x01);ycno+=1;no+=2;
    //        procdata_yc_dword(rtuno,ycno,1,buf+no,byteorder,wordorder,0x00);ycno+=1;no+=4;
    //        procdata_yc_word(rtuno,ycno,6,buf+no,byteorder,0x00);ycno+=6;no+=12;
    //        procdata_yc_dword(rtuno,ycno,1,buf+no,byteorder,wordorder,0x00);ycno+=1;no+=4;
    //        procdata_yc_word(rtuno,ycno,6,buf+no,byteorder,0x00);ycno+=6;no+=12;
    //        no+=12;
    //        procdata_yc_dword(rtuno,ycno,2,buf+no,byteorder,wordorder,0x00);ycno+=2;no+=8;
    //        procdata_yc_word(rtuno,ycno,1,buf+no,byteorder,0x01);ycno+=1;no+=2;
    //        procdata_yc_word(rtuno,ycno,1,buf+no,byteorder,0x00);ycno+=1;no+=2;
    //        no+=2;
    //        procdata_yc_word(rtuno,ycno,1,buf+no,byteorder,0x00);ycno+=1;no+=2;
    //        no+=12;
    //        procdata_yc_word(rtuno,ycno,1,buf+no,byteorder,0x00);ycno+=1;no+=2;
    //        no+=6;
    //        procdata_yc_word(rtuno,ycno,1,buf+no,byteorder,0x00);ycno+=1;no+=2;
    //        no+=42;
    //        procdata_yc_word(rtuno,ycno,1,buf+no,byteorder,0x00);ycno+=1;no+=2;
    //        no+=10;
    //        procdata_yc_dword(rtuno,ycno,3,buf+no,byteorder,wordorder,0x00);ycno+=3;no+=8;
    //        no+=2;
    //        procdata_yx(rtuno,yxno,1,1,buf+no,byteorder);yxno++;
    //        no-=2;
    //        procdata_yx(rtuno,yxno,1,1,buf+no,byteorder);yxno++;
    //        no+=2;
    //        procdata_yx(rtuno,yxno,1,2,buf+no,byteorder);yxno++;
    //        no-=2;
    //        procdata_yx(rtuno,yxno,5,9,buf+no,byteorder);yxno+=5;
    //        break;
    //    case 5112:
    //        ycno = 32,no=0;
    //        procdata_yc_word(rtuno,ycno,1,buf+no,byteorder,0x00);ycno+=1;no+=2;
    //        no+=2;
    //        procdata_yc_word(rtuno,ycno,4,buf+no,byteorder,0x00);ycno+=4;no+=8;
    //        no+=12;
    //        procdata_yc_word(rtuno,ycno,1,buf+no,byteorder,0x01);ycno+=1;no+=2;
    //        no+=4;
    //        procdata_yc_dword(rtuno,ycno,1,buf+no,byteorder,wordorder,0x00);ycno+=1;no+=4;
    //        no+=32;
    //        procdata_yc_word(rtuno,ycno,1,buf+no,byteorder,0x01);ycno+=1;no+=2;
    //        procdata_yc_word(rtuno,ycno,1,buf+no,byteorder,0x00);ycno+=1;no+=2;
    //        break;
    //    case 7012:
    //        ycno = 41,no=0;
    //        procdata_yc_word(rtuno,ycno,10,buf+no,byteorder,0x00);ycno+=10;no+=20;
    //        break;
    //    case 5006:
    //        ycno = 51,yxno = 8,no=0;

    //        procdata_yx(rtuno,yxno,1,1,buf+no,byteorder);yxno++;
    //        no+=58;
    //        procdata_yx(rtuno,yxno,1,7,buf+no,byteorder);yxno++;
    //        procdata_yx(rtuno,yxno,1,7,buf+no-2,byteorder);
    //        no+=6;
    //        procdata_yc_word(rtuno,ycno,1,buf+no,byteorder,0x00);ycno+=1;no+=2;
    //        procdata_yc_word(rtuno,ycno,1,buf+no,byteorder,0x01);ycno+=1;no+=2;
    //        break;
    //    }
    return 1;
}

// datatype = 0x00 U32 & S32 ; = 0x01 float
void CModbus::procdata_yc_dword(int rtuno, int ycno, int ycnum, unsigned char *buf, char byteorder, char wordorder,
                                char datatype) {
    int ycvalue = 0;
    float ycvalue_f = 0;
    for (int i = 0; i < ycnum; i++) {
        if (wordorder) {
            if (byteorder) // 低字在前，高字节在前
            {
                ycvalue = *(buf + i * 4) * 256 + *(buf + i * 4 + 1) + *(buf + i * 4 + 2) * 256 * 256 * 256 +
                          *(buf + i * 4 + 3) * 256 * 256;
            } else // 低字在前，低字节在前
            {
                ycvalue = *(buf + i * 4) + *(buf + i * 4 + 1) * 256 + *(buf + i * 4 + 2) * 256 * 256 +
                          *(buf + i * 4 + 3) * 256 * 256 * 256;
            }
        } else {
            if (byteorder) // 高字在前，高字节在前
            {
                ycvalue = *(buf + i * 4) * 256 * 256 * 256 + *(buf + i * 4 + 1) * 256 * 256 + *(buf + i * 4 + 2) * 256 +
                          *(buf + i * 4 + 3);
            } else // 高字在前，低字节在前
            {
                ycvalue = *(buf + i * 4) * 256 * 256 + *(buf + i * 4 + 1) * 256 * 256 * 256 + *(buf + i * 4 + 2) +
                          *(buf + i * 4 + 3) * 256;
            }
        }
        Self64 utctime = GetCurMSTime();
        if (datatype) {
            ycvalue_f = *((float *) &ycvalue);
            ycvalue_f = ycvalue_f * 100;

            SetYcValue_float(rtuno, ycno + i, (BYTE *) &ycvalue_f, 1, utctime);
        } else {
            SetYcValue_int(rtuno, ycno + i, (BYTE *) &ycvalue, 1, utctime);
        }
    }
}

Self64 CModbus::GetCurMSTime() {
    Self64 t;

#if defined(WIN32)
    struct _timeb LongTime;
    _ftime(&LongTime);
    t = (Self64) LongTime.time * (Self64) 1000;
    t += (Self64) LongTime.millitm;
#else
    struct timeval LongTime;
    gettimeofday(&LongTime, NULL);
    t = (Self64) LongTime.tv_sec * (Self64) 1000;
    t += (Self64) (LongTime.tv_usec / 1000);
#endif
    return t;
}

// datatype = 0x00 U16 ;=0x01 S16
void CModbus::procdata_yc_word(int rtuno, int ycno, int ycnum, unsigned char *buf, char byteorder, char datatype) {
    short ycvalue_s = 0;
    int ycvalue_u = 0;
    int index = 0;
    Self64 utctime = GetCurMSTime();
    for (int i = 0; i < ycnum; i++) {
        if (byteorder) // 大端
        {
            if (datatype) // 有符号
            {
                ycvalue_s = (*(buf + index)) * 256 + (*(buf + index + 1));
                SetYcValue_short(rtuno, ycno + i, (BYTE *) &ycvalue_s, 1, utctime);

            } else {
                ycvalue_u = (*(buf + index)) * 256 + (*(buf + index + 1));
                SetYcValue_int(rtuno, ycno + i, (BYTE *) &ycvalue_u, 1, utctime);
            }
        } else {
            if (datatype) {
                ycvalue_s = (*(buf + index)) + (*(buf + index + 1)) * 256;
                SetYcValue_short(rtuno, ycno + i, (BYTE *) &ycvalue_s, 1, utctime);

            } else {
                ycvalue_u = (*(buf + index)) + (*(buf + index + 1)) * 256;
                SetYcValue_int(rtuno, ycno + i, (BYTE *) &ycvalue_u, 1, utctime);
            }
        }
        index += 2;
    }
}

void CModbus::procdata_yx(int rtuno, int yxno, int yxnum, int offset, unsigned char *buf, char byteorder) {
    unsigned short tempdata = 0;
    unsigned char yxvalue = 0x00;
    if (byteorder) {
        tempdata = (*(buf)) * 256 + (*(buf + 1));
    } else {
        tempdata = (*(buf)) + (*(buf + 1)) * 256;
    }
    Self64 utctime = GetCurMSTime();
    for (int i = 0; i < yxnum; i++) {
        yxvalue = (tempdata >> (i + offset)) & 0x01;
        SetSingleYxValue(rtuno, yxno + i, yxvalue, utctime, 0);
    }
}


int CModbus::txPro(int tdNo) {
    short rtuno = tdNo;
    if (rtuno == -1)
        return -1;

    RTUPARA *para = GetRtuPara(rtuno);
    if (para == NULL)
        return 0;
    tdNo = para->chanNo;
    if (dsd[tdNo].commFlag == 1)
        return 0;
    rtuno = GetCurrentRtuNo(tdNo);
    para = GetRtuPara(rtuno);

    rtudelay[rtuno]++;
    if (rtudelay[rtuno] > chanFreshTime) {
        RXCOMMAND *rxcommand = GetRxcommand(rtuno);
        if (rxcommand == NULL)
            return 0;
        if (rxcommand->flag == 1 && rxcommand->type == YKOPERATIONMSG && rxcommand->data.yk[1] == YKCMDMSG) {
            rxcommand->flag = 0;
            YKRETSTRUCT *ykret = GetYkRetPara(rtuno);
            ykret->rtuno = rtuno;
            ykret->ykno = rxcommand->data.yk[4] + rxcommand->data.yk[5] * 256;
            ykret->ret = 1;
            ykret->ykprocflag = 1;

        } else if (rxcommand->flag == 1 && rxcommand->type == YKOPERATIONMSG && rxcommand->data.yk[1] == YKEXEMSG) {
            usleep(400);
            // MakeYkExeFrame_goodwe(tdNo,para,rxcommand);
            MakeYkExeFrame(tdNo, para, rxcommand);
            // MakeYkExeFrame_jscn(tdNo,para,rxcommand);
            rxcommand->flag = 0;
        } else if (rxcommand->flag == 1 && rxcommand->type == YKOPERATIONMSG && rxcommand->data.yk[1] == YKDELMSG) {
            rxcommand->flag = 0;
            YKRETSTRUCT *ykret = GetYkRetPara(rtuno);
            ykret->rtuno = rtuno;
            ykret->ykno = rxcommand->data.yk[4] + rxcommand->data.yk[5] * 256;
            ykret->ret = 1;
            ykret->ykprocflag = 1;

        } else if (rxcommand->flag == 1 && (rxcommand->type == 48 || rxcommand->type == 50)) {
            MakeYTExeFrame(tdNo, para, rxcommand);
            rxcommand->flag = 0;
        } else {
            EditCallDataFrm(tdNo, para);
        }

        rtudelay[rtuno] = 0;
    }


    return 0;
}

void CModbus::flywheelControl(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    float YtValue = 0, ActValue = 0, CompenValue = 0;
    memcpy(&YtValue, rxcmd->data.buf + 7, 4);
    int value = GetYcValue(rtupara->rtuNo, 11);
    ActValue = value * 0.1;
    CompenValue = YtValue - ActValue;

    RXCOMMAND rx;
    rx.flag = 1;
    rx.type = 50;
    rx.cmdlen = 8;
    int i = 0;
    rx.data.buf[i++] = 0x00;
    rx.data.buf[i++] = 0x00;
    rx.data.buf[i++] = 0x00;
    rx.data.buf[i++] = 0x01;
    // infoaddr
    rx.data.buf[i++] = 0x00;
    rx.data.buf[i++] = 0x00;
    rx.data.buf[i++] = 0x00;
    memcpy(rx.data.buf + i, &CompenValue, 4);
    i += 4;
    rx.data.buf[i++] = 0x00;
    time_t atm = time(NULL);
    memcpy(rx.data.buf + i, &atm, sizeof(time_t));

    //    RXCOMMAND Mail ;
    //    if(GetCommandPara(0,Mail) == FALSE) return;
    RXCOMMAND *Mail = GetRxcommand(0);
    if (Mail->flag == 1)
        return;
    else
        memcpy(Mail, &rx, sizeof(RXCOMMAND));
}

void CModbus::MakeYTExeFrame_WaterLight(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int ykno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    int i = 0;
    float value = 0;
    BYTE len = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    if (ykno == 0) // 逆变输出电压设置
    {
        dsd[tdNo].shmbuf[i++] = 0x4e;
        dsd[tdNo].shmbuf[i++] = 0x86;
        memcpy(&value, rxcmd->data.buf + 7, 4);
        value = value * 10;
        dsd[tdNo].shmbuf[i++] = HIBYTE((short) (value));
        dsd[tdNo].shmbuf[i++] = LOBYTE((short) (value));
    } else // 逆变输出频率设置
    {
        dsd[tdNo].shmbuf[i++] = 0x4e;
        dsd[tdNo].shmbuf[i++] = 0x87;
        memcpy(&value, rxcmd->data.buf + 7, 4);
        value = value * 100;
        dsd[tdNo].shmbuf[i++] = HIBYTE((short) (value));
        dsd[tdNo].shmbuf[i++] = LOBYTE((short) (value));
    }
    alog_printf(ALOG_LVL_DEBUG, true, "ykno = %d,value = %.1f\n", ykno, value);
    len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYTExeFrame_goodwe_MT(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    float value = 0;
    BYTE len = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x10;
    dsd[tdNo].shmbuf[i++] = 0x01;
    dsd[tdNo].shmbuf[i++] = 0x00;
    dsd[tdNo].shmbuf[i++] = 0x00;
    dsd[tdNo].shmbuf[i++] = 0x01;
    dsd[tdNo].shmbuf[i++] = 0x02;
    memcpy(&value, rxcmd->data.buf + 6, 4);
    dsd[tdNo].shmbuf[i++] = (int) (value / 256) & 0xff;
    dsd[tdNo].shmbuf[i++] = (int) value & 0xff;
    len = 9;

    //        if(rxcmd->data.buf[4] == 0x00)//kw
    //        {

    //        }
    //        else if(rxcmd->data.buf[4] == 0x01)//kvar
    //        {
    //            dsd[tdNo].shmbuf[i++] = 0x01;
    //            dsd[tdNo].shmbuf[i++] = 0x02;
    //            dsd[tdNo].shmbuf[i++] = 0x00;
    //            dsd[tdNo].shmbuf[i++] = 0x02;
    //            dsd[tdNo].shmbuf[i++] = 0x04;
    //            memcpy(&value,rxcmd->data.buf+6,4);
    //            dsd[tdNo].shmbuf[i++] = (int)(value/(256*256*256)) & 0xff;
    //            dsd[tdNo].shmbuf[i++] = (int)(value/(256*256)) & 0xff;
    //            dsd[tdNo].shmbuf[i++] = (int)(value/256) & 0xff;
    //            dsd[tdNo].shmbuf[i++] = (int)value & 0xff;
    //            len = 11;
    //        }

    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYTExeFrame_pow(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    float value = 0;
    BYTE len = 0;
    int ytno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    if (ytno < 2) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(ytno + 48);
        dsd[tdNo].shmbuf[i++] = LOBYTE(ytno + 48);
    } else if (ytno < 4) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(ytno + 31);
        dsd[tdNo].shmbuf[i++] = LOBYTE(ytno + 31);
    } else {
        dsd[tdNo].shmbuf[i++] = HIBYTE(ytno + 31);
        dsd[tdNo].shmbuf[i++] = LOBYTE(ytno + 31);
    }

    memcpy(&value, rxcmd->data.buf + 7, 4);
    if (ytno == 4) {
        value = value * 1000;
    } else {
        value = value * 100;
    }

    dsd[tdNo].shmbuf[i++] = (int) (value) / 256 & 0xff;
    dsd[tdNo].shmbuf[i++] = (int) (value) & 0xff;
    len = 6;

    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYTExeFrame_AMC(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    float value = 0;
    BYTE len = 0;
    int ytno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    if (ytno == 0 || ytno == 1 || ytno == 2) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(60 + ytno);
        dsd[tdNo].shmbuf[i++] = LOBYTE(60 + ytno);
    } else if (ytno == 6 || ytno == 7) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(135 + ytno);
        dsd[tdNo].shmbuf[i++] = LOBYTE(135 + ytno);
    } else if (ytno == 8 || ytno == 9) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(ytno + 281);
        dsd[tdNo].shmbuf[i++] = LOBYTE(ytno + 281);
    }

    memcpy(&value, rxcmd->data.buf + 7, 4);
    dsd[tdNo].shmbuf[i++] = HIBYTE((short) value);
    dsd[tdNo].shmbuf[i++] = LOBYTE((short) value);
    len = 6;

    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYTExeFrame_PCS(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    float value = 0;
    BYTE len = 0;
    int ytno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    if (ytno == 0) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(36);
        dsd[tdNo].shmbuf[i++] = LOBYTE(36);
    } else if (ytno == 1) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(46);
        dsd[tdNo].shmbuf[i++] = LOBYTE(46);
    } else if (ytno == 2 || ytno == 3) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(ytno + 46);
        dsd[tdNo].shmbuf[i++] = LOBYTE(ytno + 46);
    } else if (ytno == 4) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(ytno + 47);
        dsd[tdNo].shmbuf[i++] = LOBYTE(ytno + 47);
    } else {
        dsd[tdNo].shmbuf[i++] = HIBYTE(ytno + 52);
        dsd[tdNo].shmbuf[i++] = LOBYTE(ytno + 52);
    }

    memcpy(&value, rxcmd->data.buf + 7, 4);
    if (ytno == 7 || ytno == 8) {
        value = value * 100;
    } else if (ytno != 0) {
        value = value * 10;
    }

    dsd[tdNo].shmbuf[i++] = HIBYTE((short) value);
    dsd[tdNo].shmbuf[i++] = LOBYTE((short) value);
    len = 6;

    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYTExeFrame_tem(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    float value = 0;
    BYTE len = 0;
    int ytno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    dsd[tdNo].shmbuf[i++] = HIBYTE(ytno + 161);
    dsd[tdNo].shmbuf[i++] = LOBYTE(ytno + 161);
    memcpy(&value, rxcmd->data.buf + 7, 4);
    dsd[tdNo].shmbuf[i++] = (int) (value / 256) & 0xff;
    dsd[tdNo].shmbuf[i++] = (int) value & 0xff;
    len = 6;

    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYTExeFrame_sen2(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    float value = 0;
    BYTE len = 0;
    int ytno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    dsd[tdNo].shmbuf[i++] = HIBYTE(ytno + 1);
    dsd[tdNo].shmbuf[i++] = LOBYTE(ytno + 1);
    memcpy(&value, rxcmd->data.buf + 7, 4);
    if (ytno == 9) {
        value = value * 1000;
    }
    dsd[tdNo].shmbuf[i++] = (int) (value / 256) & 0xff;
    dsd[tdNo].shmbuf[i++] = (int) value & 0xff;
    len = 6;

    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYTExeFrame_PLC(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    float value = 0;
    BYTE len = 0;
    int ytno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    if (ytno <= 1) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(ytno + 1025);
        dsd[tdNo].shmbuf[i++] = LOBYTE(ytno + 1025);
    } else if (ytno <= 4 && ytno >= 2) {
        dsd[tdNo].shmbuf[i++] = HIBYTE((ytno - 2) + 41739);
        dsd[tdNo].shmbuf[i++] = LOBYTE((ytno - 2) + 41739);
    } else if (ytno <= 9 && ytno >= 5) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(ytno - 5 + 58880);
        dsd[tdNo].shmbuf[i++] = LOBYTE(ytno - 5 + 58880);
    } else if (ytno == 10) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(58888);
        dsd[tdNo].shmbuf[i++] = LOBYTE(58888);
    } else if (ytno <= 19 && ytno >= 11) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(ytno - 11 + 58892);
        dsd[tdNo].shmbuf[i++] = LOBYTE(ytno - 11 + 58892);
    } else if (ytno == 20) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(58902);
        dsd[tdNo].shmbuf[i++] = LOBYTE(58902);
    }


    memcpy(&value, rxcmd->data.buf + 7, 4);
    if (ytno == 0 || ytno == 1 || ytno == 9 || ytno == 10 || ytno == 20) {
        value = value * 1;
    } else {
        value = value * 10;
    }

    dsd[tdNo].shmbuf[i++] = HIBYTE((short) value);
    dsd[tdNo].shmbuf[i++] = LOBYTE((short) value);
    len = 6;

    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYTExeFrame_BCU(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    float value = 0;
    BYTE len = 0;
    int ytno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    if (ytno == 31 || ytno == 32) // 启动在线升级
    {
        memcpy(&value, rxcmd->data.buf + 7, 4);
        if (ytno == 31)
            fileType = 1;
        else
            fileType = 17;
        fileReadNum = getBcufile(fileType, (unsigned short) value);
        if (fileReadNum == 0) {
            YKRETSTRUCT *ykret = GetYkRetPara(rtupara->rtuNo);
            RXCOMMAND Mail;
            if (GetCommandPara(rtupara->rtuNo, Mail) == FALSE)
                return;
            RXCOMMAND *rxcommand = &Mail;
            ykret->rtuno = rtupara->rtuNo;
            ykret->ykno = rxcommand->data.yk[4] + rxcommand->data.yk[5] * 256;
            ykret->ret = 0;
            ykret->ykprocflag = 1;
            fileFlag = 0;
            SetYcValue_short(rtupara->rtuNo, 1225, (BYTE *) &fileFlag, 1);
        } else {
            fileFlag = 1;
            SetYcValue_short(rtupara->rtuNo, 1225, (BYTE *) &fileFlag, 1);
            if (fileReadNum % 128 == 0)
                fileGroupNum = fileReadNum / 128;
            else
                fileGroupNum = fileReadNum / 128 + 1;
        }
    } else {
        dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
        dsd[tdNo].shmbuf[i++] = 0x06;
        if (ytno <= 5) {
            dsd[tdNo].shmbuf[i++] = HIBYTE(ytno + 1);
            dsd[tdNo].shmbuf[i++] = LOBYTE(ytno + 1);
        } else if (ytno <= 7 && ytno >= 6) {
            dsd[tdNo].shmbuf[i++] = HIBYTE((ytno - 6) * 2 + 8);
            dsd[tdNo].shmbuf[i++] = LOBYTE((ytno - 6) * 2 + 8);
        } else {
            dsd[tdNo].shmbuf[i++] = HIBYTE(ytno + 12);
            dsd[tdNo].shmbuf[i++] = LOBYTE(ytno + 12);
        }


        memcpy(&value, rxcmd->data.buf + 7, 4);
        if (ytno == 1 || ytno == 4 || ytno == 5 || (ytno <= 28 && ytno >= 17)) {
            value = value * 10;
        }

        dsd[tdNo].shmbuf[i++] = HIBYTE((short) value);
        dsd[tdNo].shmbuf[i++] = LOBYTE((short) value);
        len = 6;

        unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
        dsd[tdNo].shmbuf[len] = crc % 256;
        dsd[tdNo].shmbuf[len + 1] = crc / 256;
        for (int i = 0; i < len + 2; i++) {
            BYTE val = dsd[tdNo].shmbuf[i];
            EnterTrnQ(tdNo, val);
        }
        dsd[tdNo].commFlag = 1;
        ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
    }
}

void CModbus::MakeYTExeFrame_sen1(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    float value = 0;
    BYTE len = 0;
    int ytno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    dsd[tdNo].shmbuf[i++] = HIBYTE(ytno + 2);
    dsd[tdNo].shmbuf[i++] = LOBYTE(ytno + 2);
    memcpy(&value, rxcmd->data.buf + 7, 4);
    if (ytno == 11 || ytno == 12 || ytno == 13 || ytno == 20 || ytno == 21 || ytno == 22) {
        value = value * 10;
    } else if (ytno == 23 || ytno == 25) {
        value = value * 1000;
    }
    dsd[tdNo].shmbuf[i++] = HIBYTE((short) value);
    dsd[tdNo].shmbuf[i++] = LOBYTE((short) value);
    len = 6;

    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYTExeFrame_goodwe_HT(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    float value = 0;
    BYTE len = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x10;
    dsd[tdNo].shmbuf[i++] = 0xA2;
    dsd[tdNo].shmbuf[i++] = 0x08;
    dsd[tdNo].shmbuf[i++] = 0x00;
    dsd[tdNo].shmbuf[i++] = 0x01;
    dsd[tdNo].shmbuf[i++] = 0x02;
    memcpy(&value, rxcmd->data.buf + 6, 4);
    dsd[tdNo].shmbuf[i++] = (int) (value / 256) & 0xff;
    dsd[tdNo].shmbuf[i++] = (int) value & 0xff;
    len = 9;
    //    if(rtupara->type == 1)//100kW
    //    {
    //        if(rxcmd->data.buf[4] == 0x00)//kw
    //        {
    //            dsd[tdNo].shmbuf[i++] = 0xA2;
    //            dsd[tdNo].shmbuf[i++] = 0x08;
    //            dsd[tdNo].shmbuf[i++] = 0x00;
    //            dsd[tdNo].shmbuf[i++] = 0x01;
    //            dsd[tdNo].shmbuf[i++] = 0x02;
    //            memcpy(&value,rxcmd->data.buf+6,4);
    //            dsd[tdNo].shmbuf[i++] = (int)(value/256) & 0xff;
    //            dsd[tdNo].shmbuf[i++] = (int)value & 0xff;
    //            len = 9;
    //        }
    //        else if(rxcmd->data.buf[4] == 0x01)//kvar
    //        {
    //            dsd[tdNo].shmbuf[i++] = 0xA2;
    //            dsd[tdNo].shmbuf[i++] = 0x0A;
    //            dsd[tdNo].shmbuf[i++] = 0x00;
    //            dsd[tdNo].shmbuf[i++] = 0x02;
    //            dsd[tdNo].shmbuf[i++] = 0x04;
    //            memcpy(&value,rxcmd->data.buf+6,4);
    //            dsd[tdNo].shmbuf[i++] = (int)(value/(256*256*256)) & 0xff;
    //            dsd[tdNo].shmbuf[i++] = (int)(value/(256*256)) & 0xff;
    //            dsd[tdNo].shmbuf[i++] = (int)(value/256) & 0xff;
    //            dsd[tdNo].shmbuf[i++] = (int)value & 0xff;
    //            len = 11;
    //        }
    //    }
    //    else if(rtupara->type == 2)//36kW
    //    {
    //        if(rxcmd->data.buf[4] == 0x00)//kw
    //        {
    //            dsd[tdNo].shmbuf[i++] = 0x01;
    //            dsd[tdNo].shmbuf[i++] = 0x00;
    //            dsd[tdNo].shmbuf[i++] = 0x00;
    //            dsd[tdNo].shmbuf[i++] = 0x01;
    //            dsd[tdNo].shmbuf[i++] = 0x02;
    //            memcpy(&value,rxcmd->data.buf+6,4);
    //            dsd[tdNo].shmbuf[i++] = (int)(value/256) & 0xff;
    //            dsd[tdNo].shmbuf[i++] = (int)value & 0xff;
    //            len = 9;
    //        }
    //        else if(rxcmd->data.buf[4] == 0x01)//kvar
    //        {
    //            dsd[tdNo].shmbuf[i++] = 0x01;
    //            dsd[tdNo].shmbuf[i++] = 0x02;
    //            dsd[tdNo].shmbuf[i++] = 0x00;
    //            dsd[tdNo].shmbuf[i++] = 0x02;
    //            dsd[tdNo].shmbuf[i++] = 0x04;
    //            memcpy(&value,rxcmd->data.buf+6,4);
    //            dsd[tdNo].shmbuf[i++] = (int)(value/(256*256*256)) & 0xff;
    //            dsd[tdNo].shmbuf[i++] = (int)(value/(256*256)) & 0xff;
    //            dsd[tdNo].shmbuf[i++] = (int)(value/256) & 0xff;
    //            dsd[tdNo].shmbuf[i++] = (int)value & 0xff;
    //            len = 11;
    //        }
    //    }

    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYkExeFrame_pow(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    dsd[tdNo].shmbuf[i++] = 0x00;
    dsd[tdNo].shmbuf[i++] = 0x01;
    if (rxcmd->data.yk[6] == 0x33) {
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x00;
    } else {
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x01;
    }
    BYTE len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}


void CModbus::MakeYkExeFrame_PCS(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int ykno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    if (ykno == 0) {
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x23;
        if (rxcmd->data.yk[6] == 0x33) {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0xAA;
        } else {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x55;
        }
    } else {
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x46;
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x55;
    }

    BYTE len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYkExeFrame_PLC(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int ykno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    if (ykno == 0) {
        dsd[tdNo].shmbuf[i++] = 0x04;
        dsd[tdNo].shmbuf[i++] = 0x00;
    } else if (ykno == 1) {
        dsd[tdNo].shmbuf[i++] = 0xa3;
        dsd[tdNo].shmbuf[i++] = 0x0e;
    } else if (ykno == 2) {
        dsd[tdNo].shmbuf[i++] = 0xe6;
        dsd[tdNo].shmbuf[i++] = 0x15;
    }


    if (rxcmd->data.yk[6] == 0x33) {
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x00;
    } else {
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x01;
    }

    BYTE len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYkExeFrame_BCU(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int ykno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    if (ykno == 4 || ykno == 5) {
        if (ykno == 4) {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x0d;
        } else {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x0e;
        }
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x01;
    } else {
        if (ykno == 0) {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x07;
        } else if (ykno == 1) {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x09;
        } else if (ykno == 2) {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x0b;
        } else if (ykno == 3) {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x0c;
        } else if (ykno > 5) {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = ykno + 9;
        }

        if (rxcmd->data.yk[6] == 0x33) {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x00;
        } else {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x01;
        }
    }

    BYTE len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYkExeFrame_sen1(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    dsd[tdNo].shmbuf[i++] = 0x00;
    dsd[tdNo].shmbuf[i++] = 0x01;
    if (rxcmd->data.yk[6] == 0x33) {
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x00;
    } else {
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x01;
    }
    BYTE len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYkExeFrame_JingLang(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int ykno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    if (ykno == 0) // 开关机
    {
        dsd[tdNo].shmbuf[i++] = 0x0b;
        dsd[tdNo].shmbuf[i++] = 0xbe;
        if (rxcmd->data.yk[6] == 0x33) {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0xde;
        } else {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0xbe;
        }
    } else // 限功率开关
    {
        dsd[tdNo].shmbuf[i++] = 0x0b;
        dsd[tdNo].shmbuf[i++] = 0xfd;
        if (rxcmd->data.yk[6] == 0x33) {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x55;
        } else {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0xaa;
        }
    }
    BYTE len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYkExeFrame_goodwe(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x10;
    if (rtupara->type == 1) // 100kW
    {
        if (rxcmd->data.yk[6] == 0x33) {
            dsd[tdNo].shmbuf[i++] = 0xA1;
            dsd[tdNo].shmbuf[i++] = 0x72;
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x01;
            dsd[tdNo].shmbuf[i++] = 0x02;
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x00;
        } else {
            dsd[tdNo].shmbuf[i++] = 0xA1;
            dsd[tdNo].shmbuf[i++] = 0x72;
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x01;
            dsd[tdNo].shmbuf[i++] = 0x02;
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x01;
        }
    } else if (rtupara->type == 2) // 36kW
    {
        if (rxcmd->data.yk[6] == 0x33) {
            dsd[tdNo].shmbuf[i++] = 0x01;
            dsd[tdNo].shmbuf[i++] = 0x21;
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x01;
            dsd[tdNo].shmbuf[i++] = 0x02;
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x00;
        } else {
            dsd[tdNo].shmbuf[i++] = 0x01;
            dsd[tdNo].shmbuf[i++] = 0x20;
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x01;
            dsd[tdNo].shmbuf[i++] = 0x02;
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x00;
        }
    }
    BYTE len = 9;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYTExeFrame_jscn(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x10;
    int ytno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    if (ytno == 0) {
        dsd[tdNo].shmbuf[i++] = 0x01;
        dsd[tdNo].shmbuf[i++] = 0x01;
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x01;
        dsd[tdNo].shmbuf[i++] = 0x02;
    } else if (ytno == 1) {
        dsd[tdNo].shmbuf[i++] = 0x01;
        dsd[tdNo].shmbuf[i++] = 0x02;
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x01;
        dsd[tdNo].shmbuf[i++] = 0x02;
    } else if (ytno == 2) {
        dsd[tdNo].shmbuf[i++] = 0x01;
        dsd[tdNo].shmbuf[i++] = 0x03;
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x01;
        dsd[tdNo].shmbuf[i++] = 0x02;
    }


    if ((rxcmd->cmdlen - 3) % 3 == 0) // short
    {
        dsd[tdNo].shmbuf[i++] = rxcmd->data.buf[8];
        dsd[tdNo].shmbuf[i++] = rxcmd->data.buf[7];
    } else // float
    {
        float value = 0;
        memcpy(&value, rxcmd->data.buf + 7, 4);
        alog_printf(ALOG_LVL_DEBUG, true, "YT value is %d\n", (int) value);
        alog_printf(ALOG_LVL_DEBUG, true, "YT byte 7 is %d\n", *(rxcmd->data.buf + 7));
        alog_printf(ALOG_LVL_DEBUG, true, "YT byte 8 is %d\n", *(rxcmd->data.buf + 8));
        alog_printf(ALOG_LVL_DEBUG, true, "YT byte 9 is %d\n", *(rxcmd->data.buf + 9));
        alog_printf(ALOG_LVL_DEBUG, true, "YT byte 10 is %d\n", *(rxcmd->data.buf + 10));
        dsd[tdNo].shmbuf[i++] = HIBYTE((short) (value));
        dsd[tdNo].shmbuf[i++] = LOBYTE((short) (value));
    }

    BYTE len = 9;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYkExeFrame_jscn(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x10;
    if (rxcmd->data.yk[6] == 0x33) {
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0xe0;
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x02;
        dsd[tdNo].shmbuf[i++] = 0x04;
        dsd[tdNo].shmbuf[i++] = 0x5a;
        dsd[tdNo].shmbuf[i++] = 0x53;
        dsd[tdNo].shmbuf[i++] = 0x54;
        dsd[tdNo].shmbuf[i++] = 0x50;

    } else {
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0xdc;
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x02;
        dsd[tdNo].shmbuf[i++] = 0x04;
        dsd[tdNo].shmbuf[i++] = 0x5a;
        dsd[tdNo].shmbuf[i++] = 0x52;
        dsd[tdNo].shmbuf[i++] = 0x55;
        dsd[tdNo].shmbuf[i++] = 0x4E;
    }

    BYTE len = 11;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYkExeFrame_sun(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    if (rxcmd->data.yk[4] + rxcmd->data.yk[5] * 256 == 0) {
        dsd[tdNo].shmbuf[i++] = 0x13;
        dsd[tdNo].shmbuf[i++] = 0x8d;
        if (rxcmd->data.yk[6] == 0x33) {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0xce;
        } else {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0xcf;
        }
    } else {
        dsd[tdNo].shmbuf[i++] = 0x13;
        dsd[tdNo].shmbuf[i++] = 0x8e;
        if (rxcmd->data.yk[6] == 0x33) {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x55;
        } else {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0xAA;
        }
    }

    BYTE len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

unsigned char CModbus::FindModbusTypeByRtu(RTUPARA *rtupara) {
    unsigned char type = 0;
    for (int i = 0; i < pmod->ordernum; i++) {
        if (rtupara->type == (pmod + i)->rtuaddr && (pmod + i)->type > 19) {
            type = (pmod + i)->type;
            break;
        }
    }
    return type;
}

void CModbus::MakeYTExeFrame_JingLang(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    float value = 0;
    BYTE len = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    dsd[tdNo].shmbuf[i++] = 0x0b;
    dsd[tdNo].shmbuf[i++] = 0xeb;
    memcpy(&value, rxcmd->data.buf + 7, 4);
    value = value * 100;
    dsd[tdNo].shmbuf[i++] = HIBYTE((short) (value));
    dsd[tdNo].shmbuf[i++] = LOBYTE((short) (value));
    len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYTExeFrame_huawei(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int ykno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    int i = 0;
    float value = 0;
    BYTE len = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    if (ykno == 0) // 逆变输出功率设置(kW)
    {
        dsd[tdNo].shmbuf[i++] = 0x9C;
        dsd[tdNo].shmbuf[i++] = 0xB8;
        memcpy(&value, rxcmd->data.buf + 7, 4);
        value = value * 10;
        dsd[tdNo].shmbuf[i++] = HIBYTE((short) (value));
        dsd[tdNo].shmbuf[i++] = LOBYTE((short) (value));
    } else if (ykno == 1) // 逆变输出功率设置(%)
    {
        dsd[tdNo].shmbuf[i++] = 0x9C;
        dsd[tdNo].shmbuf[i++] = 0xBD;
        memcpy(&value, rxcmd->data.buf + 7, 4);
        value = value * 10;
        dsd[tdNo].shmbuf[i++] = HIBYTE((short) (value));
        dsd[tdNo].shmbuf[i++] = LOBYTE((short) (value));
    }
    len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYkExeFrame_huawei(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    if (rxcmd->data.yk[6] == 0x33) {
        dsd[tdNo].shmbuf[i++] = 0x9D;
        dsd[tdNo].shmbuf[i++] = 0x09;
    } else {
        dsd[tdNo].shmbuf[i++] = 0x9D;
        dsd[tdNo].shmbuf[i++] = 0x08;
    }

    dsd[tdNo].shmbuf[i++] = 0x00;
    dsd[tdNo].shmbuf[i++] = 0x01;

    BYTE len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYkExeFrame(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    unsigned char modbus_type = FindModbusTypeByRtu(rtupara);
    switch (modbus_type) {
        case 22:
            MakeYkExeFrame_goodwe_MT(tdNo, rtupara, rxcmd);
            break;
        case 33:
            MakeYkExeFrame_goodwe_HT(tdNo, rtupara, rxcmd);
            break;
        case 34:
            MakeYkExeFrame_huawei(tdNo, rtupara, rxcmd);
            break;
        case 35:
            MakeYkExeFrame_sun(tdNo, rtupara, rxcmd);
            break;
        case 37:
            MakeYkExeFrame_sen1(tdNo, rtupara, rxcmd);
            break;
        case 40:
            MakeYkExeFrame_pow(tdNo, rtupara, rxcmd);
            break;
        case 44:
            MakeYkExeFrame_JingLang(tdNo, rtupara, rxcmd);
            break;
        case 47:
            MakeYkExeFrame_BCU(tdNo, rtupara, rxcmd);
            break;
        case 49:
            MakeYkExeFrame_PCS(tdNo, rtupara, rxcmd);
            break;
        case 52:
            MakeYkExeFrame_PLC(tdNo, rtupara, rxcmd);
            break;
        default:
            break;
    }
}

void CModbus::MakeYkExeFrame_goodwe_MT(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x10;
    if (rxcmd->data.yk[6] == 0x33) {
        dsd[tdNo].shmbuf[i++] = 0x01;
        dsd[tdNo].shmbuf[i++] = 0x21;
    } else {
        dsd[tdNo].shmbuf[i++] = 0x01;
        dsd[tdNo].shmbuf[i++] = 0x20;
    }

    dsd[tdNo].shmbuf[i++] = 0x00;
    dsd[tdNo].shmbuf[i++] = 0x01;
    dsd[tdNo].shmbuf[i++] = 0x02;
    dsd[tdNo].shmbuf[i++] = 0x00;
    dsd[tdNo].shmbuf[i++] = 0x00;

    BYTE len = 9;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYkExeFrame_goodwe_HT(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x10;
    dsd[tdNo].shmbuf[i++] = 0xA1;
    dsd[tdNo].shmbuf[i++] = 0x72;
    dsd[tdNo].shmbuf[i++] = 0x00;
    dsd[tdNo].shmbuf[i++] = 0x01;
    dsd[tdNo].shmbuf[i++] = 0x02;

    if (rxcmd->data.yk[6] == 0x33) {
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x00;
    } else {
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x01;
    }

    BYTE len = 9;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYkExeFrame_old(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    if (rtupara->type == 1) {
        dsd[tdNo].shmbuf[i++] = 0x05;
        dsd[tdNo].shmbuf[i++] = rxcmd->data.yk[4];
        dsd[tdNo].shmbuf[i++] = rxcmd->data.yk[5];
        if (rxcmd->data.yk[6] == 0x33) {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x00;
        } else {
            dsd[tdNo].shmbuf[i++] = 0xff;
            dsd[tdNo].shmbuf[i++] = 0x00;
        }
    } else {
        dsd[tdNo].shmbuf[i++] = 0x06;
        if (rxcmd->data.yk[4] + rxcmd->data.yk[5] * 256 == 0) {
            dsd[tdNo].shmbuf[i++] = 0x13;
            dsd[tdNo].shmbuf[i++] = 0x8d;
            if (rxcmd->data.yk[6] == 0x33) {
                dsd[tdNo].shmbuf[i++] = 0x00;
                dsd[tdNo].shmbuf[i++] = 0xce;
            } else {
                dsd[tdNo].shmbuf[i++] = 0x00;
                dsd[tdNo].shmbuf[i++] = 0xcf;
            }
        } else {
            dsd[tdNo].shmbuf[i++] = 0x13;
            dsd[tdNo].shmbuf[i++] = 0xAA;
            if (rxcmd->data.yk[6] == 0x33) {
                dsd[tdNo].shmbuf[i++] = 0x00;
                dsd[tdNo].shmbuf[i++] = 0x55;
            } else {
                dsd[tdNo].shmbuf[i++] = 0x00;
                dsd[tdNo].shmbuf[i++] = 0xAA;
            }
        }
    }

    BYTE len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}


void CModbus::MakeYTExeFrame(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    unsigned char modbus_type = FindModbusTypeByRtu(rtupara);
    switch (modbus_type) {
        case 22:
            MakeYTExeFrame_goodwe_MT(tdNo, rtupara, rxcmd);
            break;
        case 33:
            MakeYTExeFrame_goodwe_HT(tdNo, rtupara, rxcmd);
            break;
        case 34:
            MakeYTExeFrame_huawei(tdNo, rtupara, rxcmd);
            break;
        case 35:
            MakeYTExeFrame_sun(tdNo, rtupara, rxcmd);
            break;
        case 37:
            MakeYTExeFrame_sen1(tdNo, rtupara, rxcmd);
            break;
        case 38:
            MakeYTExeFrame_sen2(tdNo, rtupara, rxcmd);
            break;
        case 39:
            MakeYTExeFrame_tem(tdNo, rtupara, rxcmd);
            break;
        case 40:
            MakeYTExeFrame_pow(tdNo, rtupara, rxcmd);
            break;
        case 44:
            MakeYTExeFrame_JingLang(tdNo, rtupara, rxcmd);
            break;
        case 47:
            MakeYTExeFrame_BCU(tdNo, rtupara, rxcmd);
            break;
        case 49:
            MakeYTExeFrame_PCS(tdNo, rtupara, rxcmd);
            break;
        case 52:
            MakeYTExeFrame_PLC(tdNo, rtupara, rxcmd);
            break;
        case 53:
            MakeYTExeFrame_AMC(tdNo, rtupara, rxcmd);
            break;
        default:
            break;
    }
}

void CModbus::MakeYTExeFrame_sun(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    int ytno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    if (ytno == 0) {
        dsd[tdNo].shmbuf[i++] = 0x13;
        dsd[tdNo].shmbuf[i++] = 0xae;
    }

    if ((rxcmd->cmdlen - 3) % 3 == 0) // short
    {
        dsd[tdNo].shmbuf[i++] = rxcmd->data.buf[8];
        dsd[tdNo].shmbuf[i++] = rxcmd->data.buf[7];
    } else // float
    {
        float value = 0;
        memcpy(&value, rxcmd->data.buf + 7, 4);
        dsd[tdNo].shmbuf[i++] = HIBYTE((short) (value * 10));
        dsd[tdNo].shmbuf[i++] = LOBYTE((short) (value * 10));
    }

    BYTE len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYTExeFrame_old(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    int ytno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    if (rtupara->type == 1) // nr
    {
        dsd[tdNo].shmbuf[i++] = 0x03;
        dsd[tdNo].shmbuf[i++] = 0xe8;
    } else // sun
    {
        if (ytno % 2 == 0) // 有功
        {
            dsd[tdNo].shmbuf[i++] = 0x13;
            dsd[tdNo].shmbuf[i++] = 0xae;
        } else // 无功
        {
            dsd[tdNo].shmbuf[i++] = 0x13;
            dsd[tdNo].shmbuf[i++] = 0xaf;
        }
    }

    if ((rxcmd->cmdlen - 3) % 3 == 0) // short
    {
        dsd[tdNo].shmbuf[i++] = rxcmd->data.buf[8];
        dsd[tdNo].shmbuf[i++] = rxcmd->data.buf[7];
    } else // float
    {
        float value = 0;
        memcpy(&value, rxcmd->data.buf + 7, 4);
        dsd[tdNo].shmbuf[i++] = HIBYTE((short) (value * 10));
        dsd[tdNo].shmbuf[i++] = LOBYTE((short) (value * 10));
    }

    BYTE len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);

    //        shmbuf[tdNo][0] = (unsigned char)(rtupara->rtuAddr);
    //        shmbuf[tdNo][1] = 0x06;
    //        unsigned short ykno = rxcmd->data.yk[4]*256+rxcmd->data.yk[5];
    //            shmbuf[tdNo][2] = rxcmd->data.yk[4];
    //            shmbuf[tdNo][3] = rxcmd->data.yk[5];
    //       shmbuf[tdNo][4] = rxcmd->data.yk[6];
    //        shmbuf[tdNo][5] = rxcmd->data.yk[7];

    //        BYTE len = 6;
    //        unsigned short crc = CRC16(shmbuf[tdNo],len);
    //        shmbuf[tdNo][len] = crc%256;
    //        shmbuf[tdNo][len+1] = crc/256;
    //        for(int i=0;i<len+2;i++)
    //        {
    //                BYTE val = shmbuf[tdNo][i];
    //                EnterTrnQ(tdNo,val);
    //        }
    //        dsd[tdNo].commFlag = 1;
    //                ShowData( tdNo, 1/*send*/, shmbuf[tdNo], len+2, 0/*data*/ );
}

void CModbus::MakeFileExeFrame_BCU(int tdNo, RTUPARA *rtupara) {
    int i = 0, len = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x15;
    dsd[tdNo].shmbuf[i++] = 0x00;
    dsd[tdNo].shmbuf[i++] = 0x06;
    dsd[tdNo].shmbuf[i++] = HIBYTE(fileType);
    dsd[tdNo].shmbuf[i++] = LOBYTE(fileType);
    dsd[tdNo].shmbuf[i++] = HIBYTE(fileGroupNo);
    dsd[tdNo].shmbuf[i++] = LOBYTE(fileGroupNo);

    if ((fileGroupNo + 1) != fileGroupNum) {
        len = MAX_FILE_RECORDNUM;
    } else {
        if ((fileReadNum - fileGroupNo * MAX_FILE_RECORDNUM * 2) % 2 == 0)
            len = (fileReadNum - fileGroupNo * MAX_FILE_RECORDNUM * 2) / 2;
        else
            len = (fileReadNum - fileGroupNo * MAX_FILE_RECORDNUM * 2) / 2 + 1;
    }
    dsd[tdNo].shmbuf[i++] = HIBYTE(len);
    dsd[tdNo].shmbuf[i++] = LOBYTE(len);
    dsd[tdNo].shmbuf[2] = len * 2 + 9;
    for (int j = 0; j < len * 2; j++) {
        dsd[tdNo].shmbuf[i++] = filebuf[fileReadNum - fileGroupNo * MAX_FILE_RECORDNUM * 2 + j];
    }

    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len * 2 + 9);
    dsd[tdNo].shmbuf[i++] = crc % 256;
    dsd[tdNo].shmbuf[i++] = crc / 256;
    for (int k = 0; k < len * 2 + 9 + 2; k++) {
        BYTE val = dsd[tdNo].shmbuf[k];
        EnterTrnQ(tdNo, val);
    }
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len * 2 + 9 + 2, 0 /*data*/);
    dsd[tdNo].commFlag = 1;

    float rate = (((float) fileGroupNo + 1) / ((float) fileGroupNum)) * 100;
    SetYcValue_float(rtupara->rtuNo, 1226, (BYTE *) (&rate), 1);

    fileGroupNo++;
    if (fileGroupNo == fileGroupNum) {
        fileFlag = 0;
        SetYcValue_short(rtupara->rtuNo, 1225, (BYTE *) &fileFlag, 1);
        free(filebuf);
        filebuf = NULL;
        fileFlag = 0;
        fileType = 0;
        fileGroupNo = 0;
        fileGroupNum = 0;
        fileReadNum = 0;
    }
}

void CModbus::MakeSetTimeFrame(int tdNo, RTUPARA *rtupara, WORD Addr) {
    dsd[tdNo].shmbuf[0] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[1] = 0x10;
    dsd[tdNo].shmbuf[2] = Addr / 256;
    dsd[tdNo].shmbuf[3] = Addr % 256;
    dsd[tdNo].shmbuf[4] = 0;
    dsd[tdNo].shmbuf[5] = 6;
    dsd[tdNo].shmbuf[6] = 12;
    FETIME st;
    getTime(&st);
    dsd[tdNo].shmbuf[7] = (st.aYear + 1900) / 256;
    dsd[tdNo].shmbuf[8] = (st.aYear + 1900) % 256;
    dsd[tdNo].shmbuf[9] = st.aMon / 256;
    dsd[tdNo].shmbuf[10] = st.aMon % 256;
    dsd[tdNo].shmbuf[11] = st.aDay / 256;
    dsd[tdNo].shmbuf[12] = st.aDay % 256;
    dsd[tdNo].shmbuf[13] = st.aHour / 256;
    dsd[tdNo].shmbuf[14] = st.aHour % 256;
    dsd[tdNo].shmbuf[15] = st.aMin / 256;
    dsd[tdNo].shmbuf[16] = st.aMin % 256;
    dsd[tdNo].shmbuf[17] = st.aSec / 256;
    dsd[tdNo].shmbuf[18] = st.aSec % 256;


    BYTE len = 19;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
    dsd[tdNo].commFlag = 1;
}
// void CModbus::MakeSetWriteFrame(int tdNo,RTUPARA *rtupara,WORD Addr)
//{
//         shmbuf[tdNo][0] = (unsigned char)(rtupara->rtuAddr);
//	shmbuf[tdNo][1] = 0x10;
//	shmbuf[tdNo][2] = Addr/256;
//	shmbuf[tdNo][3] = Addr%256;
//	shmbuf[tdNo][4] = 0;
//	shmbuf[tdNo][5] = 1;
//	shmbuf[tdNo][6] = 2;
//	shmbuf[tdNo][7] = 0;
//	shmbuf[tdNo][8] = 2;

//	BYTE len = 9;
//		unsigned short crc = CRC16(shmbuf[tdNo],len);
//		shmbuf[tdNo][len] = crc%256;
//		shmbuf[tdNo][len+1] = crc/256;
//		for(int i=0;i<len+2;i++)
//		{
//			BYTE val = shmbuf[tdNo][i];
//			EnterTrnQ(tdNo,val);
//		}
////		ShowData( tdNo, 1/*send*/, shmbuf[tdNo], len+2, 0/*data*/ );
//}
void CModbus::EditCallDataFrm(int tdNo, RTUPARA *rtupara) {
    int modbusrcnt = 0;
    if (pmod)
        modbusrcnt = pmod->ordernum;
    else
        return;
    if (rtudata[rtupara->rtuNo].txPtr >= modbusrcnt) {
        rtudata[rtupara->rtuNo].txPtr = 0;
        // chanloop[tdNo]++;
        rtuloop[rtupara->rtuNo]++;
    }
    int i;
    for (i = rtudata[rtupara->rtuNo].txPtr; i < modbusrcnt; i++) {
        MODBUS *modbus = pmod + i;
        if ((modbus->rtuaddr != rtupara->type) || (rtuloop[rtupara->rtuNo] % modbus->freshtime) != 0)
            continue;
        if (rtupara->rtuNo == 1 && fileFlag == 1) // bcu通道开始升级程序，采集停止召唤
        {
            MakeFileExeFrame_BCU(tdNo, rtupara);
        } else {
            dsd[tdNo].shmbuf[0] = rtupara->rtuAddr;
            dsd[tdNo].shmbuf[1] = modbus->order;
            dsd[tdNo].shmbuf[2] = modbus->startaddr / 256;
            dsd[tdNo].shmbuf[3] = modbus->startaddr % 256;
            dsd[tdNo].shmbuf[4] = modbus->length / 256;
            dsd[tdNo].shmbuf[5] = modbus->length % 256;
            int len = 6;
            unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
            dsd[tdNo].shmbuf[len] = crc % 256;
            dsd[tdNo].shmbuf[len + 1] = crc / 256;
            for (int j = 0; j < len + 2; j++) {
                BYTE val = dsd[tdNo].shmbuf[j];
                EnterTrnQ(tdNo, val);
            }
            dsd[tdNo].txCmd = modbus->type;
            dsd[tdNo].commFlag = 1;
            ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
        }
        rtudata[rtupara->rtuNo].txPtr = i;
        break;
    }
    if (i >= modbusrcnt) {
        rtudata[rtupara->rtuNo].txPtr = 0;
        // chanloop[tdNo]++;
        rtuloop[rtupara->rtuNo]++;
        ChangeRtuNo(tdNo);
    }
}

/*
int CModbus::procdata_20(int rtuno, int pModeNo,unsigned char cmdtype,int datalen,unsigned char *buf)
{
    if(pModeNo < 0) return 0;

    short startAddr = (pmod+rtudata[rtuno].txPtr)->startaddr;
    short endAddr = startAddr + MIN(datalen/2,(pmod+rtudata[rtuno].txPtr)->length);
    int i,j,k,ycNum = 0,yxNum = 0,kwhNum = 0;
    for(i=0;i<(pmode+pModeNo)->procNum;i++)
    {
        if((pmode+pModeNo)->modbusproc[i].order == cmdtype)
        {
            if((pmode+pModeNo)->modbusproc[i].regAddr < endAddr)
            {
                if((pmode+pModeNo)->modbusproc[i].regAddr >= startAddr)
                {
                    WORD tempword = 0;
                    unsigned char tempchar = 0x00;
                    if((pmode+pModeNo)->modbusproc[i].dataType == 0x00)//Bit
                    {
                        k = ((pmode+pModeNo)->modbusproc[i].regAddr-startAddr)*2;
                        for(j=0;j<(pmode+pModeNo)->modbusproc[i].bitNum;j++)
                        {
                            if(j%16==0)
                            {
                                if((pmode+pModeNo)->modbusproc[i].byteOrder == 0x00)
                                    tempword = (*(buf + k))*256 + *(buf + k + 1);
                                else
                                    tempword = *(buf + k) + (*(buf + k + 1))*256;
                                k+=2;
                            }
                            tempchar = tempword>>((pmode+pModeNo)->modbusproc[i].bitOffset+j%16) & 0x01;
                            SetSingleYxValue(rtuno,yxNum+j,tempchar,0);

                        }
                        yxNum+=(pmode+pModeNo)->modbusproc[i].bitNum;
                    }
                    else if((pmode+pModeNo)->modbusproc[i].dataType == 0x01)//Bits
                    {
                        k = ((pmode+pModeNo)->modbusproc[i].regAddr-startAddr)*2;
                        if((pmode+pModeNo)->modbusproc[i].byteOrder == 0x00)
                            tempword = (*(buf + k))*256 + *(buf + k + 1);
                        else
                            tempword = *(buf + k) + (*(buf + k + 1))*256;
                        tempword = tempword>>((pmode+pModeNo)->modbusproc[i].bitOffset) &
((0x01<<((pmode+pModeNo)->modbusproc[i].bitNum)) - 1); SetYcValue_short(rtuno,ycNum,(BYTE *)&tempword,1); ycNum+=1;
                    }
                    else if((pmode+pModeNo)->modbusproc[i].dataType == 0x02)//UINT16
                    {
                        k = ((pmode+pModeNo)->modbusproc[i].regAddr-startAddr)*2;
                        for(j=0;j<(pmode+pModeNo)->modbusproc[i].regNum;j++)
                        {
                            int ycval = 0;
                            if((pmode+pModeNo)->modbusproc[i].byteOrder)//小端
                            {
                                ycval = *(buf+k) + *(buf+k+1)*256;
                            }
                            else//大端
                            {
                                ycval = *(buf+k)*256 + *(buf+k+1);
                            }
                            SetYcValue_int(rtuno,ycNum,(BYTE *)&ycval,1);
                            k+=2;
                            ycNum++;
                        }
                    }
                    else if((pmode+pModeNo)->modbusproc[i].dataType == 0x03)//SINT16
                    {
                        k = ((pmode+pModeNo)->modbusproc[i].regAddr-startAddr)*2;
                        for(j=0;j<(pmode+pModeNo)->modbusproc[i].regNum;j++)
                        {
                            short ycval = 0;
                            if((pmode+pModeNo)->modbusproc[i].byteOrder)//小端
                            {
                                ycval = *(buf+k) + *(buf+k+1)*256;
                            }
                            else//大端
                            {
                                ycval = *(buf+k)*256 + *(buf+k+1);
                            }
                            SetYcValue_short(rtuno,ycNum,(BYTE *)&ycval,1);
                            k+=2;
                            ycNum++;
                        }
                    }
                    else if((pmode+pModeNo)->modbusproc[i].dataType == 0x04 || (pmode+pModeNo)->modbusproc[i].dataType
== 0x05)//UINT32 SINT32
                    {
                        k = ((pmode+pModeNo)->modbusproc[i].regAddr-startAddr)*2;
                        for(j=0;j<(pmode+pModeNo)->modbusproc[i].regNum/2;j++)
                        {
                            int ycval = 0;
                            if((pmode+pModeNo)->modbusproc[i].wordOrder)//小端
                            {
                                if((pmode+pModeNo)->modbusproc[i].byteOrder)//小端 0123
                                {
                                    ycval = *(buf+k) + *(buf+k+1)*256 + *(buf+k+2)*256*256 + *(buf+k+3)*256*256*256;
                                }
                                else//大端 1032
                                {
                                    ycval = *(buf+k)*256 + *(buf+k+1) + *(buf+k+2)*256*256*256 + *(buf+k+3)*256*256;
                                }
                            }
                            else
                            {
                                if((pmode+pModeNo)->modbusproc[i].byteOrder)//小端 2301
                                {
                                    ycval = *(buf+k)*256*256 + *(buf+k+1)*256*256*256 + *(buf+k+2) + *(buf+k+3)*256;
                                }
                                else//大端3210
                                {
                                    ycval = *(buf+k)*256*256*256 + *(buf+k+1)*256*256 + *(buf+k+2)*256 + *(buf+k+3);
                                }
                            }
                            SetYcValue_int(rtuno,ycNum,(BYTE *)&ycval,1);
                            k+=4;
                            ycNum++;
                        }
                    }
                    else if((pmode+pModeNo)->modbusproc[i].dataType == 0x06)//FLOAT
                    {
                        k = ((pmode+pModeNo)->modbusproc[i].regAddr-startAddr)*2;
                        for(j=0;j<(pmode+pModeNo)->modbusproc[i].regNum/2;j++)
                        {
                            float ycval_f = 0;int ycval = 0;
                            if((pmode+pModeNo)->modbusproc[i].wordOrder)//小端
                            {
                                if((pmode+pModeNo)->modbusproc[i].byteOrder)//小端 0123
                                {
                                    ycval = *(buf+k) + *(buf+k+1)*256 + *(buf+k+2)*256*256 + *(buf+k+3)*256*256*256;
                                }
                                else//大端 1032
                                {
                                    ycval = *(buf+k)*256 + *(buf+k+1) + *(buf+k+2)*256*256*256 + *(buf+k+3)*256*256;
                                }
                            }
                            else
                            {
                                if((pmode+pModeNo)->modbusproc[i].byteOrder)//小端 2301
                                {
                                    ycval = *(buf+k)*256*256 + *(buf+k+1)*256*256*256 + *(buf+k+2) + *(buf+k+3)*256;
                                }
                                else//大端3210
                                {
                                    ycval = *(buf+k)*256*256*256 + *(buf+k+1)*256*256 + *(buf+k+2)*256 + *(buf+k+3);
                                }
                            }
                            ycval_f = *((float *)&ycval);
                            SetYcValue_float(rtuno,ycNum,(BYTE *)&ycval_f,1);
                            k+=4;
                            ycNum++;
                        }
                    }
                    else if((pmode+pModeNo)->modbusproc[i].dataType == 0x07)//kwh
                    {
                        k = ((pmode+pModeNo)->modbusproc[i].regAddr-startAddr)*2;
                        for(j=0;j<(pmode+pModeNo)->modbusproc[i].regNum/2;j++)
                        {
                            unsigned int kwhval = 0;
                            if((pmode+pModeNo)->modbusproc[i].wordOrder)//小端
                            {
                                if((pmode+pModeNo)->modbusproc[i].byteOrder)//小端 0123
                                {
                                    kwhval = *(buf+k) + *(buf+k+1)*256 + *(buf+k+2)*256*256 + *(buf+k+3)*256*256*256;
                                }
                                else//大端 1032
                                {
                                    kwhval = *(buf+k)*256 + *(buf+k+1) + *(buf+k+2)*256*256*256 + *(buf+k+3)*256*256;
                                }
                            }
                            else
                            {
                                if((pmode+pModeNo)->modbusproc[i].byteOrder)//小端 2301
                                {
                                    kwhval = *(buf+k)*256*256 + *(buf+k+1)*256*256*256 + *(buf+k+2) + *(buf+k+3)*256;
                                }
                                else//大端3210
                                {
                                    kwhval = *(buf+k)*256*256*256 + *(buf+k+1)*256*256 + *(buf+k+2)*256 + *(buf+k+3);
                                }
                            }
                            SetKwhValue(rtuno,kwhNum,kwhval);
                            k+=4;
                            kwhNum++;
                        }
                    }
                }
                else
                {
                    if((pmode+pModeNo)->modbusproc[i].dataType == 0x00)
                    {
                        yxNum += (pmode+pModeNo)->modbusproc[i].bitNum;
                    }
                    else if((pmode+pModeNo)->modbusproc[i].dataType == 0x01 || (pmode+pModeNo)->modbusproc[i].dataType
== 0x02 || (pmode+pModeNo)->modbusproc[i].dataType == 0x03)
                    {
                        ycNum +=(pmode+pModeNo)->modbusproc[i].regNum;
                    }
                    else if((pmode+pModeNo)->modbusproc[i].dataType == 0x04 || (pmode+pModeNo)->modbusproc[i].dataType
== 0x05 || (pmode+pModeNo)->modbusproc[i].dataType == 0x06)
                    {
                        ycNum +=(pmode+pModeNo)->modbusproc[i].regNum/2;
                    }
                    else
                    {
                        kwhNum +=(pmode+pModeNo)->modbusproc[i].regNum/2;
                    }
                }
            }
            else
            {
                break;
            }
        }
        else
        {
            if((pmode+pModeNo)->modbusproc[i].dataType == 0x00)
            {
                yxNum += (pmode+pModeNo)->modbusproc[i].bitNum;
            }
            else if((pmode+pModeNo)->modbusproc[i].dataType == 0x01 || (pmode+pModeNo)->modbusproc[i].dataType == 0x02
|| (pmode+pModeNo)->modbusproc[i].dataType == 0x03)
            {
                ycNum +=(pmode+pModeNo)->modbusproc[i].regNum;
            }
            else if((pmode+pModeNo)->modbusproc[i].dataType == 0x04 || (pmode+pModeNo)->modbusproc[i].dataType == 0x05
|| (pmode+pModeNo)->modbusproc[i].dataType == 0x06)
            {
                ycNum +=(pmode+pModeNo)->modbusproc[i].regNum/2;
            }
            else
            {
                kwhNum +=(pmode+pModeNo)->modbusproc[i].regNum/2;
            }
        }

    }
    return 1;
}
*/

unsigned short CModbus::CRC16(unsigned char *Array, unsigned int Len) {
    unsigned short IX, IY, CRC;

    CRC = 0xFFFF;
    if (Len == 0)
        return CRC;
    unsigned char Temp;
    Len--;
    for (IX = 0; IX <= Len; IX++) {
        Temp = LOBYTE(CRC) ^ Array[IX];
        CRC = (HIBYTE(CRC) * 256) + Temp;
        for (IY = 0; IY <= 7; IY++)
            if ((CRC & 1) != 0)
                CRC = (CRC >> 1) ^ 0xA001;
            else
                CRC = CRC >> 1;
    };
    return CRC;
}


int CModbus::getoffset(int cur_id, int type, int rtutype) {
    /*
0://遥信0..7-8..15
1://遥测四字节整数(低字前1032)
2://遥信8..15-0..7
3://二字节有符号遥测帧(10)
4://二字节有符号遥测帧(10)高位符号其余绝对值
5://二字节无符号遥测帧(10)
7://遥测四字节整数(高字前3210)反序
8://遥测四字节浮点(高字前3210)反序
9://遥测四字节整数(低字前0123)正序
10://遥测四字节浮点(低字前0123)正序
11://电度帧整数(0123正序)
12://电度帧整数(3210反序)
13://电度帧整数(低字前1032)
14://电度帧浮点(0123正序)
15://电度帧浮点(3210反序)
16://电度帧浮点(1032低字前)
23://二字节有符号遥测帧(01)
24://二字节有符号遥测帧(01)高位符号其余绝对值
25://二字节无符号遥测帧(01)
*/

    MODBUS *modbus = pmod;
    int num = 0;
    int i;

    for (int i = 0; i < cur_id; i++, modbus++) {
        if (rtutype != modbus->rtuaddr)
            continue;
        switch (modbus->type) {

            case 0: // 遥信0..7-8..15
            case 2: // 遥信8..15-0..7
                if (type == 0)
                    num += modbus->length;
                continue;
                break;
            case 1: // 遥测四字节整数(低字前1032)
            case 7: // 遥测四字节整数(高字前3210)反序
            case 8: // 遥测四字节浮点(高字前3210)反序
            case 9: // 遥测四字节整数(低字前0123)正序
            case 10: // 遥测四字节浮点(低字前0123)正序
                if (type == 1)
                    num += modbus->length / 2;
                break;
            case 3: // 二字节有符号遥测帧(10)
            case 4: // 二字节有符号遥测帧(10)高位符号其余绝对值
            case 5: // 二字节无符号遥测帧(10)
            case 23: // 二字节有符号遥测帧(01)
            case 24: // 二字节有符号遥测帧(01)高位符号其余绝对值
            case 25: // 二字节无符号遥测帧(01)
                if (type == 1)
                    num += modbus->length;
                break;
            case 11: // 电度帧整数(0123正序)
            case 12: // 电度帧整数(3210反序)
            case 13: // 电度帧整数(低字前1032)
            case 14: // 电度帧浮点(0123正序)
            case 15: // 电度帧浮点(3210反序)
            case 16: // 电度帧浮点(1032低字前)
                if (type == 2)
                    num += modbus->length / 2;
                break;
        }
    }
    return num;
}
#include <string.h>
#include "test.h"

#include "alog_printf.h"


// 北京欢迎您
CModbus modbus;
int chanFreshTime = 300;
void getNameByPid(int pid, char *task_name) {
    char proc_pid_path[256];
    char buf[256];

    sprintf(proc_pid_path, "/proc/%d/status", pid);
    FILE *fp = fopen(proc_pid_path, "r");
    if (NULL != fp) {
        if (fgets(buf, 255, fp) == NULL) {
            fclose(fp);
        }
        fclose(fp);
        sscanf(buf, "%*s %s", task_name);
    }
}

int getProcNo(char *m_ProcName) {
    int ret = 0;
    for (int i = 0; i < 20; i++) {
        if (m_ProcName[i] >= '0' && m_ProcName[i] <= '9') {
            ret = (int) (m_ProcName[i] - 0x30);
            break;
        }
    }
    return ret;
}

#ifdef WIN32
UINT WINAPI SetBaseAddr(HWND Addr) {
    modbus.pid = Addr;
    return 0;
}
UINT WINAPI RxPro(int tdNo, void *fertdata) { return (modbus.rxPro(tdNo, (FERTDATA *) fertdata)); }
UINT WINAPI TxPro(int tdNo, void *fertdata) { return (modbus.txPro(tdNo, (FERTDATA *) fertdata)); }
#else
extern "C" {
int SetBaseAddr(int Addr, void *fertdata) {

    char m_ProcName[20];
    getNameByPid(Addr, m_ProcName);
    modbus.m_ProcNo = getProcNo(m_ProcName);
    modbus.getModbusConfig();
    modbus.pid = Addr;
    modbus.SetBaseAddr((FERTDATA *) fertdata);
    // modbus.InitRTUaddr();
    return 0;
}

int RxPro(int tdNo, void *fertdata) {
    modbus.SetBaseAddr((FERTDATA *) fertdata);
    return (modbus.rxPro(tdNo));
}

int TxPro(int tdNo, void *fertdata) {
    modbus.SetBaseAddr((FERTDATA *) fertdata);
    return (modbus.txPro(tdNo));
}
}
#endif

int CModbus::rxPro(int tdNo) {
    short rtuno = tdNo;
    RTUPARA *rtupara = GetRtuPara(rtuno);
    tdNo = rtupara->chanNo;
    if (dsd[tdNo].commFlag == 0)
        return 0;
    rtuno = GetCurrentRtuNo(tdNo);
    int ret = procdata(tdNo, rtuno, &dsd[tdNo]);
    if (ret == 1)
        CallscirxOvertime(&dsd[tdNo], tdNo, rtuno);
    else {
        dsd[tdNo].rxOvertime = 0;
        usleep(40); // 珠海沃顿PCS反应不及时
        dsd[tdNo].commFlag = 0;
        dsd[tdNo].txCmd = 0;
        ClearRtuRecFailnum(rtuno);
        ClearRec(tdNo);
        ChangeRtuNo(tdNo);
    }

    return 0;
}

int CModbus::getBcufile(int filetype, unsigned short verno) {
    char filename[100];
    char *m_pHomeDir = getenv("RUNDIR");
    if (m_pHomeDir == NULL)
        return 0;
    if (filetype == 1) // bcu file
    {
        sprintf(filename, "%s/bin/BCU_V%d.%d.%d.bin", m_pHomeDir, (verno >> 12) & 0x0f, (verno >> 8) & 0x0f,
                verno & 0xff);
    } else // bmu file
    {
        sprintf(filename, "%s/bin/BMU_V%d.%d.%d.bin", m_pHomeDir, (verno >> 12) & 0x0f, (verno >> 8) & 0x0f,
                verno & 0xff);
    }
    int file = open(filename, 0, O_RDWR);
    if (file < 0) {
        printf("PROTOCOL Err:File %s not exist\n", filename);
        return 0;
    }
    filebuf = (unsigned char *) malloc(MAX_FILE_BUF);
    int num = read(file, filebuf, MAX_FILE_BUF);
    close(file);
    if (num <= 0) {
        alog_printf(ALOG_LVL_INFO, true, "BCU升级文件%s读取失败\n", filename); // 参数文件读取失败
        return 0;
    }
    if (num != ((*(filebuf + 4)) * 256 * 256 * 256 + (*(filebuf + 5)) * 256 * 256 + (*(filebuf + 6)) * 256 +
                (*(filebuf + 7)))) {
        alog_printf(ALOG_LVL_INFO, true, "BCU升级文件%s校验失败1\n", filename); // 参数文件校验失败
        return 0;
    }
    // 需要校验版本号
    return num;
}

void CModbus::getModbusConfig() {
    char filename[100];
    char *m_pHomeDir = getenv("RUNDIR");
    if (m_pHomeDir == NULL)
        return;
    // FILE	*fp;
    sprintf(filename, "%s/dat/ModbusCall.dat", m_pHomeDir);
    int file = open(filename, 0, O_RDWR);
    if (file < 0) {
        printf("PROTOCOL Err:File %s not exist\n", filename);
        return;
    }
    int num = read(file, pmod, sizeof(MODBUS) * RTU_MAX_NUM);
    close(file);
    if (num > 0)
        printf("modbusCall参数配置文件%s读取成功\n", filename); // 参数文件读取成功
}

void CModbus::CallscirxOvertime(MODBUSDATA *sci, int tdNo, short rtuno) {
    CHANPARA *chanp = GetChanPara(tdNo);
    if (chanp == NULL)
        return;
    if (sci->rxOvertime > 6000) {
        // alog_printf(ALOG_LVL_DEBUG,true,"接收超时\n");
        _TRACE(tdNo, "接收超时", 1);
        sci->commFlag = 0;
        sci->rxOvertime = 0;
        ClearRec(tdNo);
        rtudata[rtuno].txPtr++;
        AddRtuRecFailnum(rtuno, 1);
        /// ChangeRtuNo(tdNo);
    } else {
        sci->rxOvertime++;
    }
}
int CModbus::procdata(int tdNo, int rtuno, MODBUSDATA *modbusdata) {
    short len = 0, i;
    LenRecQ(tdNo, &len);

    if (rtuno == -1)
        return 1;
    RTUPARA *rtupara = GetRtuPara(rtuno);
    int modbusrcnt = 0;
    // MODBUS *mp=GetModbusPara();
    if (pmod)
        modbusrcnt = pmod->ordernum;
    else
        return 1;
    while (len >= 3) {

        for (i = 0; i < 3; i++)
            GetRecVal(tdNo, &dsd[tdNo].shmbuf[i]);
        BYTE rturaddr = dsd[tdNo].shmbuf[0];
        BYTE CmdType = dsd[tdNo].shmbuf[1];
        BYTE datalen = dsd[tdNo].shmbuf[2];
        if (rturaddr != rtupara->rtuAddr) {
            for (i = 0; i < 3; i++)
                BackRecVal(tdNo);
            return 1;
        }

        if (CmdType > 4 && CmdType < 17)
            datalen = 3;

        if ((datalen + 5) > len) {
            for (i = 0; i < 3; i++)
                BackRecVal(tdNo);
            return 1;
        }
        for (i = 0; i < datalen + 2; i++)
            GetRecVal(tdNo, dsd[tdNo].shmbuf + 3 + i);

        unsigned short crc = CRC16(dsd[tdNo].shmbuf, datalen + 3);
        if (crc == (dsd[tdNo].shmbuf[datalen + 3] + dsd[tdNo].shmbuf[datalen + 4] * 256)) {
            ShowData(tdNo, 0 /*recv*/, dsd[tdNo].shmbuf, datalen + 5, 0 /*data*/);
            len -= datalen + 5;
        } else {
            len -= datalen + 5;
            continue;
        }

        int i;
        int num = 0, ycnum;
        Self64 utctime = GetCurMSTime();
        switch (CmdType) {
            case 0x01:
            case 0x02: {
                int j;
                unsigned char yxvalue = 0x00;
                num = getoffset(rtudata[rtuno].txPtr, 0, rtupara->type);

                for (j = 0; j < (pmod + rtudata[rtuno].txPtr)->length; j++) {
                    yxvalue = dsd[tdNo].shmbuf[3 + j / 8] >> (j % 8) & 0x01;
                    SetSingleYxValue((unsigned short) rtuno, num + j, yxvalue, utctime, 0);
                    // alog_printf(ALOG_LVL_DEBUG,true,"yxstartnum = %d,yxno = %d,yxvalue = %d,shutbuf =
                    // %d\n",num,num+j,yxvalue,dsd[tdNo].shmbuf[3+j/8]);
                }
                //            int pModeNo = -1;
                //            for(i=0;i<8;i++)
                //            {
                //                if((pmode+i)->dataModeNo != rtupara->type) continue;
                //                else
                //                {
                //                    pModeNo = i;
                //                    break;
                //                }
                //            }
                //            short startAddr = (pmod+rtudata[rtuno].txPtr)->startaddr;
                //            short endAddr = startAddr + MIN(datalen,(pmod+rtudata[rtuno].txPtr)->length);
                //            int i,j;unsigned char yxvalue = 0x00;
                //            for(i=0;i<(pmode+pModeNo)->procNum;i++)
                //            {
                //                if((pmode+pModeNo)->modbusproc[i].regAddr < endAddr)
                //                {
                //                    if((pmode+pModeNo)->modbusproc[i].regAddr >= startAddr)
                //                    {
                //                        for(j=0;j<(pmod+rtudata[rtuno].txPtr)->length;j++)
                //                        {
                //                            yxvalue = dsd[tdNo].shmbuf[3+j/8]>>(j%8) & 0x01;
                //                            SetSingleYxValue((unsigned short)rtuno,num+j,yxvalue,0);
                //                        }
                //                    }
                //                    else
                //                    {
                //                        num += (pmode+pModeNo)->modbusproc[i].regNum;
                //                    }
                //                }
                //            }
                rtudata[rtuno].txPtr++;
                if (rtudata[rtupara->rtuNo].txPtr >= modbusrcnt) {
                    rtudata[rtupara->rtuNo].txPtr = 0;
                    // chanloop[tdNo]++;
                    rtuloop[rtuno]++;
                }
                return 0;
            } break;
            case 0x04:
            case 0x03: {
                switch (dsd[tdNo].txCmd) {
                    case 0: // 遥信0..7-8..15
                        num = getoffset(rtudata[rtuno].txPtr, 0, rtupara->type);
                        SetSingleYxValue(rtuno, num, dsd[tdNo].shmbuf[3], utctime, 0);
                        break;
                    case 2: // 遥信8..15-0..7
                        num = getoffset(rtudata[rtuno].txPtr, 0, rtupara->type);
                        SetSingleYxValue(rtuno, num, dsd[tdNo].shmbuf[4], utctime, 0);
                        break;
                    case 1: // 遥测四字节整数(低字前1032)
                        ycnum = rtupara->ycnum;
                        num = getoffset(rtudata[rtuno].txPtr, 1, rtupara->type);
                        for (i = 0; i < MIN(datalen / 4, ycnum - num); i++) {
                            int ycval = 0;
                            // 低字在前，高字在后
                            ycval = (dsd[tdNo].shmbuf[i * 4 + 3] * 256 + dsd[tdNo].shmbuf[i * 4 + 4]) +
                                    (dsd[tdNo].shmbuf[i * 4 + 5] * 256 + dsd[tdNo].shmbuf[i * 4 + 6]) * 65536;
                            SetYcValue_int(rtuno, num + i, (BYTE *) &ycval, 1, utctime);
                        };
                        break;
                    case 3: // 二字节有符号遥测帧(10)
                        ycnum = rtupara->ycnum;
                        num = getoffset(rtudata[rtuno].txPtr, 1, rtupara->type);
                        for (i = 0; i < MIN(datalen / 2, ycnum - num); i++) {
                            short ycval = dsd[tdNo].shmbuf[i * 2 + 3] * 256 + dsd[tdNo].shmbuf[i * 2 + 4];
                            SetYcValue_short(rtuno, num + i, (BYTE *) &ycval, 1, utctime);
                        }
                        break;

                    case 4: // 二字节有符号遥测帧(10)高位符号其余绝对值
                        ycnum = rtupara->ycnum;
                        num = getoffset(rtudata[rtuno].txPtr, 1, rtupara->type);
                        for (i = 0; i < MIN(datalen / 2, ycnum - num); i++) {
                            short ycval;
                            int v = dsd[tdNo].shmbuf[i * 2 + 3] * 256 + dsd[tdNo].shmbuf[i * 2 + 4];
                            if (v & 0x8000)
                                ycval = (-1) * (v & 0x7fff);
                            else
                                ycval = v;
                            SetYcValue_short(rtuno, num + i, (BYTE *) &ycval, 1, utctime);
                        }
                        break;
                    case 5: // 二字节无符号遥测帧(10)
                        ycnum = rtupara->ycnum;
                        num = getoffset(rtudata[rtuno].txPtr, 1, rtupara->type);
                        for (i = 0; i < MIN(datalen / 2, ycnum - num); i++) {
                            int ycval = dsd[tdNo].shmbuf[i * 2 + 3] * 256 + dsd[tdNo].shmbuf[i * 2 + 4];
                            SetYcValue_int(rtuno, num + i, (BYTE *) &ycval, 1, utctime);
                        }
                        break;
                    case 6: // 遥测四字节整数(高字前3210)反序
                        ycnum = rtupara->ycnum;
                        num = getoffset(rtudata[rtuno].txPtr, 1, rtupara->type);
                        for (i = 0; i < MIN(datalen / 4, ycnum - num); i++) {
                            int ycval = (dsd[tdNo].shmbuf[i * 4 + 3] * 256 + dsd[tdNo].shmbuf[i * 4 + 4]) * 65536 +
                                        (dsd[tdNo].shmbuf[i * 4 + 5] * 256 + dsd[tdNo].shmbuf[i * 4 + 6]);
                            SetYcValue_int(rtuno, num + i, (BYTE *) &ycval, 1, utctime);
                        }
                        break;

                    case 7: // 遥测四字节浮点(高字前2301)反序
                        ycnum = rtupara->ycnum;
                        num = getoffset(rtudata[rtuno].txPtr, 1, rtupara->type);
                        for (i = 0; i < MIN(datalen / 4, ycnum - num); i++) {
                            int ycval =
                                    (dsd[tdNo].shmbuf[3 + i * 4] * 256 + dsd[tdNo].shmbuf[3 + i * 4 + 1]) +
                                    (dsd[tdNo].shmbuf[3 + i * 4 + 2] * 256 + dsd[tdNo].shmbuf[3 + i * 4 + 3]) * 65536;
                            float y = 0;
                            y = *((float *) &ycval);
                            y *= 100;
                            SetYcValue_float(rtuno, num + i, (BYTE *) &y, 1, utctime);
                        }
                        break;
                    case 8: // 遥测四字节整数(低字前0123)正序
                        ycnum = rtupara->ycnum;
                        num = getoffset(rtudata[rtuno].txPtr, 1, rtupara->type);
                        for (i = 0; i < MIN(datalen / 4, ycnum - num); i++) {
                            int ycval = dsd[tdNo].shmbuf[i * 4 + 3] + dsd[tdNo].shmbuf[i * 4 + 4] * 256 +
                                        (dsd[tdNo].shmbuf[i * 4 + 5] + dsd[tdNo].shmbuf[i * 4 + 6] * 256) * 65536;
                            SetYcValue_int(rtuno, num + i, (BYTE *) &ycval, 1, utctime);
                        }
                        break;
                    case 9: // 遥测四字节浮点(低字前0123)正序
                        ycnum = rtupara->ycnum;
                        num = getoffset(rtudata[rtuno].txPtr, 1, rtupara->type);
                        for (i = 0; i < MIN(datalen / 4, ycnum - num); i++) {
                            int ycval = dsd[tdNo].shmbuf[i * 4 + 3] + dsd[tdNo].shmbuf[i * 4 + 4] * 256 +
                                        (dsd[tdNo].shmbuf[i * 4 + 5] + dsd[tdNo].shmbuf[i * 4 + 6] * 256) * 65536;
                            float y = 0;
                            y = *((float *) &ycval);
                            y *= 100;
                            SetYcValue_float(rtuno, num + i, (BYTE *) &y, 1, utctime);
                        }
                        break;
                    case 10: // 电度帧整数(0123正序)
                        ycnum = rtupara->kwhnum;
                        num = getoffset(rtudata[rtuno].txPtr, 2, rtupara->type);
                        for (i = 0; i < MIN(datalen / 4, ycnum - num); i++) {
                            DWORD ycval = dsd[tdNo].shmbuf[i * 4 + 3] + dsd[tdNo].shmbuf[i * 4 + 4] * 256 +
                                          (dsd[tdNo].shmbuf[i * 4 + 5] + dsd[tdNo].shmbuf[i * 4 + 6] * 256) * 65536;
                            SetKwhValue(rtuno, num + i, ycval, utctime);
                        }
                        break;
                    case 11: // 电度帧整数(3210反序)
                        ycnum = rtupara->kwhnum;
                        num = getoffset(rtudata[rtuno].txPtr, 2, rtupara->type);
                        for (i = 0; i < MIN(datalen / 4, ycnum - num); i++) {
                            DWORD ycval = (dsd[tdNo].shmbuf[i * 4 + 3] * 256 + dsd[tdNo].shmbuf[i * 4 + 4]) * 65536 +
                                          (dsd[tdNo].shmbuf[i * 4 + 5] * 256 + dsd[tdNo].shmbuf[i * 4 + 6]);
                            SetKwhValue(rtuno, num + i, ycval, utctime);
                        }
                        break;
                    case 12: // 电度帧整数(低字前1032)
                        ycnum = rtupara->kwhnum;
                        num = getoffset(rtudata[rtuno].txPtr, 2, rtupara->type);
                        for (i = 0; i < MIN(datalen / 4, ycnum - num); i++) {
                            DWORD ycval = (dsd[tdNo].shmbuf[i * 4 + 3] * 256 + dsd[tdNo].shmbuf[i * 4 + 4]) +
                                          (dsd[tdNo].shmbuf[i * 4 + 5] * 256 + dsd[tdNo].shmbuf[i * 4 + 6]) * 65536;
                            SetKwhValue(rtuno, num + i, ycval, utctime);
                        }
                        break;
                    case 13: // 电度帧浮点(0123正序)
                        ycnum = rtupara->kwhnum;
                        num = getoffset(rtudata[rtuno].txPtr, 2, rtupara->type);
                        for (i = 0; i < MIN(datalen / 4, ycnum - num); i++) {
                            DWORD ycval = dsd[tdNo].shmbuf[i * 4 + 3] + dsd[tdNo].shmbuf[i * 4 + 4] * 256 +
                                          (dsd[tdNo].shmbuf[i * 4 + 5] + dsd[tdNo].shmbuf[i * 4 + 6] * 256) * 65536;
                            float y = 0;
                            y = *((float *) &ycval);
                            ycval = y * 100;
                            SetKwhValue(rtuno, num + i, ycval, utctime);
                        }
                        break;
                    case 14: // 电度帧浮点(3210反序)
                        ycnum = rtupara->kwhnum;
                        num = getoffset(rtudata[rtuno].txPtr, 2, rtupara->type);
                        for (i = 0; i < MIN(datalen / 4, ycnum - num); i++) {
                            DWORD ycval = (dsd[tdNo].shmbuf[i * 4 + 3] * 256 + dsd[tdNo].shmbuf[i * 4 + 4]) * 65536 +
                                          (dsd[tdNo].shmbuf[i * 4 + 5] * 256 + dsd[tdNo].shmbuf[i * 4 + 6]);
                            float y = 0;
                            y = *((float *) &ycval);
                            ycval = y * 100;
                            SetKwhValue(rtuno, num + i, ycval, utctime);
                        }
                        break;

                    case 15: // 电度帧浮点(1032低字前)
                        ycnum = rtupara->kwhnum;
                        num = getoffset(rtudata[rtuno].txPtr, 2, rtupara->type);
                        for (i = 0; i < MIN(datalen / 4, ycnum - num); i++) {
                            DWORD ycval = (dsd[tdNo].shmbuf[i * 4 + 3] * 256 + dsd[tdNo].shmbuf[i * 4 + 4]) +
                                          (dsd[tdNo].shmbuf[i * 4 + 5] * 256 + dsd[tdNo].shmbuf[i * 4 + 6]) * 65536;
                            float y = 0;
                            y = *((float *) &ycval);
                            ycval = y * 100;
                            SetKwhValue(rtuno, num + i, ycval, utctime);
                        }
                        break;
                    case 16: // 遥测浮点（3210）
                        ycnum = rtupara->ycnum;

                        num = getoffset(rtudata[rtuno].txPtr, 1, rtupara->type);
                        for (i = 0; i < MIN(datalen / 4, ycnum - num); i++) {
                            int ycval =
                                    (dsd[tdNo].shmbuf[3 + i * 4] * 256 + dsd[tdNo].shmbuf[3 + i * 4 + 1]) * 256 * 256 +
                                    (dsd[tdNo].shmbuf[3 + i * 4 + 2] * 256 + dsd[tdNo].shmbuf[3 + i * 4 + 3]);
                            float y = 0;
                            y = *((float *) &ycval);
                            y *= 100;
                            SetYcValue_float(rtuno, num + i, (BYTE *) &y, 1, utctime);
                        }
                        break;
                    case 17: // 二字节有符号遥测帧(01)
                        ycnum = rtupara->ycnum;
                        num = getoffset(rtudata[rtuno].txPtr, 1, rtupara->type);
                        for (i = 0; i < MIN(datalen / 2, ycnum - num); i++) {
                            short ycval = dsd[tdNo].shmbuf[i * 2 + 3] + dsd[tdNo].shmbuf[i * 2 + 4] * 256;
                            SetYcValue_short(rtuno, num + i, (BYTE *) &ycval, 1, utctime);
                        }
                        break;

                    case 18: // 二字节有符号遥测帧(01)高位符号其余绝对值
                        ycnum = rtupara->ycnum;
                        num = getoffset(rtudata[rtuno].txPtr, 1, rtupara->type);
                        for (i = 0; i < MIN(datalen / 2, ycnum - num); i++) {
                            short ycval;
                            int v = dsd[tdNo].shmbuf[i * 2 + 3] + dsd[tdNo].shmbuf[i * 2 + 4] * 256;
                            if (v & 0x8000)
                                ycval = (-1) * (v & 0x7fff);
                            else
                                ycval = v;

                            SetYcValue_short(rtuno, num + i, (BYTE *) &ycval, 1, utctime);
                        }
                        break;
                    case 19: // 二字节无符号遥测帧(01)
                        ycnum = rtupara->ycnum;
                        num = getoffset(rtudata[rtuno].txPtr, 1, rtupara->type);
                        for (i = 0; i < MIN(datalen / 2, ycnum - num); i++) {
                            int ycval = dsd[tdNo].shmbuf[i * 2 + 3] + dsd[tdNo].shmbuf[i * 2 + 4] * 256;
                            SetYcValue_int(rtuno, num + i, (BYTE *) &ycval, 1, utctime);
                        }
                        break;
                    case 20: // 江苏电科院光伏项目-阳光逆变器
                        procdata_20(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 21: // 怀柔光伏项目-阳光逆变器
                        procdata_21(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 22: // 怀柔光伏项目-固特威36kW逆变器
                        procdata_22(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 26: // 江苏电科院光伏项目-nari逆变器
                        procdata_26(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 27: // 北京经研院-nari逆变器
                        procdata_27(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 28: // 北京经研院-气象站
                        procdata_28(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 29: // 江苏电科院储能项目PCS
                        procdata_29(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 30: // 江苏电科院储能项目BMS
                        procdata_30(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 31: // 清川院水光项目PCS
                        procdata_31(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 32: // 北菜配电室
                        procdata_32(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 33: // 怀柔光伏项目-固特威HT逆变器
                        procdata_33(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 34: // 怀柔光伏项目-华为逆变器
                        procdata_34(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 35: // 怀柔光伏项目-阳光逆变器
                        procdata_35(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 36: // 怀柔光伏项目-安科瑞电表
                        procdata_36(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 37: // 热失控传感器
                        procdata_37(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 38: // 超声波传感器
                        procdata_38(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 39: // 温度检测
                        procdata_39(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 40: // 电源
                        procdata_40(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 44: // 电信园-锦浪光伏逆变器-V19
                        procdata_44(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 47: // BCU
                        procdata_47(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 49: // PCS
                        procdata_49(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 52: // BMS-PLC液冷机
                        procdata_52(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 53: // BMS-电表
                        procdata_53(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                    case 54: // BMS-消防
                        procdata_54(rtuno, (pmod + rtudata[rtuno].txPtr)->startaddr, datalen, dsd[tdNo].shmbuf + 3);
                        break;
                        break;
                }

                rtudata[rtuno].txPtr++;
                if (rtudata[rtupara->rtuNo].txPtr >= modbusrcnt) {
                    rtudata[rtupara->rtuNo].txPtr = 0;
                    rtuloop[rtuno]++;
                }

                return 0;
            } break;
            case 0x05: // 遥控
            {
                YKRETSTRUCT *ykret = GetYkRetPara(rtuno);
                RXCOMMAND Mail;
                if (GetCommandPara(rtuno, Mail) == FALSE)
                    return FALSE;
                RXCOMMAND *rxcommand = &Mail;
                ykret->rtuno = rtuno;
                ykret->ykno = rxcommand->data.yk[4] + rxcommand->data.yk[5] * 256;
                ykret->ret = 1;
                ykret->ykprocflag = 1;

                return 0;
            } break;
            case 0x06: // 单点遥调
            {
                YKRETSTRUCT *ykret = GetYkRetPara(rtuno);
                RXCOMMAND Mail;
                if (GetCommandPara(rtuno, Mail) == FALSE)
                    return FALSE;
                RXCOMMAND *rxcommand = &Mail;
                ykret->rtuno = rtuno;
                ykret->ykno = rxcommand->data.yk[4] + rxcommand->data.yk[5] * 256;
                ykret->ret = 1;
                ykret->ykprocflag = 1;
                //            if(rtupara->type == 1)
                //            {
                //                YKRETSTRUCT *ykret = GetYkRetPara(rtuno);
                //                RXCOMMAND Mail ;
                //                if(GetCommandPara(rtuno,Mail) == FALSE) return FALSE;
                //                RXCOMMAND *rxcommand = &Mail;
                //                ykret->rtuno = rtuno;
                //                ykret->ykno = rxcommand->data.yk[4]+rxcommand->data.yk[5]*256;
                //                ykret->ret = 1;
                //                ykret->ykprocflag = 1;
                //            }
                //            else
                //            {
                //                RXCOMMAND *rxcommand = GetRxcommand(rtuno);
                //                if(rxcommand->data.buf[0] == 0x01)
                //                {
                //                    rxcommand->data.buf[0] = rxcommand->data.buf[0] + 0x01;
                //                }
                //                else
                //                {
                //                    YKRETSTRUCT *ykret = GetYkRetPara(rtuno);
                //                    RXCOMMAND Mail ;
                //                    if(GetCommandPara(rtuno,Mail) == FALSE) return FALSE;
                //                    RXCOMMAND *rxcommand = &Mail;
                //                    ykret->rtuno = rtuno;
                //                    ykret->ykno = rxcommand->data.yk[4]+rxcommand->data.yk[5]*256;
                //                    ykret->ret = 1;
                //                    ykret->ykprocflag = 1;
                //                }

                //            }
                return 0;
            } break;
            case 0x10: // 多点遥调
            {
                YKRETSTRUCT *ykret = GetYkRetPara(rtuno);
                RXCOMMAND Mail;
                if (GetCommandPara(rtuno, Mail) == FALSE)
                    return FALSE;
                RXCOMMAND *rxcommand = &Mail;
                ykret->rtuno = rtuno;
                ykret->ykno = rxcommand->data.yk[4] + rxcommand->data.yk[5] * 256;
                ykret->ret = 1;
                ykret->ykprocflag = 1;
                rtudata[rtuno].txPtr++;
                if (rtudata[rtupara->rtuNo].txPtr >= modbusrcnt) {
                    rtudata[rtupara->rtuNo].txPtr = 0;
                    // chanloop[tdNo]++;
                    rtuloop[rtuno]++;
                }
                return 0;
            } break;
            case 0x95: // 写文件
            {
                fileFlag = 0;
                free(filebuf);
                filebuf = NULL;
                fileFlag = 0;
                fileType = 0;
                fileGroupNo = 0;
                fileGroupNum = 0;
                fileReadNum = 0;
                return 0;
            } break;
        }
    }
    return 1;
}

int CModbus::procdata_29(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    int yxno, no = 0;
    if (startaddr == 900) {
        yxno = 0;
        procdata_yc_word(rtuno, 0, 19, buf + no, byteorder, 0x01);
        no += 38;
        procdata_yx(rtuno, yxno, 4, 1, buf + no, byteorder);
        no += 1;
        yxno += 4;
        procdata_yx(rtuno, yxno, 5, 2, buf + no, byteorder);
        no += 1;
        yxno += 5;
        procdata_yx(rtuno, yxno, 4, 1, buf + no, byteorder);
        no += 1;
        yxno += 4;
        procdata_yx(rtuno, yxno, 2, 6, buf + no, byteorder);
        no += 1;
        yxno += 2;
        procdata_yx(rtuno, yxno, 8, 0, buf + no, byteorder);
        no += 1;
        yxno += 8;
        procdata_yx(rtuno, yxno, 8, 0, buf + no, byteorder);
        no += 1;
        yxno += 8;
        procdata_yx(rtuno, yxno, 8, 0, buf + no, byteorder);
        no += 1;
        yxno += 8;

    } else if (startaddr == 957) {
        procdata_yc_word(rtuno, 19, 3, buf, byteorder, 0x01);
    }
    return 1;
}

int CModbus::procdata_32(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x01; // 字序，高字在前
    int ycno, no;

    if (startaddr == 256) {
        ycno = 0;
        no = 0;
        procdata_yc_word(rtuno, ycno, 9, buf + no, byteorder, 0x00);
        ycno += 9;
        no += 18;
        procdata_yc_word(rtuno, ycno, 12, buf + no, byteorder, 0x01);
        ycno += 12;
        no += 24;
        procdata_yc_word(rtuno, ycno, 5, buf + no, byteorder, 0x01);
        ycno += 5;
        no += 10;
    } else if (startaddr == 63) {
        ycno = 26;
        procdata_yc_dword(rtuno, ycno, 4, buf, byteorder, wordorder, 0x00);
    }

    return 1;
}

int CModbus::procdata_31(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x00; // 字序，低字在前
    int ycno, yxno, no;

    if (startaddr == 15205) {
        ycno = 0;
        procdata_yc_word(rtuno, ycno, 4, buf, byteorder, 0x00);
    } else if (startaddr == 20102) {
        ycno = 4;
        procdata_yc_word(rtuno, ycno, 2, buf, byteorder, 0x00);
    } else if (startaddr == 25201) {
        ycno = 6, yxno = 0, no = 0;
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
        ycno += 2;
        no += 4;
        no += 4;
        procdata_yc_word(rtuno, ycno, 12, buf + no, byteorder, 0x00);
        ycno += 12;
        no += 24;
        no += 16;
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
        ycno += 2;
        no += 4;
        no += 12;
        procdata_yc_word(rtuno, ycno, 3, buf + no, byteorder, 0x00);
        ycno += 3;
        no += 6;
        no += 2;
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno++;
        no += 2;
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno++;
        no += 2;
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno++;
        no += 2;
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno++;
        no += 2;
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno++;
        no += 2;
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno++;
        no += 2;

    } else if (startaddr == 25261) {
        yxno = 6;
        no = 0;
        procdata_yx(rtuno, yxno, 16, 0, buf + no, byteorder);
        yxno += 16;
        no += 2;
        procdata_yx(rtuno, yxno, 15, 0, buf + no, byteorder);
        yxno += 15;
        no += 2;
        no += 4;
        procdata_yx(rtuno, yxno, 11, 0, buf + no, byteorder);
        yxno += 11;
        no += 2;
    }

    return 1;
}

int CModbus::procdata_30(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x00; // 字序，低字在前
    int ycno, no = 0;
    if (startaddr == 10000) {
        ycno = 0;
        procdata_yc_word(rtuno, ycno, 15, buf + no, byteorder, 0x00);
        ycno += 15;
        no += 30;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
        ycno += 1;
        no += 2;
        procdata_yc_word(rtuno, ycno, 11, buf + no, byteorder, 0x00);
        ycno += 11;
        no += 22;
        procdata_yc_dword(rtuno, ycno, 10, buf + no, byteorder, wordorder, 0x00);
        ycno += 10;
        no += 40;
        procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x00);
    } else if (startaddr == 10064) {
        ycno = 41;
        procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x00);
    } else if (startaddr == 10128) {
        ycno = 63;
        procdata_yc_word(rtuno, ycno, 100, buf + no, byteorder, 0x00);

    } else if (startaddr == 10228) {
        ycno = 163;
        procdata_yc_word(rtuno, ycno, 100, buf + no, byteorder, 0x00);

    } else if (startaddr == 10328) {
        ycno = 263;
        procdata_yc_word(rtuno, ycno, 100, buf + no, byteorder, 0x00);
    } else if (startaddr == 10428) {
        ycno = 363;
        procdata_yc_word(rtuno, ycno, 100, buf + no, byteorder, 0x00);
    } else if (startaddr == 11152) {
        ycno = 463;
        procdata_yc_word(rtuno, ycno, 120, buf + no, byteorder, 0x00);
    }

    return 1;
}

int CModbus::procdata_28(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    procdata_yc_word(rtuno, 0, 13, buf, byteorder, 0x01);
    return 1;
}

int CModbus::procdata_27(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x00; // 字序，低字在前
    int ycno = 0, no = 0;
    if (startaddr == 0) {
        procdata_yc_word(rtuno, ycno, 25, buf, byteorder, 0x01);
    } else if (startaddr == 26) {
        ycno = 25;
        procdata_yc_dword(rtuno, ycno, 4, buf, byteorder, wordorder, 0x00);
    }

    return 1;
}


int CModbus::procdata_26(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x00; // 字序，低字在前
    int ycno = 0, i, no = 0;
    procdata_yc_word(rtuno, ycno, 10, buf + no, byteorder, 0x01);
    ycno += 10;
    no += 20;
    no += 6;
    procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
    ycno += 1;
    no += 2;
    no += 6;
    procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x01);
    ycno += 2;
    no += 4;
    no += 2;
    procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
    ycno += 1;
    no += 2;
    no += 4;
    procdata_yc_dword(rtuno, ycno, 6, buf + no, byteorder, wordorder, 0x00);
    return 1;
}

int CModbus::procdata_21(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x01; // 字序，低字在前
    int yxno, ycno, i, no;
    switch (startaddr) {
        case 5002:
            ycno = 0;
            no = 0;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            procdata_yc_dword(rtuno, ycno, 2, buf + no, byteorder, wordorder, 0x00);
            ycno += 2;
            no += 8;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            procdata_yc_word(rtuno, ycno, 6, buf + no, byteorder, 0x00);
            ycno += 6;
            no += 12;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            procdata_yc_word(rtuno, ycno, 6, buf + no, byteorder, 0x00);
            ycno += 6;
            no += 12;
            no += 12;
            procdata_yc_dword(rtuno, ycno, 2, buf + no, byteorder, wordorder, 0x00);
            ycno += 2;
            no += 8;

            break;
        case 5034:
            ycno = 20;
            no = 0;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            no += 2;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2; // 设备工作状态
            no += 12;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2; // 告警码
            no += 6;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            break;
        case 5070:
            ycno = 25;
            no = 0;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            no += 10;
            procdata_yc_dword(rtuno, ycno, 3, buf + no, byteorder, wordorder, 0x00);
            no += 8;
            yxno = 1;
            procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder); // 停机
            yxno = 2;
            procdata_yx(rtuno, yxno, 1, 4, buf + no, byteorder); // 待机
            yxno = 4;
            procdata_yx(rtuno, yxno, 5, 9, buf + no, byteorder); // 其他
            no += 2;
            yxno = 0;
            procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder); // 运行
            yxno = 3;
            procdata_yx(rtuno, yxno, 1, 2, buf + no, byteorder); // 故障

            break;
        case 5112:
            ycno = 29;
            no = 0;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            no += 2;
            procdata_yc_word(rtuno, ycno, 10, buf + no, byteorder, 0x00);
            ycno += 10;
            no += 20;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            no += 4;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            procdata_yc_word(rtuno, ycno, 8, buf + no, byteorder, 0x00);
            ycno += 8;
            no += 16;
            no += 4;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4; // 逆变器状态
            no += 4;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            break;
        case 5145:
            ycno = 53;
            no = 0;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4;
            break;
        case 7012:
            ycno = 56;
            no = 0;
            procdata_yc_word(rtuno, ycno, 24, buf + no, byteorder, 0x00);
            ycno += 24;
            no += 48;
            break;
        case 5005:
            yxno = 8;
            no = 0;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno += 1;
            no += 2;
            procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
            yxno += 1;
            no += 2;
            break;
        case 5035:
            yxno = 10;
            ycno = 80;
            no = 0;
            procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
            yxno += 1;
            no += 2;
            no += 4;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            break;
    }
    return 1;
}

int CModbus::procdata_34(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x01; // 字序，高字在前
    int yxno, ycno, i, no;
    switch (startaddr) {
        case 30071:
            ycno = 0;
            no = 0;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4;
            procdata_yc_dword(rtuno, ycno, 5, buf + no, byteorder, wordorder, 0x00);
            break;
        case 32000:
            yxno = 0;
            ycno = 7;
            no = 0;
            procdata_yx(rtuno, yxno, 9, 0, buf + no, byteorder);
            yxno += 9;
            no += 2;
            procdata_yx(rtuno, yxno, 3, 0, buf + no, byteorder);
            yxno += 3;
            no += 2;
            procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
            yxno += 2;
            no += 2;
            procdata_yx(rtuno, yxno, 16, 0, buf + no, byteorder);
            yxno += 16;
            no += 2;
            procdata_yx(rtuno, yxno, 16, 0, buf + no, byteorder);
            yxno += 16;
            no += 2;
            procdata_yx(rtuno, yxno, 16, 0, buf + no, byteorder);
            yxno += 16;
            no += 2;
            procdata_yx(rtuno, yxno, 16, 0, buf + no, byteorder);
            yxno += 16;
            no += 2;
            procdata_yx(rtuno, yxno, 4, 0, buf + no, byteorder);
            yxno += 4;
            no += 2;
            procdata_yc_word(rtuno, ycno, 48, buf + no, byteorder, 0x01);
            break;
        case 32064:
            ycno = 63;
            no = 0;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            procdata_yc_word(rtuno, ycno, 6, buf + no, byteorder, 0x00);
            ycno += 6;
            no += 12;
            procdata_yc_dword(rtuno, ycno, 6, buf + no, byteorder, wordorder, 0x00);
            ycno += 6;
            no += 24;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            no += 12;
            procdata_yc_dword(rtuno, ycno, 2, buf + no, byteorder, wordorder, 0x00);
            ycno += 2;
            no += 8;
            break;
        case 32344:
            ycno = 55;
            no = 0;
            procdata_yc_word(rtuno, ycno, 8, buf + no, byteorder, 0x01);
            ycno += 8;
            no += 16;
            break;
        case 35300:
            ycno = 83;
            no = 0;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            break;
        case 40120:
            ycno = 89;
            no = 0;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            no += 4;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            break;
    }
    return 1;
}

int CModbus::procdata_33(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x01; // 字序，高字在前
    int yxno, ycno, i, no;
    Self64 utctime = GetCurMSTime();
    switch (startaddr) {
        case 32016:
            ycno = 0;
            no = 0;
            procdata_yc_word(rtuno, ycno, 48, buf + no, byteorder, 0x01);
            break;
        case 32066:
            ycno = 48;
            no = 0;
            procdata_yc_word(rtuno, ycno, 6, buf + no, byteorder, 0x00);
            ycno += 6;
            no += 12;
            procdata_yc_dword(rtuno, ycno, 3, buf + no, byteorder, wordorder, 0x00);
            ycno += 3;
            no += 12;
            no += 4;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            no += 6;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            no += 2;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            no += 36;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            no += 1;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            break;
        case 35710:
            ycno = 62;
            yxno = 0;
            no = 0;
            procdata_yx(rtuno, yxno, 1, 15, buf, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 14, buf, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 13, buf, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 9, buf, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 8, buf, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 4, buf, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 3, buf, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 2, buf, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 1, buf, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 0, buf, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 15, buf + 2, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 13, buf + 2, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 12, buf + 2, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 11, buf + 2, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 10, buf + 2, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 9, buf + 2, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 8, buf + 2, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 7, buf + 2, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 6, buf + 2, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 5, buf + 2, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 2, buf + 2, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 1, buf + 2, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 0, buf + 2, byteorder);
            yxno++;
            no += 4;
            no += 12;
            procdata_yx(rtuno, yxno, 1, 14, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 13, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 12, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 6, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 5, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 2, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            procdata_yx(rtuno, yxno, 1, 15, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 13, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 12, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 11, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 10, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 9, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 8, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 7, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 6, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 5, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 4, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 3, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 2, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            no += 60;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            no += 14;
            yxno = 46;
            SetSingleYxValue(rtuno, yxno, 0x00, utctime, 0);
            SetSingleYxValue(rtuno, yxno + 1, 0x00, utctime, 0);
            SetSingleYxValue(rtuno, yxno + 2, 0x00, utctime, 0);
            SetSingleYxValue(rtuno, yxno + 3, 0x00, utctime, 0);
            SetSingleYxValue(rtuno, yxno + *(buf + no + 1), 0x01, utctime, 0);
            break;
        case 41322:
            ycno = 63;
            no = 0;
            yxno = 50;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            no += 4;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            no += 6;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            break;
        case 41480:
            ycno = 66;
            no = 0;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x01);
            break;
    }
    return 1;
}

int CModbus::procdata_22(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x01; // 字序，高字在前
    int yxno, ycno, no;
    Self64 utctime = GetCurMSTime();
    switch (startaddr) {
        case 0:
            ycno = 0;
            no = 0;
            procdata_yc_word(rtuno, ycno, 6, buf + no, byteorder, 0x00);
        case 256: // yt
            ycno = 6;
            no = 0;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4;
            procdata_yc_dword(rtuno, ycno, 2, buf + no, byteorder, wordorder, 0x00);
            ycno += 2;
            no += 8;
            break;
        case 768:
            yxno = 27;
            ycno = 10;
            no = 0;
            procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x01);
            ycno += 4;
            no += 8;
            procdata_yc_word(rtuno, ycno, 9, buf + no, byteorder, 0x00);
            ycno += 9;
            no += 18;
            no += 2;
            SetSingleYxValue(rtuno, yxno, 0x00, utctime, 0);
            SetSingleYxValue(rtuno, yxno + 1, 0x00, utctime, 0);
            SetSingleYxValue(rtuno, yxno + 2, 0x00, utctime, 0);
            SetSingleYxValue(rtuno, yxno + 3, 0x00, utctime, 0);
            SetSingleYxValue(rtuno, *(buf + no + 1), 0x01, utctime, 0);
            no += 2;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;

            procdata_yx(rtuno, yxno, 1, 15, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 14, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 13, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 9, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 7, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 6, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 5, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 4, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 3, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 2, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            procdata_yx(rtuno, yxno, 1, 15, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 14, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 13, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 12, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 11, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 10, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 9, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 8, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 7, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 6, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 4, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 3, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 2, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            procdata_yc_dword(rtuno, ycno, 2, buf + no, byteorder, wordorder, 0x00);
            ycno += 2;
            no += 8;
            no += 6;
            yxno = 31;
            procdata_yx(rtuno, yxno, 1, 13, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 7, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 3, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 2, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
            yxno++;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            no += 12;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            break;
        case 850:
            ycno = 27;
            no = 0;
            yxno = 37;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            no += 6;
            procdata_yc_word(rtuno, ycno, 20, buf + no, byteorder, 0x01);
            ycno += 20;
            no += 40;
            procdata_yx(rtuno, yxno, 15, 0, buf + no, byteorder);
            break;
    }
    return 1;
}

int CModbus::procdata_37(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    int ycno, yxno, no, i;
    unsigned short tempdata;
    switch (startaddr) {
        case 1:
            ycno = 0;
            yxno = 0;
            no = 0;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            procdata_yc_word(rtuno, ycno, 27, buf + no, byteorder, 0x00);
            ycno += 27;
            no += 54;
            break;
        case 0:
            ycno = 27;
            yxno = 1;
            no = 0;
            procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x00);
            ycno += 4;
            no += 8;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            for (i = 0; i < 3; i++) {
                tempdata = (*(buf + no)) * 256 + (*(buf + no + 1));
                if (tempdata != 0) {
                    SetSingleYxValue(rtuno, tempdata - 1 + yxno, 0x01);
                }
                no += 2;
                yxno += 7;
            }
            break;
    }
    return 1;
}

int CModbus::procdata_38(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    int ycno, yxno, no, i;
    unsigned short tempdata;
    switch (startaddr) {
        case 1:
            ycno = 0;
            no = 0;
            procdata_yc_word(rtuno, ycno, 11, buf + no, byteorder, 0x00);
            break;
        case 0:
            ycno = 11;
            yxno = 0;
            no = 0;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;
            tempdata = (*(buf + no)) * 256 + (*(buf + no + 1));
            if (tempdata != 0) {
                SetSingleYxValue(rtuno, tempdata - 1 + yxno, 0x01);
            }
            break;
    }
    return 1;
}

int CModbus::procdata_39(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x00; // 字序，高字在前
    int ycno, no;
    switch (startaddr) {
        case 0:
            ycno = 0;
            no = 0;
            procdata_yc_dword(rtuno, ycno, 32, buf + no, byteorder, wordorder, 0x00);
            break;
        case 64:
            ycno = 32;
            no = 0;
            procdata_yc_dword(rtuno, ycno, 32, buf + no, byteorder, wordorder, 0x00);
            break;
        case 160:
            ycno = 64;
            no = 0;
            procdata_yc_word(rtuno, ycno, 3, buf + no, byteorder, 0x00);
            break;
    }
    return 1;
}

int CModbus::procdata_40(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x00; // 字序，高字在前
    int ycno, yxno, no;

    switch (startaddr) {
        case 1:
            yxno = 0;
            no = 0;
            procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
            yxno++;
            no += 2;

            break;
        case 16:
            ycno = 0;
            no = 0;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            break;
        case 32:
            ycno = 5;
            no = 0;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            break;
        case 48:
            ycno = 3;
            no = 0;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4;
    }
    return 1;
}

int CModbus::procdata_36(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x00; // 字序，高字在前
    int ycno, no;
    switch (startaddr) {
        case 0:
            ycno = 0;
            no = 0;
            procdata_yc_dword(rtuno, ycno, 15, buf + no, byteorder, wordorder, 0x00);
            break;
        case 97:
            ycno = 15;
            no = 0;
            procdata_yc_word(rtuno, ycno, 6, buf + no, byteorder, 0x00);
            ycno += 6;
            no += 12;
            no += 32;
            procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x00);
            break;
        case 356:
            ycno = 25;
            no = 0;
            procdata_yc_dword(rtuno, ycno, 12, buf + no, byteorder, wordorder, 0x00);
            ycno += 12;
            no += 48;
            procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x01);
            break;
    }
    return 1;
}

int CModbus::procdata_54(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    int ycno = 0, yxno = 0, no = 0;
    int temp;
    unsigned short data;

    temp = *(buf + no); // VOC 状态
    SetYcValue_int(rtuno, ycno, (BYTE *) &temp, 1);
    ycno += 1;
    if (temp == 0 || temp == 0xff)
        data = 0;
    else if (temp == 2)
        data = 1;
    else if (temp == 4)
        data = 2;
    else if (temp == 5)
        data = 4;
    else if (temp == 8)
        data = 8;
    else if (temp == 11)
        data = 16;
    else if (temp == 10)
        data = 32;
    procdata_yx(rtuno, yxno, 6, 0, (BYTE *) &data, byteorder);
    yxno += 6;
    no += 1;

    temp = *(buf + no); // 系统状态
    SetYcValue_int(rtuno, ycno, (BYTE *) &temp, 1);
    ycno += 1;
    if (temp == 0 || temp == 0xff)
        data = 0;
    else if (temp == 2)
        data = 1;
    else if (temp == 4)
        data = 2;
    else if (temp == 5)
        data = 4;
    else if (temp == 8)
        data = 8;
    else if (temp == 11)
        data = 16;
    else if (temp == 10)
        data = 32;
    procdata_yx(rtuno, yxno, 6, 0, (BYTE *) &data, byteorder);
    yxno += 6;
    no += 1;

    temp = *(buf + no); // 当前温度
    SetYcValue_short(rtuno, ycno, (BYTE *) &temp, 1);
    ycno += 1;
    no += 1;

    temp = *(buf + no); // 温度报警状态
    SetYcValue_int(rtuno, ycno, (BYTE *) &temp, 1);
    ycno += 1;
    if (temp == 0 || temp == 0xff)
        data = 0;
    else if (temp == 2)
        data = 1;
    else if (temp == 4)
        data = 2;
    else if (temp == 5)
        data = 4;
    else if (temp == 8)
        data = 8;
    else if (temp == 11)
        data = 16;
    else if (temp == 10)
        data = 32;
    procdata_yx(rtuno, yxno, 6, 0, (BYTE *) &data, byteorder);
    yxno += 6;
    no += 1;

    procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
    ycno += 2;
    no += 4; // CO 浓度H2 浓度

    temp = *(buf + no + 1); // 烟雾报警状态
    SetYcValue_int(rtuno, ycno, (BYTE *) &temp, 1);
    ycno += 1;
    if (temp == 0 || temp == 0xff)
        data = 0;
    else if (temp == 2)
        data = 1;
    else if (temp == 4)
        data = 2;
    else if (temp == 5)
        data = 4;
    else if (temp == 8)
        data = 8;
    else if (temp == 11)
        data = 16;
    else if (temp == 10)
        data = 32;
    procdata_yx(rtuno, yxno, 6, 0, (BYTE *) &data, byteorder);
    yxno += 6;
    no += 2;

    procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
    yxno += 1;
    no += 2;
    procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
    yxno += 1;
    no += 2;
}

int CModbus::procdata_53(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x01; // 字序，低字在前
    int ycno = 0, no = 0;
    if (startaddr == 0 && datalen == 128) {
        procdata_yc_dword(rtuno, ycno, 15, buf + no, byteorder, wordorder, 0x00);
        ycno += 15;
        no += 60;
        no += 60;
        procdata_yc_word(rtuno, ycno, 3, buf + no, byteorder, 0x00);
        ycno += 3;
        no += 6;
    } else if (startaddr == 97 && datalen == 92) {
        ycno = 21;
        procdata_yc_word(rtuno, ycno, 6, buf + no, byteorder, 0x00);
        ycno += 6;
        no += 12;
        procdata_yc_word(rtuno, ycno, 16, buf + no, byteorder, 0x01);
        ycno += 16;
        no += 32;
        procdata_yc_word(rtuno, ycno, 16, buf + no, byteorder, 0x00);
        ycno += 16;
        no += 32;
        procdata_yc_dword(rtuno, ycno, 3, buf + no, byteorder, wordorder, 0x00);
        ycno += 3;
        no += 12;
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
        ycno += 2;
        no += 4;
    } else if (startaddr == 289 && datalen == 4) {
        ycno = 64;
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
        ycno += 2;
        no += 4;
    } else if (startaddr == 4353 && datalen == 120) {
        ycno = 66;
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
        ycno += 2;
        no += 4;
        procdata_yc_dword(rtuno, ycno, 23, buf + no, byteorder, wordorder, 0x00);
        ycno += 23;
        no += 92;
        procdata_yc_word(rtuno, ycno, 12, buf + no, byteorder, 0x00);
        ycno += 12;
        no += 24;
    }
}

int CModbus::procdata_52(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x01; // 字序，低字在前
    int ycno = 0, yxno = 0, no = 0;
    if (startaddr == 1024 && datalen == 6) {
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2;
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
    } else if (startaddr == 41739 && datalen == 8) {
        yxno = 1;
        ycno = 2;
        procdata_yc_word(rtuno, ycno, 3, buf + no, byteorder, 0x00);
        ycno += 3;
        no += 6;
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
    } else if (startaddr == 58880 && datalen == 46) {
        ycno = 5;
        yxno = 2;
        procdata_yc_word(rtuno, ycno, 21, buf + no, byteorder, 0x00);
        ycno += 21;
        no += 42;
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
    } else if (startaddr == 59136 && datalen == 98) {
        ycno = 28;
        yxno = 3;
        procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x01);
        ycno += 4;
        no += 8;
        procdata_yc_word(rtuno, ycno, 23, buf + no, byteorder, 0x00);
        ycno += 2;
        no += 4;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1;
        no += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 2, 0, buf + no, byteorder);
        yxno += 2;
        no += 2;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1;
        no += 2;

        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2;
        no += 2;
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2;
        ycno += 1;
        no += 4;
        procdata_yc_dword(rtuno, ycno, 8, buf + no, byteorder, wordorder, 0x00);
        ycno += 8;
        no += 32;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
    }

    return 1;
}

int CModbus::procdata_49(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    int ycno = 0, yxno = 0, no = 0;
    if (startaddr == 3 && datalen == 124) {
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x01);
        ycno += 2;
        no += 4; // 3,4
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x01);
        ycno += 2;
        no += 4; // 5,6
        procdata_yc_word(rtuno, ycno, 3, buf + no, byteorder, 0x00);
        ycno += 3;
        no += 6; // 7,8,9
        procdata_yc_word(rtuno, ycno, 7, buf + no, byteorder, 0x01);
        ycno += 7;
        no += 14; // 10-16
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1;
        no += 2; // 17
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x01);
        ycno += 2;
        no += 2; // 18
        no += 2;
        procdata_yc_word(rtuno, ycno, 3, buf + no, byteorder, 0x01);
        ycno += 3;
        no += 2; // 20
        no += 4;
        procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x01);
        ycno += 4;
        no += 2; // 23
        no += 6;
        procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x00);
        ycno += 4; // 27,28,29,30
        procdata_yx(rtuno, yxno, 7, 0, buf + no, byteorder);
        yxno += 7;
        no += 2; // 27
        no += 2;
        procdata_yx(rtuno, yxno, 16, 0, buf + no, byteorder);
        yxno += 16;
        no += 2; // 29
        procdata_yx(rtuno, yxno, 16, 0, buf + no, byteorder);
        yxno += 16;
        no += 2; // 30
        procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x00);
        ycno += 4;
        no += 8;
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; //
        procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x00);
        ycno += 4;
        no += 8;
        no += 12;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
        ycno += 1;
        no += 2; // 46
        no += 2;
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
        ycno += 2;
        no += 4; // 48,49
        no += 2;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
        ycno += 1;
        no += 2; // 51
        no += 6;
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
        ycno += 2;
        no += 4; // 55,56
        procdata_yc_word(rtuno, ycno, 8, buf + no, byteorder, 0x00);
        ycno += 8;
        no += 16; // 57
    }

    return 1;
}

int CModbus::procdata_47(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    int ycno, yxno, no = 0;
    if (startaddr == 1 && datalen == 86) {
        ycno = 0;
        yxno = 0;
        procdata_yc_word(rtuno, ycno, 6, buf + no, byteorder, 0x00);
        ycno += 6;
        no += 12;
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; // Pack均衡控制模式
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1; // Pack均衡启停使能
        procdata_yx(rtuno, yxno, 16, 0, buf + no, byteorder);
        yxno += 16;
        no += 2; // PackX均衡启停使能
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; // Pack风冷控制模式
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1; // Pack风冷启停使能
        procdata_yx(rtuno, yxno, 16, 0, buf + no, byteorder);
        yxno += 16;
        no += 2; // PackX风冷启停使能
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; // 簇液冷控制模式
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; // 簇液冷控制使能
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; // 系统保护状态复位
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; // 分断路器
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; // 接触器控制模式
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; // 接触器自动分合使能
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; // 预充电接触器分合使能
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; // 正极接触器分合使能
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; // 负极接触器分合使能

        procdata_yc_word(rtuno, ycno, 15, buf + no, byteorder, 0x00);
        ycno += 15;
        no += 30; // 电芯电压充电截止值
        procdata_yc_word(rtuno, ycno, 3, buf + no, byteorder, 0x01);
        ycno += 3;
        no += 6; // 电芯低温液冷启动值
        procdata_yc_word(rtuno, ycno, 6, buf + no, byteorder, 0x00);
        ycno += 6;
        no += 12; // 簇内电芯温差风扇/液冷启动值

    } else if (startaddr == 1 && datalen == 220) {
        ycno = 32;
        yxno = 43;
        unsigned short status = 0, ret = 0;

        if (byteorder)
            status = (*buf) * 256 + (*(buf + 1));
        else
            status = (*buf) + (*(buf + 1)) * 256;

        if (status == 0x00)
            ret = 0x01;
        else if (status == 0x01)
            ret = 0x02;
        else if (status == 0x10)
            ret = 0x04;
        else if (status == 0x11)
            ret = 0x08;
        else if (status == 0x12)
            ret = 0x10;
        else if (status == 0xf0)
            ret = 0x20;
        else
            ret = 0x00;

        procdata_yx(rtuno, yxno, 6, 0, (unsigned char *) &ret, 0x00);
        yxno += 6; // 电池系统运行状态
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
        ycno += 2;
        no += 4; // 电池系统运行状态
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x01);
        ycno += 2;
        no += 4; // 簇电流
        procdata_yc_word(rtuno, ycno, 9, buf + no, byteorder, 0x01);
        ycno += 9;
        no += 18; // 簇SOC
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; // 簇液冷机状态
        procdata_yx(rtuno, yxno, 3, 0, buf + no, byteorder);
        yxno += 3;
        no += 2; // 预充、正极、负极接触器
        procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x00);
        ycno += 4;
        no += 8; // 最大电芯电压
        procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x01);
        ycno += 4;
        no += 8; // 最高温度
        ycno = 1222;
        yxno = 73;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1;
        no += 2; // Pack通讯状态
        procdata_yx(rtuno, yxno, 1, 0, buf + no, byteorder);
        yxno += 1;
        no += 2; // 故障状态
        no += 8; // 预留
        procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
        ycno += 2;
        no += 4; // 软件版本
        ycno = 53;
        procdata_yc_word(rtuno, ycno, 79, buf + no, byteorder, 0x00); // 电压-电芯1-79

    } else if (startaddr == 111 && datalen == 220) {
        ycno = 132;
        procdata_yc_word(rtuno, ycno, 110, buf + no, byteorder, 0x00); // 电压-电芯80-189
    } else if (startaddr == 221 && datalen == 220) {
        ycno = 242;
        procdata_yc_word(rtuno, ycno, 99, buf + no, byteorder, 0x00);
        ycno += 99;
        no += 198; // 电压-电芯190-288
        procdata_yc_word(rtuno, ycno, 11, buf + no, byteorder, 0x01); // 温度1-11;

    } else if (startaddr == 331 && datalen == 220) {
        ycno = 352;
        procdata_yc_word(rtuno, ycno, 110, buf + no, byteorder, 0x01); // 温度12-121
    } else if (startaddr == 441 && datalen == 220) {
        ycno = 462;
        procdata_yc_word(rtuno, ycno, 110, buf + no, byteorder, 0x01); // 温度122-231
    } else if (startaddr == 551 && datalen == 220) {
        ycno = 572;
        procdata_yc_word(rtuno, ycno, 57, buf + no, byteorder, 0x01);
        ycno += 57;
        no += 114; // 温度232-288
        procdata_yc_word(rtuno, ycno, 53, buf + no, byteorder, 0x00); // 状态-电芯1-53
    } else if (startaddr == 661 && datalen == 220) {
        ycno = 682;
        procdata_yc_word(rtuno, ycno, 110, buf + no, byteorder, 0x00); // 状态-电芯54-163
    } else if (startaddr == 771 && datalen == 220) {
        ycno = 792;
        procdata_yc_word(rtuno, ycno, 110, buf + no, byteorder, 0x00); // 状态-电芯164-273
    } else if (startaddr == 881 && datalen == 220) {
        ycno = 902;
        yxno = 53;
        unsigned short status = 0, ret = 0;
        procdata_yc_word(rtuno, ycno, 32, buf + no, byteorder, 0x00);
        ycno += 32;
        no += 62; // 状态-电芯274-288,pack状态1-16,状态-簇1
        if (byteorder)
            status = (*(buf + no)) * 256 + (*(buf + no + 1));
        else
            status = (*(buf + no)) + (*(buf + no + 1)) * 256;
        ret = (0x01 << (status & 0x0f)) / 2;
        procdata_yx(rtuno, yxno, 6, 0, (unsigned char *) &ret, 0x00);
        yxno += 6;

        ret = (0x01 << ((status & 0xf0) >> 4)) / 2;
        if (!byteorder)
            ret = ret * 256;
        procdata_yx(rtuno, yxno, 6, 0, (unsigned char *) &ret, 0x00);
        yxno += 6;

        ret = (0x01 << ((status & 0x0f00) >> 8)) / 2;
        if (!byteorder)
            ret = ret * 256;
        procdata_yx(rtuno, yxno, 5, 0, (unsigned char *) &ret, 0x00);
        yxno += 5;

        ret = (0x01 << ((status & 0xf000) >> 12)) / 2;
        if (!byteorder)
            ret = ret * 256;
        procdata_yx(rtuno, yxno, 3, 0, (unsigned char *) &ret, 0x00);
        yxno += 3;
        no += 2;

        ycno = 1227;
        yxno = 74;
        if (byteorder)
            status = (*(buf + no)) * 256 + (*(buf + no + 1));
        else
            status = (*(buf + no)) + (*(buf + no + 1)) * 256;
        ret = (0x01 << (status & 0x0f)) / 2;
        procdata_yx(rtuno, yxno, 3, 0, (unsigned char *) &ret, 0x00);
        yxno += 3;

        ret = (0x01 << ((status & 0xf0) >> 4)) / 2;
        procdata_yx(rtuno, yxno, 3, 0, (unsigned char *) &ret, 0x00);
        yxno += 3;
        no += 2;

        no += 14;
        procdata_yc_word(rtuno, ycno, 70, buf + no, byteorder, 0x00); // 直流内阻-电芯1-70
    } else if (startaddr == 991 && datalen == 220) {
        ycno = 1004;
        procdata_yc_word(rtuno, ycno, 110, buf + no, byteorder, 0x00); // 直流内阻-电芯71-180
    } else if (startaddr == 1101 && datalen == 216) {
        ycno = 1114;
        procdata_yc_word(rtuno, ycno, 108, buf + no, byteorder, 0x00); // 直流内阻-电芯181-288
    }

    return 1;
}

int CModbus::procdata_44(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x00; // 字序，高字在前
    int ycno, yxno, no;
    switch (startaddr) {
        case 3004:
            ycno = 0, no = 0;
            procdata_yc_dword(rtuno, ycno, 4, buf + no, byteorder, wordorder, 0x00);
            ycno += 4;
            no += 16;
            no += 4;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            no += 2;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            break;
        case 3029:
            ycno = 6, no = 0;
            no += 2;
            procdata_yc_word(rtuno, ycno, 11, buf + no, byteorder, 0x00);
            ycno += 11;
            no += 22;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4;
            procdata_yc_dword(rtuno, ycno, 2, buf + no, byteorder, wordorder, 0x00);
            ycno += 2;
            no += 8;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4;
            procdata_yc_word(rtuno, ycno, 3, buf + no, byteorder, 0x01);
            ycno += 3;
            no += 6;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4;
            procdata_yc_dword(rtuno, ycno, 2, buf + no, byteorder, wordorder, 0x00);
            ycno += 2;
            no += 8;
            break;
        case 3298:
            ycno = 32, no = 0;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            procdata_yc_word(rtuno, ycno, 21, buf + no, byteorder, 0x00);
            ycno += 21;
            no += 42;
            break;
        case 3499:
            ycno = 54, no = 0;
            procdata_yc_word(rtuno, ycno, 10, buf + no, byteorder, 0x00);
            ycno += 10;
            no += 20;
        case 3529:
            ycno = 64, no = 0;
            procdata_yc_word(rtuno, ycno, 10, buf + no, byteorder, 0x01);
            ycno += 10;
            no += 20;
            break;
        case 3006:
            ycno = 74, yxno = 0, no = 0;
            procdata_yx(rtuno, yxno, 1, 5, buf + no, byteorder);
            yxno += 1;
            no += 2;
            no += 86;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            no += 34;
            procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
            yxno += 1;
            no += 2;
            procdata_yx(rtuno, yxno, 1, 5, buf + no, byteorder);
            yxno += 1;
            no += 2;
            break;
    }
    return 1;
}


int CModbus::procdata_35(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x01; // 字序，低字在前
    int ycno, yxno, i, j, no;
    switch (startaddr) {
        case 4999:
            ycno = 0, yxno = 0, no = 0;
            procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x00);
            ycno += 4;
            no += 8;
            procdata_yc_dword(rtuno, ycno, 2, buf + no, byteorder, wordorder, 0x00);
            ycno += 2;
            no += 8;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            procdata_yc_word(rtuno, ycno, 6, buf + no, byteorder, 0x00);
            ycno += 6;
            no += 12;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            procdata_yc_word(rtuno, ycno, 6, buf + no, byteorder, 0x00);
            ycno += 6;
            no += 12;
            no += 12;
            procdata_yc_dword(rtuno, ycno, 2, buf + no, byteorder, wordorder, 0x00);
            ycno += 2;
            no += 8;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            procdata_yc_word(rtuno, ycno, 3, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            no += 12;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            no += 6;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            no += 42;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            no += 10;
            procdata_yc_dword(rtuno, ycno, 3, buf + no, byteorder, wordorder, 0x00);
            ycno += 3;
            no += 12;
            no += 2;
            procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
            yxno++;
            no -= 2;
            procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
            yxno++;
            no += 2;
            procdata_yx(rtuno, yxno, 1, 2, buf + no, byteorder);
            yxno++;
            no -= 2;
            procdata_yx(rtuno, yxno, 5, 9, buf + no, byteorder);
            yxno += 5;
            break;
        case 5112:
            ycno = 33, no = 0;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            no += 2;
            procdata_yc_word(rtuno, ycno, 10, buf + no, byteorder, 0x00);
            ycno += 10;
            no += 20;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            no += 4;
            procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
            ycno += 1;
            no += 4;
            procdata_yc_word(rtuno, ycno, 2, buf + no, byteorder, 0x00);
            ycno += 2;
            no += 4;
            no += 28;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            break;
        case 7012:
            ycno = 50, no = 0;
            procdata_yc_word(rtuno, ycno, 18, buf + no, byteorder, 0x00);
            ycno += 18;
            no += 36;
            break;
        case 5006:
            ycno = 68, yxno = 8, no = 0;
            procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
            yxno++;
            no += 58;
            no += 6;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
            ycno += 1;
            no += 2;
            procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
            ycno += 1;
            no += 2;
            break;
    }
    return 1;
}

int CModbus::procdata_20(int rtuno, int startaddr, int datalen, unsigned char *buf) {
    char byteorder = 0x01; // 字节序,高字节在前
    char wordorder = 0x01; // 字序，低字在前
    int ycno, yxno, i, j, no;
    if (startaddr == 4999 && datalen == 166) {
        ycno = 0, yxno = 0, no = 0;
        procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x00);
        ycno += 4;
        no += 8;
        procdata_yc_dword(rtuno, ycno, 2, buf + no, byteorder, wordorder, 0x00);
        ycno += 2;
        no += 8;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
        ycno += 1;
        no += 2;
        procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
        ycno += 1;
        no += 4;
        procdata_yc_word(rtuno, ycno, 6, buf + no, byteorder, 0x00);
        ycno += 6;
        no += 12;
        procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
        ycno += 1;
        no += 4;
        procdata_yc_word(rtuno, ycno, 6, buf + no, byteorder, 0x00);
        ycno += 6;
        no += 12;
        no += 12;
        procdata_yc_dword(rtuno, ycno, 2, buf + no, byteorder, wordorder, 0x00);
        ycno += 2;
        no += 8;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
        ycno += 1;
        no += 2;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1;
        no += 2;
        no += 2;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1;
        no += 2;
        no += 12;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1;
        no += 2;
        no += 6;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1;
        no += 2;
        no += 42;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1;
        no += 2;
        no += 10;
        procdata_yc_dword(rtuno, ycno, 3, buf + no, byteorder, wordorder, 0x00);
        ycno += 3;
        no += 8;
        no += 2;
        procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
        yxno++;
        no -= 2;
        procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
        yxno++;
        no += 2;
        procdata_yx(rtuno, yxno, 1, 2, buf + no, byteorder);
        yxno++;
        no -= 2;
        procdata_yx(rtuno, yxno, 5, 9, buf + no, byteorder);
        yxno += 5;
    } else if (startaddr == 5112 && datalen == 70) {
        ycno = 32, no = 0;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1;
        no += 2;
        no += 2;
        procdata_yc_word(rtuno, ycno, 4, buf + no, byteorder, 0x00);
        ycno += 4;
        no += 8;
        no += 12;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
        ycno += 1;
        no += 2;
        no += 4;
        procdata_yc_dword(rtuno, ycno, 1, buf + no, byteorder, wordorder, 0x00);
        ycno += 1;
        no += 4;
        no += 32;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
        ycno += 1;
        no += 2;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1;
        no += 2;
    } else if (startaddr == 7012 && datalen == 20) {
        ycno = 41, no = 0;
        procdata_yc_word(rtuno, ycno, 10, buf + no, byteorder, 0x00);
        ycno += 10;
        no += 20;
    } else if (startaddr == 5006 && datalen == 68) {
        ycno = 51, yxno = 8, no = 0;

        procdata_yx(rtuno, yxno, 1, 1, buf + no, byteorder);
        yxno++;
        no += 58;
        procdata_yx(rtuno, yxno, 1, 7, buf + no, byteorder);
        yxno++;
        procdata_yx(rtuno, yxno, 1, 7, buf + no - 2, byteorder);
        no += 6;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x00);
        ycno += 1;
        no += 2;
        procdata_yc_word(rtuno, ycno, 1, buf + no, byteorder, 0x01);
        ycno += 1;
        no += 2;
    }
    //    switch (startaddr)
    //    {
    //    case 4999:
    //        ycno = 0,yxno = 0,no=0;
    //        procdata_yc_word(rtuno,ycno,4,buf+no,byteorder,0x00);ycno+=4;no+=8;
    //        procdata_yc_dword(rtuno,ycno,2,buf+no,byteorder,wordorder,0x00);ycno+=2;no+=8;
    //        procdata_yc_word(rtuno,ycno,1,buf+no,byteorder,0x01);ycno+=1;no+=2;
    //        procdata_yc_dword(rtuno,ycno,1,buf+no,byteorder,wordorder,0x00);ycno+=1;no+=4;
    //        procdata_yc_word(rtuno,ycno,6,buf+no,byteorder,0x00);ycno+=6;no+=12;
    //        procdata_yc_dword(rtuno,ycno,1,buf+no,byteorder,wordorder,0x00);ycno+=1;no+=4;
    //        procdata_yc_word(rtuno,ycno,6,buf+no,byteorder,0x00);ycno+=6;no+=12;
    //        no+=12;
    //        procdata_yc_dword(rtuno,ycno,2,buf+no,byteorder,wordorder,0x00);ycno+=2;no+=8;
    //        procdata_yc_word(rtuno,ycno,1,buf+no,byteorder,0x01);ycno+=1;no+=2;
    //        procdata_yc_word(rtuno,ycno,1,buf+no,byteorder,0x00);ycno+=1;no+=2;
    //        no+=2;
    //        procdata_yc_word(rtuno,ycno,1,buf+no,byteorder,0x00);ycno+=1;no+=2;
    //        no+=12;
    //        procdata_yc_word(rtuno,ycno,1,buf+no,byteorder,0x00);ycno+=1;no+=2;
    //        no+=6;
    //        procdata_yc_word(rtuno,ycno,1,buf+no,byteorder,0x00);ycno+=1;no+=2;
    //        no+=42;
    //        procdata_yc_word(rtuno,ycno,1,buf+no,byteorder,0x00);ycno+=1;no+=2;
    //        no+=10;
    //        procdata_yc_dword(rtuno,ycno,3,buf+no,byteorder,wordorder,0x00);ycno+=3;no+=8;
    //        no+=2;
    //        procdata_yx(rtuno,yxno,1,1,buf+no,byteorder);yxno++;
    //        no-=2;
    //        procdata_yx(rtuno,yxno,1,1,buf+no,byteorder);yxno++;
    //        no+=2;
    //        procdata_yx(rtuno,yxno,1,2,buf+no,byteorder);yxno++;
    //        no-=2;
    //        procdata_yx(rtuno,yxno,5,9,buf+no,byteorder);yxno+=5;
    //        break;
    //    case 5112:
    //        ycno = 32,no=0;
    //        procdata_yc_word(rtuno,ycno,1,buf+no,byteorder,0x00);ycno+=1;no+=2;
    //        no+=2;
    //        procdata_yc_word(rtuno,ycno,4,buf+no,byteorder,0x00);ycno+=4;no+=8;
    //        no+=12;
    //        procdata_yc_word(rtuno,ycno,1,buf+no,byteorder,0x01);ycno+=1;no+=2;
    //        no+=4;
    //        procdata_yc_dword(rtuno,ycno,1,buf+no,byteorder,wordorder,0x00);ycno+=1;no+=4;
    //        no+=32;
    //        procdata_yc_word(rtuno,ycno,1,buf+no,byteorder,0x01);ycno+=1;no+=2;
    //        procdata_yc_word(rtuno,ycno,1,buf+no,byteorder,0x00);ycno+=1;no+=2;
    //        break;
    //    case 7012:
    //        ycno = 41,no=0;
    //        procdata_yc_word(rtuno,ycno,10,buf+no,byteorder,0x00);ycno+=10;no+=20;
    //        break;
    //    case 5006:
    //        ycno = 51,yxno = 8,no=0;

    //        procdata_yx(rtuno,yxno,1,1,buf+no,byteorder);yxno++;
    //        no+=58;
    //        procdata_yx(rtuno,yxno,1,7,buf+no,byteorder);yxno++;
    //        procdata_yx(rtuno,yxno,1,7,buf+no-2,byteorder);
    //        no+=6;
    //        procdata_yc_word(rtuno,ycno,1,buf+no,byteorder,0x00);ycno+=1;no+=2;
    //        procdata_yc_word(rtuno,ycno,1,buf+no,byteorder,0x01);ycno+=1;no+=2;
    //        break;
    //    }
    return 1;
}

// datatype = 0x00 U32 & S32 ; = 0x01 float
void CModbus::procdata_yc_dword(int rtuno, int ycno, int ycnum, unsigned char *buf, char byteorder, char wordorder,
                                char datatype) {
    int ycvalue = 0;
    float ycvalue_f = 0;
    for (int i = 0; i < ycnum; i++) {
        if (wordorder) {
            if (byteorder) // 低字在前，高字节在前
            {
                ycvalue = *(buf + i * 4) * 256 + *(buf + i * 4 + 1) + *(buf + i * 4 + 2) * 256 * 256 * 256 +
                          *(buf + i * 4 + 3) * 256 * 256;
            } else // 低字在前，低字节在前
            {
                ycvalue = *(buf + i * 4) + *(buf + i * 4 + 1) * 256 + *(buf + i * 4 + 2) * 256 * 256 +
                          *(buf + i * 4 + 3) * 256 * 256 * 256;
            }
        } else {
            if (byteorder) // 高字在前，高字节在前
            {
                ycvalue = *(buf + i * 4) * 256 * 256 * 256 + *(buf + i * 4 + 1) * 256 * 256 + *(buf + i * 4 + 2) * 256 +
                          *(buf + i * 4 + 3);
            } else // 高字在前，低字节在前
            {
                ycvalue = *(buf + i * 4) * 256 * 256 + *(buf + i * 4 + 1) * 256 * 256 * 256 + *(buf + i * 4 + 2) +
                          *(buf + i * 4 + 3) * 256;
            }
        }
        Self64 utctime = GetCurMSTime();
        if (datatype) {
            ycvalue_f = *((float *) &ycvalue);
            ycvalue_f = ycvalue_f * 100;

            SetYcValue_float(rtuno, ycno + i, (BYTE *) &ycvalue_f, 1, utctime);
        } else {
            SetYcValue_int(rtuno, ycno + i, (BYTE *) &ycvalue, 1, utctime);
        }
    }
}

Self64 CModbus::GetCurMSTime() {
    Self64 t;

#if defined(WIN32)
    struct _timeb LongTime;
    _ftime(&LongTime);
    t = (Self64) LongTime.time * (Self64) 1000;
    t += (Self64) LongTime.millitm;
#else
    struct timeval LongTime;
    gettimeofday(&LongTime, NULL);
    t = (Self64) LongTime.tv_sec * (Self64) 1000;
    t += (Self64) (LongTime.tv_usec / 1000);
#endif
    return t;
}

// datatype = 0x00 U16 ;=0x01 S16
void CModbus::procdata_yc_word(int rtuno, int ycno, int ycnum, unsigned char *buf, char byteorder, char datatype) {
    short ycvalue_s = 0;
    int ycvalue_u = 0;
    int index = 0;
    Self64 utctime = GetCurMSTime();
    for (int i = 0; i < ycnum; i++) {
        if (byteorder) // 大端
        {
            if (datatype) // 有符号
            {
                ycvalue_s = (*(buf + index)) * 256 + (*(buf + index + 1));
                SetYcValue_short(rtuno, ycno + i, (BYTE *) &ycvalue_s, 1, utctime);

            } else {
                ycvalue_u = (*(buf + index)) * 256 + (*(buf + index + 1));
                SetYcValue_int(rtuno, ycno + i, (BYTE *) &ycvalue_u, 1, utctime);
            }
        } else {
            if (datatype) {
                ycvalue_s = (*(buf + index)) + (*(buf + index + 1)) * 256;
                SetYcValue_short(rtuno, ycno + i, (BYTE *) &ycvalue_s, 1, utctime);

            } else {
                ycvalue_u = (*(buf + index)) + (*(buf + index + 1)) * 256;
                SetYcValue_int(rtuno, ycno + i, (BYTE *) &ycvalue_u, 1, utctime);
            }
        }
        index += 2;
    }
}

void CModbus::procdata_yx(int rtuno, int yxno, int yxnum, int offset, unsigned char *buf, char byteorder) {
    unsigned short tempdata = 0;
    unsigned char yxvalue = 0x00;
    if (byteorder) {
        tempdata = (*(buf)) * 256 + (*(buf + 1));
    } else {
        tempdata = (*(buf)) + (*(buf + 1)) * 256;
    }
    Self64 utctime = GetCurMSTime();
    for (int i = 0; i < yxnum; i++) {
        yxvalue = (tempdata >> (i + offset)) & 0x01;
        SetSingleYxValue(rtuno, yxno + i, yxvalue, utctime, 0);
    }
}


int CModbus::txPro(int tdNo) {
    short rtuno = tdNo;
    if (rtuno == -1)
        return -1;

    RTUPARA *para = GetRtuPara(rtuno);
    if (para == NULL)
        return 0;
    tdNo = para->chanNo;
    if (dsd[tdNo].commFlag == 1)
        return 0;
    rtuno = GetCurrentRtuNo(tdNo);
    para = GetRtuPara(rtuno);

    rtudelay[rtuno]++;
    if (rtudelay[rtuno] > chanFreshTime) {
        RXCOMMAND *rxcommand = GetRxcommand(rtuno);
        if (rxcommand == NULL)
            return 0;
        if (rxcommand->flag == 1 && rxcommand->type == YKOPERATIONMSG && rxcommand->data.yk[1] == YKCMDMSG) {
            rxcommand->flag = 0;
            YKRETSTRUCT *ykret = GetYkRetPara(rtuno);
            ykret->rtuno = rtuno;
            ykret->ykno = rxcommand->data.yk[4] + rxcommand->data.yk[5] * 256;
            ykret->ret = 1;
            ykret->ykprocflag = 1;

        } else if (rxcommand->flag == 1 && rxcommand->type == YKOPERATIONMSG && rxcommand->data.yk[1] == YKEXEMSG) {
            usleep(400);
            // MakeYkExeFrame_goodwe(tdNo,para,rxcommand);
            MakeYkExeFrame(tdNo, para, rxcommand);
            // MakeYkExeFrame_jscn(tdNo,para,rxcommand);
            rxcommand->flag = 0;
        } else if (rxcommand->flag == 1 && rxcommand->type == YKOPERATIONMSG && rxcommand->data.yk[1] == YKDELMSG) {
            rxcommand->flag = 0;
            YKRETSTRUCT *ykret = GetYkRetPara(rtuno);
            ykret->rtuno = rtuno;
            ykret->ykno = rxcommand->data.yk[4] + rxcommand->data.yk[5] * 256;
            ykret->ret = 1;
            ykret->ykprocflag = 1;

        } else if (rxcommand->flag == 1 && (rxcommand->type == 48 || rxcommand->type == 50)) {
            MakeYTExeFrame(tdNo, para, rxcommand);
            rxcommand->flag = 0;
        } else {
            EditCallDataFrm(tdNo, para);
        }

        rtudelay[rtuno] = 0;
    }


    return 0;
}

void CModbus::flywheelControl(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    float YtValue = 0, ActValue = 0, CompenValue = 0;
    memcpy(&YtValue, rxcmd->data.buf + 7, 4);
    int value = GetYcValue(rtupara->rtuNo, 11);
    ActValue = value * 0.1;
    CompenValue = YtValue - ActValue;

    RXCOMMAND rx;
    rx.flag = 1;
    rx.type = 50;
    rx.cmdlen = 8;
    int i = 0;
    rx.data.buf[i++] = 0x00;
    rx.data.buf[i++] = 0x00;
    rx.data.buf[i++] = 0x00;
    rx.data.buf[i++] = 0x01;
    // infoaddr
    rx.data.buf[i++] = 0x00;
    rx.data.buf[i++] = 0x00;
    rx.data.buf[i++] = 0x00;
    memcpy(rx.data.buf + i, &CompenValue, 4);
    i += 4;
    rx.data.buf[i++] = 0x00;
    time_t atm = time(NULL);
    memcpy(rx.data.buf + i, &atm, sizeof(time_t));

    //    RXCOMMAND Mail ;
    //    if(GetCommandPara(0,Mail) == FALSE) return;
    RXCOMMAND *Mail = GetRxcommand(0);
    if (Mail->flag == 1)
        return;
    else
        memcpy(Mail, &rx, sizeof(RXCOMMAND));
}

void CModbus::MakeYTExeFrame_WaterLight(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int ykno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    int i = 0;
    float value = 0;
    BYTE len = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    if (ykno == 0) // 逆变输出电压设置
    {
        dsd[tdNo].shmbuf[i++] = 0x4e;
        dsd[tdNo].shmbuf[i++] = 0x86;
        memcpy(&value, rxcmd->data.buf + 7, 4);
        value = value * 10;
        dsd[tdNo].shmbuf[i++] = HIBYTE((short) (value));
        dsd[tdNo].shmbuf[i++] = LOBYTE((short) (value));
    } else // 逆变输出频率设置
    {
        dsd[tdNo].shmbuf[i++] = 0x4e;
        dsd[tdNo].shmbuf[i++] = 0x87;
        memcpy(&value, rxcmd->data.buf + 7, 4);
        value = value * 100;
        dsd[tdNo].shmbuf[i++] = HIBYTE((short) (value));
        dsd[tdNo].shmbuf[i++] = LOBYTE((short) (value));
    }
    alog_printf(ALOG_LVL_DEBUG, true, "ykno = %d,value = %.1f\n", ykno, value);
    len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYTExeFrame_goodwe_MT(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    float value = 0;
    BYTE len = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x10;
    dsd[tdNo].shmbuf[i++] = 0x01;
    dsd[tdNo].shmbuf[i++] = 0x00;
    dsd[tdNo].shmbuf[i++] = 0x00;
    dsd[tdNo].shmbuf[i++] = 0x01;
    dsd[tdNo].shmbuf[i++] = 0x02;
    memcpy(&value, rxcmd->data.buf + 6, 4);
    dsd[tdNo].shmbuf[i++] = (int) (value / 256) & 0xff;
    dsd[tdNo].shmbuf[i++] = (int) value & 0xff;
    len = 9;

    //        if(rxcmd->data.buf[4] == 0x00)//kw
    //        {

    //        }
    //        else if(rxcmd->data.buf[4] == 0x01)//kvar
    //        {
    //            dsd[tdNo].shmbuf[i++] = 0x01;
    //            dsd[tdNo].shmbuf[i++] = 0x02;
    //            dsd[tdNo].shmbuf[i++] = 0x00;
    //            dsd[tdNo].shmbuf[i++] = 0x02;
    //            dsd[tdNo].shmbuf[i++] = 0x04;
    //            memcpy(&value,rxcmd->data.buf+6,4);
    //            dsd[tdNo].shmbuf[i++] = (int)(value/(256*256*256)) & 0xff;
    //            dsd[tdNo].shmbuf[i++] = (int)(value/(256*256)) & 0xff;
    //            dsd[tdNo].shmbuf[i++] = (int)(value/256) & 0xff;
    //            dsd[tdNo].shmbuf[i++] = (int)value & 0xff;
    //            len = 11;
    //        }

    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYTExeFrame_pow(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    float value = 0;
    BYTE len = 0;
    int ytno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    if (ytno < 2) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(ytno + 48);
        dsd[tdNo].shmbuf[i++] = LOBYTE(ytno + 48);
    } else if (ytno < 4) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(ytno + 31);
        dsd[tdNo].shmbuf[i++] = LOBYTE(ytno + 31);
    } else {
        dsd[tdNo].shmbuf[i++] = HIBYTE(ytno + 31);
        dsd[tdNo].shmbuf[i++] = LOBYTE(ytno + 31);
    }

    memcpy(&value, rxcmd->data.buf + 7, 4);
    if (ytno == 4) {
        value = value * 1000;
    } else {
        value = value * 100;
    }

    dsd[tdNo].shmbuf[i++] = (int) (value) / 256 & 0xff;
    dsd[tdNo].shmbuf[i++] = (int) (value) & 0xff;
    len = 6;

    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYTExeFrame_AMC(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    float value = 0;
    BYTE len = 0;
    int ytno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    if (ytno == 0 || ytno == 1 || ytno == 2) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(60 + ytno);
        dsd[tdNo].shmbuf[i++] = LOBYTE(60 + ytno);
    } else if (ytno == 6 || ytno == 7) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(135 + ytno);
        dsd[tdNo].shmbuf[i++] = LOBYTE(135 + ytno);
    } else if (ytno == 8 || ytno == 9) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(ytno + 281);
        dsd[tdNo].shmbuf[i++] = LOBYTE(ytno + 281);
    }

    memcpy(&value, rxcmd->data.buf + 7, 4);
    dsd[tdNo].shmbuf[i++] = HIBYTE((short) value);
    dsd[tdNo].shmbuf[i++] = LOBYTE((short) value);
    len = 6;

    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYTExeFrame_PCS(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    float value = 0;
    BYTE len = 0;
    int ytno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    if (ytno == 0) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(36);
        dsd[tdNo].shmbuf[i++] = LOBYTE(36);
    } else if (ytno == 1) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(46);
        dsd[tdNo].shmbuf[i++] = LOBYTE(46);
    } else if (ytno == 2 || ytno == 3) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(ytno + 46);
        dsd[tdNo].shmbuf[i++] = LOBYTE(ytno + 46);
    } else if (ytno == 4) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(ytno + 47);
        dsd[tdNo].shmbuf[i++] = LOBYTE(ytno + 47);
    } else {
        dsd[tdNo].shmbuf[i++] = HIBYTE(ytno + 52);
        dsd[tdNo].shmbuf[i++] = LOBYTE(ytno + 52);
    }

    memcpy(&value, rxcmd->data.buf + 7, 4);
    if (ytno == 7 || ytno == 8) {
        value = value * 100;
    } else if (ytno != 0) {
        value = value * 10;
    }

    dsd[tdNo].shmbuf[i++] = HIBYTE((short) value);
    dsd[tdNo].shmbuf[i++] = LOBYTE((short) value);
    len = 6;

    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYTExeFrame_tem(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    float value = 0;
    BYTE len = 0;
    int ytno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    dsd[tdNo].shmbuf[i++] = HIBYTE(ytno + 161);
    dsd[tdNo].shmbuf[i++] = LOBYTE(ytno + 161);
    memcpy(&value, rxcmd->data.buf + 7, 4);
    dsd[tdNo].shmbuf[i++] = (int) (value / 256) & 0xff;
    dsd[tdNo].shmbuf[i++] = (int) value & 0xff;
    len = 6;

    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYTExeFrame_sen2(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    float value = 0;
    BYTE len = 0;
    int ytno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    dsd[tdNo].shmbuf[i++] = HIBYTE(ytno + 1);
    dsd[tdNo].shmbuf[i++] = LOBYTE(ytno + 1);
    memcpy(&value, rxcmd->data.buf + 7, 4);
    if (ytno == 9) {
        value = value * 1000;
    }
    dsd[tdNo].shmbuf[i++] = (int) (value / 256) & 0xff;
    dsd[tdNo].shmbuf[i++] = (int) value & 0xff;
    len = 6;

    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYTExeFrame_PLC(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    float value = 0;
    BYTE len = 0;
    int ytno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    if (ytno <= 1) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(ytno + 1025);
        dsd[tdNo].shmbuf[i++] = LOBYTE(ytno + 1025);
    } else if (ytno <= 4 && ytno >= 2) {
        dsd[tdNo].shmbuf[i++] = HIBYTE((ytno - 2) + 41739);
        dsd[tdNo].shmbuf[i++] = LOBYTE((ytno - 2) + 41739);
    } else if (ytno <= 9 && ytno >= 5) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(ytno - 5 + 58880);
        dsd[tdNo].shmbuf[i++] = LOBYTE(ytno - 5 + 58880);
    } else if (ytno == 10) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(58888);
        dsd[tdNo].shmbuf[i++] = LOBYTE(58888);
    } else if (ytno <= 19 && ytno >= 11) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(ytno - 11 + 58892);
        dsd[tdNo].shmbuf[i++] = LOBYTE(ytno - 11 + 58892);
    } else if (ytno == 20) {
        dsd[tdNo].shmbuf[i++] = HIBYTE(58902);
        dsd[tdNo].shmbuf[i++] = LOBYTE(58902);
    }


    memcpy(&value, rxcmd->data.buf + 7, 4);
    if (ytno == 0 || ytno == 1 || ytno == 9 || ytno == 10 || ytno == 20) {
        value = value * 1;
    } else {
        value = value * 10;
    }

    dsd[tdNo].shmbuf[i++] = HIBYTE((short) value);
    dsd[tdNo].shmbuf[i++] = LOBYTE((short) value);
    len = 6;

    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYTExeFrame_BCU(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    float value = 0;
    BYTE len = 0;
    int ytno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    if (ytno == 31 || ytno == 32) // 启动在线升级
    {
        memcpy(&value, rxcmd->data.buf + 7, 4);
        if (ytno == 31)
            fileType = 1;
        else
            fileType = 17;
        fileReadNum = getBcufile(fileType, (unsigned short) value);
        if (fileReadNum == 0) {
            YKRETSTRUCT *ykret = GetYkRetPara(rtupara->rtuNo);
            RXCOMMAND Mail;
            if (GetCommandPara(rtupara->rtuNo, Mail) == FALSE)
                return;
            RXCOMMAND *rxcommand = &Mail;
            ykret->rtuno = rtupara->rtuNo;
            ykret->ykno = rxcommand->data.yk[4] + rxcommand->data.yk[5] * 256;
            ykret->ret = 0;
            ykret->ykprocflag = 1;
            fileFlag = 0;
            SetYcValue_short(rtupara->rtuNo, 1225, (BYTE *) &fileFlag, 1);
        } else {
            fileFlag = 1;
            SetYcValue_short(rtupara->rtuNo, 1225, (BYTE *) &fileFlag, 1);
            if (fileReadNum % 128 == 0)
                fileGroupNum = fileReadNum / 128;
            else
                fileGroupNum = fileReadNum / 128 + 1;
        }
    } else {
        dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
        dsd[tdNo].shmbuf[i++] = 0x06;
        if (ytno <= 5) {
            dsd[tdNo].shmbuf[i++] = HIBYTE(ytno + 1);
            dsd[tdNo].shmbuf[i++] = LOBYTE(ytno + 1);
        } else if (ytno <= 7 && ytno >= 6) {
            dsd[tdNo].shmbuf[i++] = HIBYTE((ytno - 6) * 2 + 8);
            dsd[tdNo].shmbuf[i++] = LOBYTE((ytno - 6) * 2 + 8);
        } else {
            dsd[tdNo].shmbuf[i++] = HIBYTE(ytno + 12);
            dsd[tdNo].shmbuf[i++] = LOBYTE(ytno + 12);
        }


        memcpy(&value, rxcmd->data.buf + 7, 4);
        if (ytno == 1 || ytno == 4 || ytno == 5 || (ytno <= 28 && ytno >= 17)) {
            value = value * 10;
        }

        dsd[tdNo].shmbuf[i++] = HIBYTE((short) value);
        dsd[tdNo].shmbuf[i++] = LOBYTE((short) value);
        len = 6;

        unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
        dsd[tdNo].shmbuf[len] = crc % 256;
        dsd[tdNo].shmbuf[len + 1] = crc / 256;
        for (int i = 0; i < len + 2; i++) {
            BYTE val = dsd[tdNo].shmbuf[i];
            EnterTrnQ(tdNo, val);
        }
        dsd[tdNo].commFlag = 1;
        ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
    }
}

void CModbus::MakeYTExeFrame_sen1(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    float value = 0;
    BYTE len = 0;
    int ytno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    dsd[tdNo].shmbuf[i++] = HIBYTE(ytno + 2);
    dsd[tdNo].shmbuf[i++] = LOBYTE(ytno + 2);
    memcpy(&value, rxcmd->data.buf + 7, 4);
    if (ytno == 11 || ytno == 12 || ytno == 13 || ytno == 20 || ytno == 21 || ytno == 22) {
        value = value * 10;
    } else if (ytno == 23 || ytno == 25) {
        value = value * 1000;
    }
    dsd[tdNo].shmbuf[i++] = HIBYTE((short) value);
    dsd[tdNo].shmbuf[i++] = LOBYTE((short) value);
    len = 6;

    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYTExeFrame_goodwe_HT(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    float value = 0;
    BYTE len = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x10;
    dsd[tdNo].shmbuf[i++] = 0xA2;
    dsd[tdNo].shmbuf[i++] = 0x08;
    dsd[tdNo].shmbuf[i++] = 0x00;
    dsd[tdNo].shmbuf[i++] = 0x01;
    dsd[tdNo].shmbuf[i++] = 0x02;
    memcpy(&value, rxcmd->data.buf + 6, 4);
    dsd[tdNo].shmbuf[i++] = (int) (value / 256) & 0xff;
    dsd[tdNo].shmbuf[i++] = (int) value & 0xff;
    len = 9;
    //    if(rtupara->type == 1)//100kW
    //    {
    //        if(rxcmd->data.buf[4] == 0x00)//kw
    //        {
    //            dsd[tdNo].shmbuf[i++] = 0xA2;
    //            dsd[tdNo].shmbuf[i++] = 0x08;
    //            dsd[tdNo].shmbuf[i++] = 0x00;
    //            dsd[tdNo].shmbuf[i++] = 0x01;
    //            dsd[tdNo].shmbuf[i++] = 0x02;
    //            memcpy(&value,rxcmd->data.buf+6,4);
    //            dsd[tdNo].shmbuf[i++] = (int)(value/256) & 0xff;
    //            dsd[tdNo].shmbuf[i++] = (int)value & 0xff;
    //            len = 9;
    //        }
    //        else if(rxcmd->data.buf[4] == 0x01)//kvar
    //        {
    //            dsd[tdNo].shmbuf[i++] = 0xA2;
    //            dsd[tdNo].shmbuf[i++] = 0x0A;
    //            dsd[tdNo].shmbuf[i++] = 0x00;
    //            dsd[tdNo].shmbuf[i++] = 0x02;
    //            dsd[tdNo].shmbuf[i++] = 0x04;
    //            memcpy(&value,rxcmd->data.buf+6,4);
    //            dsd[tdNo].shmbuf[i++] = (int)(value/(256*256*256)) & 0xff;
    //            dsd[tdNo].shmbuf[i++] = (int)(value/(256*256)) & 0xff;
    //            dsd[tdNo].shmbuf[i++] = (int)(value/256) & 0xff;
    //            dsd[tdNo].shmbuf[i++] = (int)value & 0xff;
    //            len = 11;
    //        }
    //    }
    //    else if(rtupara->type == 2)//36kW
    //    {
    //        if(rxcmd->data.buf[4] == 0x00)//kw
    //        {
    //            dsd[tdNo].shmbuf[i++] = 0x01;
    //            dsd[tdNo].shmbuf[i++] = 0x00;
    //            dsd[tdNo].shmbuf[i++] = 0x00;
    //            dsd[tdNo].shmbuf[i++] = 0x01;
    //            dsd[tdNo].shmbuf[i++] = 0x02;
    //            memcpy(&value,rxcmd->data.buf+6,4);
    //            dsd[tdNo].shmbuf[i++] = (int)(value/256) & 0xff;
    //            dsd[tdNo].shmbuf[i++] = (int)value & 0xff;
    //            len = 9;
    //        }
    //        else if(rxcmd->data.buf[4] == 0x01)//kvar
    //        {
    //            dsd[tdNo].shmbuf[i++] = 0x01;
    //            dsd[tdNo].shmbuf[i++] = 0x02;
    //            dsd[tdNo].shmbuf[i++] = 0x00;
    //            dsd[tdNo].shmbuf[i++] = 0x02;
    //            dsd[tdNo].shmbuf[i++] = 0x04;
    //            memcpy(&value,rxcmd->data.buf+6,4);
    //            dsd[tdNo].shmbuf[i++] = (int)(value/(256*256*256)) & 0xff;
    //            dsd[tdNo].shmbuf[i++] = (int)(value/(256*256)) & 0xff;
    //            dsd[tdNo].shmbuf[i++] = (int)(value/256) & 0xff;
    //            dsd[tdNo].shmbuf[i++] = (int)value & 0xff;
    //            len = 11;
    //        }
    //    }

    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYkExeFrame_pow(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    dsd[tdNo].shmbuf[i++] = 0x00;
    dsd[tdNo].shmbuf[i++] = 0x01;
    if (rxcmd->data.yk[6] == 0x33) {
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x00;
    } else {
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x01;
    }
    BYTE len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}


void CModbus::MakeYkExeFrame_PCS(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int ykno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    if (ykno == 0) {
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x23;
        if (rxcmd->data.yk[6] == 0x33) {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0xAA;
        } else {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x55;
        }
    } else {
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x46;
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x55;
    }

    BYTE len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYkExeFrame_PLC(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int ykno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    if (ykno == 0) {
        dsd[tdNo].shmbuf[i++] = 0x04;
        dsd[tdNo].shmbuf[i++] = 0x00;
    } else if (ykno == 1) {
        dsd[tdNo].shmbuf[i++] = 0xa3;
        dsd[tdNo].shmbuf[i++] = 0x0e;
    } else if (ykno == 2) {
        dsd[tdNo].shmbuf[i++] = 0xe6;
        dsd[tdNo].shmbuf[i++] = 0x15;
    }


    if (rxcmd->data.yk[6] == 0x33) {
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x00;
    } else {
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x01;
    }

    BYTE len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYkExeFrame_BCU(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int ykno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    if (ykno == 4 || ykno == 5) {
        if (ykno == 4) {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x0d;
        } else {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x0e;
        }
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x01;
    } else {
        if (ykno == 0) {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x07;
        } else if (ykno == 1) {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x09;
        } else if (ykno == 2) {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x0b;
        } else if (ykno == 3) {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x0c;
        } else if (ykno > 5) {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = ykno + 9;
        }

        if (rxcmd->data.yk[6] == 0x33) {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x00;
        } else {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x01;
        }
    }

    BYTE len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYkExeFrame_sen1(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    dsd[tdNo].shmbuf[i++] = 0x00;
    dsd[tdNo].shmbuf[i++] = 0x01;
    if (rxcmd->data.yk[6] == 0x33) {
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x00;
    } else {
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x01;
    }
    BYTE len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYkExeFrame_JingLang(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int ykno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    if (ykno == 0) // 开关机
    {
        dsd[tdNo].shmbuf[i++] = 0x0b;
        dsd[tdNo].shmbuf[i++] = 0xbe;
        if (rxcmd->data.yk[6] == 0x33) {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0xde;
        } else {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0xbe;
        }
    } else // 限功率开关
    {
        dsd[tdNo].shmbuf[i++] = 0x0b;
        dsd[tdNo].shmbuf[i++] = 0xfd;
        if (rxcmd->data.yk[6] == 0x33) {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x55;
        } else {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0xaa;
        }
    }
    BYTE len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYkExeFrame_goodwe(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x10;
    if (rtupara->type == 1) // 100kW
    {
        if (rxcmd->data.yk[6] == 0x33) {
            dsd[tdNo].shmbuf[i++] = 0xA1;
            dsd[tdNo].shmbuf[i++] = 0x72;
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x01;
            dsd[tdNo].shmbuf[i++] = 0x02;
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x00;
        } else {
            dsd[tdNo].shmbuf[i++] = 0xA1;
            dsd[tdNo].shmbuf[i++] = 0x72;
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x01;
            dsd[tdNo].shmbuf[i++] = 0x02;
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x01;
        }
    } else if (rtupara->type == 2) // 36kW
    {
        if (rxcmd->data.yk[6] == 0x33) {
            dsd[tdNo].shmbuf[i++] = 0x01;
            dsd[tdNo].shmbuf[i++] = 0x21;
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x01;
            dsd[tdNo].shmbuf[i++] = 0x02;
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x00;
        } else {
            dsd[tdNo].shmbuf[i++] = 0x01;
            dsd[tdNo].shmbuf[i++] = 0x20;
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x01;
            dsd[tdNo].shmbuf[i++] = 0x02;
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x00;
        }
    }
    BYTE len = 9;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYTExeFrame_jscn(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x10;
    int ytno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    if (ytno == 0) {
        dsd[tdNo].shmbuf[i++] = 0x01;
        dsd[tdNo].shmbuf[i++] = 0x01;
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x01;
        dsd[tdNo].shmbuf[i++] = 0x02;
    } else if (ytno == 1) {
        dsd[tdNo].shmbuf[i++] = 0x01;
        dsd[tdNo].shmbuf[i++] = 0x02;
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x01;
        dsd[tdNo].shmbuf[i++] = 0x02;
    } else if (ytno == 2) {
        dsd[tdNo].shmbuf[i++] = 0x01;
        dsd[tdNo].shmbuf[i++] = 0x03;
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x01;
        dsd[tdNo].shmbuf[i++] = 0x02;
    }


    if ((rxcmd->cmdlen - 3) % 3 == 0) // short
    {
        dsd[tdNo].shmbuf[i++] = rxcmd->data.buf[8];
        dsd[tdNo].shmbuf[i++] = rxcmd->data.buf[7];
    } else // float
    {
        float value = 0;
        memcpy(&value, rxcmd->data.buf + 7, 4);
        alog_printf(ALOG_LVL_DEBUG, true, "YT value is %d\n", (int) value);
        alog_printf(ALOG_LVL_DEBUG, true, "YT byte 7 is %d\n", *(rxcmd->data.buf + 7));
        alog_printf(ALOG_LVL_DEBUG, true, "YT byte 8 is %d\n", *(rxcmd->data.buf + 8));
        alog_printf(ALOG_LVL_DEBUG, true, "YT byte 9 is %d\n", *(rxcmd->data.buf + 9));
        alog_printf(ALOG_LVL_DEBUG, true, "YT byte 10 is %d\n", *(rxcmd->data.buf + 10));
        dsd[tdNo].shmbuf[i++] = HIBYTE((short) (value));
        dsd[tdNo].shmbuf[i++] = LOBYTE((short) (value));
    }

    BYTE len = 9;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYkExeFrame_jscn(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x10;
    if (rxcmd->data.yk[6] == 0x33) {
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0xe0;
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x02;
        dsd[tdNo].shmbuf[i++] = 0x04;
        dsd[tdNo].shmbuf[i++] = 0x5a;
        dsd[tdNo].shmbuf[i++] = 0x53;
        dsd[tdNo].shmbuf[i++] = 0x54;
        dsd[tdNo].shmbuf[i++] = 0x50;

    } else {
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0xdc;
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x02;
        dsd[tdNo].shmbuf[i++] = 0x04;
        dsd[tdNo].shmbuf[i++] = 0x5a;
        dsd[tdNo].shmbuf[i++] = 0x52;
        dsd[tdNo].shmbuf[i++] = 0x55;
        dsd[tdNo].shmbuf[i++] = 0x4E;
    }

    BYTE len = 11;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYkExeFrame_sun(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    if (rxcmd->data.yk[4] + rxcmd->data.yk[5] * 256 == 0) {
        dsd[tdNo].shmbuf[i++] = 0x13;
        dsd[tdNo].shmbuf[i++] = 0x8d;
        if (rxcmd->data.yk[6] == 0x33) {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0xce;
        } else {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0xcf;
        }
    } else {
        dsd[tdNo].shmbuf[i++] = 0x13;
        dsd[tdNo].shmbuf[i++] = 0x8e;
        if (rxcmd->data.yk[6] == 0x33) {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x55;
        } else {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0xAA;
        }
    }

    BYTE len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

unsigned char CModbus::FindModbusTypeByRtu(RTUPARA *rtupara) {
    unsigned char type = 0;
    for (int i = 0; i < pmod->ordernum; i++) {
        if (rtupara->type == (pmod + i)->rtuaddr && (pmod + i)->type > 19) {
            type = (pmod + i)->type;
            break;
        }
    }
    return type;
}

void CModbus::MakeYTExeFrame_JingLang(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    float value = 0;
    BYTE len = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    dsd[tdNo].shmbuf[i++] = 0x0b;
    dsd[tdNo].shmbuf[i++] = 0xeb;
    memcpy(&value, rxcmd->data.buf + 7, 4);
    value = value * 100;
    dsd[tdNo].shmbuf[i++] = HIBYTE((short) (value));
    dsd[tdNo].shmbuf[i++] = LOBYTE((short) (value));
    len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYTExeFrame_huawei(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int ykno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    int i = 0;
    float value = 0;
    BYTE len = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    if (ykno == 0) // 逆变输出功率设置(kW)
    {
        dsd[tdNo].shmbuf[i++] = 0x9C;
        dsd[tdNo].shmbuf[i++] = 0xB8;
        memcpy(&value, rxcmd->data.buf + 7, 4);
        value = value * 10;
        dsd[tdNo].shmbuf[i++] = HIBYTE((short) (value));
        dsd[tdNo].shmbuf[i++] = LOBYTE((short) (value));
    } else if (ykno == 1) // 逆变输出功率设置(%)
    {
        dsd[tdNo].shmbuf[i++] = 0x9C;
        dsd[tdNo].shmbuf[i++] = 0xBD;
        memcpy(&value, rxcmd->data.buf + 7, 4);
        value = value * 10;
        dsd[tdNo].shmbuf[i++] = HIBYTE((short) (value));
        dsd[tdNo].shmbuf[i++] = LOBYTE((short) (value));
    }
    len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYkExeFrame_huawei(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    if (rxcmd->data.yk[6] == 0x33) {
        dsd[tdNo].shmbuf[i++] = 0x9D;
        dsd[tdNo].shmbuf[i++] = 0x09;
    } else {
        dsd[tdNo].shmbuf[i++] = 0x9D;
        dsd[tdNo].shmbuf[i++] = 0x08;
    }

    dsd[tdNo].shmbuf[i++] = 0x00;
    dsd[tdNo].shmbuf[i++] = 0x01;

    BYTE len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYkExeFrame(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    unsigned char modbus_type = FindModbusTypeByRtu(rtupara);
    switch (modbus_type) {
        case 22:
            MakeYkExeFrame_goodwe_MT(tdNo, rtupara, rxcmd);
            break;
        case 33:
            MakeYkExeFrame_goodwe_HT(tdNo, rtupara, rxcmd);
            break;
        case 34:
            MakeYkExeFrame_huawei(tdNo, rtupara, rxcmd);
            break;
        case 35:
            MakeYkExeFrame_sun(tdNo, rtupara, rxcmd);
            break;
        case 37:
            MakeYkExeFrame_sen1(tdNo, rtupara, rxcmd);
            break;
        case 40:
            MakeYkExeFrame_pow(tdNo, rtupara, rxcmd);
            break;
        case 44:
            MakeYkExeFrame_JingLang(tdNo, rtupara, rxcmd);
            break;
        case 47:
            MakeYkExeFrame_BCU(tdNo, rtupara, rxcmd);
            break;
        case 49:
            MakeYkExeFrame_PCS(tdNo, rtupara, rxcmd);
            break;
        case 52:
            MakeYkExeFrame_PLC(tdNo, rtupara, rxcmd);
            break;
        default:
            break;
    }
}

void CModbus::MakeYkExeFrame_goodwe_MT(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x10;
    if (rxcmd->data.yk[6] == 0x33) {
        dsd[tdNo].shmbuf[i++] = 0x01;
        dsd[tdNo].shmbuf[i++] = 0x21;
    } else {
        dsd[tdNo].shmbuf[i++] = 0x01;
        dsd[tdNo].shmbuf[i++] = 0x20;
    }

    dsd[tdNo].shmbuf[i++] = 0x00;
    dsd[tdNo].shmbuf[i++] = 0x01;
    dsd[tdNo].shmbuf[i++] = 0x02;
    dsd[tdNo].shmbuf[i++] = 0x00;
    dsd[tdNo].shmbuf[i++] = 0x00;

    BYTE len = 9;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYkExeFrame_goodwe_HT(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x10;
    dsd[tdNo].shmbuf[i++] = 0xA1;
    dsd[tdNo].shmbuf[i++] = 0x72;
    dsd[tdNo].shmbuf[i++] = 0x00;
    dsd[tdNo].shmbuf[i++] = 0x01;
    dsd[tdNo].shmbuf[i++] = 0x02;

    if (rxcmd->data.yk[6] == 0x33) {
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x00;
    } else {
        dsd[tdNo].shmbuf[i++] = 0x00;
        dsd[tdNo].shmbuf[i++] = 0x01;
    }

    BYTE len = 9;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYkExeFrame_old(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    if (rtupara->type == 1) {
        dsd[tdNo].shmbuf[i++] = 0x05;
        dsd[tdNo].shmbuf[i++] = rxcmd->data.yk[4];
        dsd[tdNo].shmbuf[i++] = rxcmd->data.yk[5];
        if (rxcmd->data.yk[6] == 0x33) {
            dsd[tdNo].shmbuf[i++] = 0x00;
            dsd[tdNo].shmbuf[i++] = 0x00;
        } else {
            dsd[tdNo].shmbuf[i++] = 0xff;
            dsd[tdNo].shmbuf[i++] = 0x00;
        }
    } else {
        dsd[tdNo].shmbuf[i++] = 0x06;
        if (rxcmd->data.yk[4] + rxcmd->data.yk[5] * 256 == 0) {
            dsd[tdNo].shmbuf[i++] = 0x13;
            dsd[tdNo].shmbuf[i++] = 0x8d;
            if (rxcmd->data.yk[6] == 0x33) {
                dsd[tdNo].shmbuf[i++] = 0x00;
                dsd[tdNo].shmbuf[i++] = 0xce;
            } else {
                dsd[tdNo].shmbuf[i++] = 0x00;
                dsd[tdNo].shmbuf[i++] = 0xcf;
            }
        } else {
            dsd[tdNo].shmbuf[i++] = 0x13;
            dsd[tdNo].shmbuf[i++] = 0xAA;
            if (rxcmd->data.yk[6] == 0x33) {
                dsd[tdNo].shmbuf[i++] = 0x00;
                dsd[tdNo].shmbuf[i++] = 0x55;
            } else {
                dsd[tdNo].shmbuf[i++] = 0x00;
                dsd[tdNo].shmbuf[i++] = 0xAA;
            }
        }
    }

    BYTE len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}


void CModbus::MakeYTExeFrame(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    unsigned char modbus_type = FindModbusTypeByRtu(rtupara);
    switch (modbus_type) {
        case 22:
            MakeYTExeFrame_goodwe_MT(tdNo, rtupara, rxcmd);
            break;
        case 33:
            MakeYTExeFrame_goodwe_HT(tdNo, rtupara, rxcmd);
            break;
        case 34:
            MakeYTExeFrame_huawei(tdNo, rtupara, rxcmd);
            break;
        case 35:
            MakeYTExeFrame_sun(tdNo, rtupara, rxcmd);
            break;
        case 37:
            MakeYTExeFrame_sen1(tdNo, rtupara, rxcmd);
            break;
        case 38:
            MakeYTExeFrame_sen2(tdNo, rtupara, rxcmd);
            break;
        case 39:
            MakeYTExeFrame_tem(tdNo, rtupara, rxcmd);
            break;
        case 40:
            MakeYTExeFrame_pow(tdNo, rtupara, rxcmd);
            break;
        case 44:
            MakeYTExeFrame_JingLang(tdNo, rtupara, rxcmd);
            break;
        case 47:
            MakeYTExeFrame_BCU(tdNo, rtupara, rxcmd);
            break;
        case 49:
            MakeYTExeFrame_PCS(tdNo, rtupara, rxcmd);
            break;
        case 52:
            MakeYTExeFrame_PLC(tdNo, rtupara, rxcmd);
            break;
        case 53:
            MakeYTExeFrame_AMC(tdNo, rtupara, rxcmd);
            break;
        default:
            break;
    }
}

void CModbus::MakeYTExeFrame_sun(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    int ytno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    if (ytno == 0) {
        dsd[tdNo].shmbuf[i++] = 0x13;
        dsd[tdNo].shmbuf[i++] = 0xae;
    }

    if ((rxcmd->cmdlen - 3) % 3 == 0) // short
    {
        dsd[tdNo].shmbuf[i++] = rxcmd->data.buf[8];
        dsd[tdNo].shmbuf[i++] = rxcmd->data.buf[7];
    } else // float
    {
        float value = 0;
        memcpy(&value, rxcmd->data.buf + 7, 4);
        dsd[tdNo].shmbuf[i++] = HIBYTE((short) (value * 10));
        dsd[tdNo].shmbuf[i++] = LOBYTE((short) (value * 10));
    }

    BYTE len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
}

void CModbus::MakeYTExeFrame_old(int tdNo, RTUPARA *rtupara, RXCOMMAND *rxcmd) {
    int i = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x06;
    int ytno = rxcmd->data.buf[4] + rxcmd->data.buf[5] * 256;
    if (rtupara->type == 1) // nr
    {
        dsd[tdNo].shmbuf[i++] = 0x03;
        dsd[tdNo].shmbuf[i++] = 0xe8;
    } else // sun
    {
        if (ytno % 2 == 0) // 有功
        {
            dsd[tdNo].shmbuf[i++] = 0x13;
            dsd[tdNo].shmbuf[i++] = 0xae;
        } else // 无功
        {
            dsd[tdNo].shmbuf[i++] = 0x13;
            dsd[tdNo].shmbuf[i++] = 0xaf;
        }
    }

    if ((rxcmd->cmdlen - 3) % 3 == 0) // short
    {
        dsd[tdNo].shmbuf[i++] = rxcmd->data.buf[8];
        dsd[tdNo].shmbuf[i++] = rxcmd->data.buf[7];
    } else // float
    {
        float value = 0;
        memcpy(&value, rxcmd->data.buf + 7, 4);
        dsd[tdNo].shmbuf[i++] = HIBYTE((short) (value * 10));
        dsd[tdNo].shmbuf[i++] = LOBYTE((short) (value * 10));
    }

    BYTE len = 6;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    dsd[tdNo].commFlag = 1;
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);

    //        shmbuf[tdNo][0] = (unsigned char)(rtupara->rtuAddr);
    //        shmbuf[tdNo][1] = 0x06;
    //        unsigned short ykno = rxcmd->data.yk[4]*256+rxcmd->data.yk[5];
    //            shmbuf[tdNo][2] = rxcmd->data.yk[4];
    //            shmbuf[tdNo][3] = rxcmd->data.yk[5];
    //       shmbuf[tdNo][4] = rxcmd->data.yk[6];
    //        shmbuf[tdNo][5] = rxcmd->data.yk[7];

    //        BYTE len = 6;
    //        unsigned short crc = CRC16(shmbuf[tdNo],len);
    //        shmbuf[tdNo][len] = crc%256;
    //        shmbuf[tdNo][len+1] = crc/256;
    //        for(int i=0;i<len+2;i++)
    //        {
    //                BYTE val = shmbuf[tdNo][i];
    //                EnterTrnQ(tdNo,val);
    //        }
    //        dsd[tdNo].commFlag = 1;
    //                ShowData( tdNo, 1/*send*/, shmbuf[tdNo], len+2, 0/*data*/ );
}

void CModbus::MakeFileExeFrame_BCU(int tdNo, RTUPARA *rtupara) {
    int i = 0, len = 0;
    dsd[tdNo].shmbuf[i++] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[i++] = 0x15;
    dsd[tdNo].shmbuf[i++] = 0x00;
    dsd[tdNo].shmbuf[i++] = 0x06;
    dsd[tdNo].shmbuf[i++] = HIBYTE(fileType);
    dsd[tdNo].shmbuf[i++] = LOBYTE(fileType);
    dsd[tdNo].shmbuf[i++] = HIBYTE(fileGroupNo);
    dsd[tdNo].shmbuf[i++] = LOBYTE(fileGroupNo);

    if ((fileGroupNo + 1) != fileGroupNum) {
        len = MAX_FILE_RECORDNUM;
    } else {
        if ((fileReadNum - fileGroupNo * MAX_FILE_RECORDNUM * 2) % 2 == 0)
            len = (fileReadNum - fileGroupNo * MAX_FILE_RECORDNUM * 2) / 2;
        else
            len = (fileReadNum - fileGroupNo * MAX_FILE_RECORDNUM * 2) / 2 + 1;
    }
    dsd[tdNo].shmbuf[i++] = HIBYTE(len);
    dsd[tdNo].shmbuf[i++] = LOBYTE(len);
    dsd[tdNo].shmbuf[2] = len * 2 + 9;
    for (int j = 0; j < len * 2; j++) {
        dsd[tdNo].shmbuf[i++] = filebuf[fileReadNum - fileGroupNo * MAX_FILE_RECORDNUM * 2 + j];
    }

    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len * 2 + 9);
    dsd[tdNo].shmbuf[i++] = crc % 256;
    dsd[tdNo].shmbuf[i++] = crc / 256;
    for (int k = 0; k < len * 2 + 9 + 2; k++) {
        BYTE val = dsd[tdNo].shmbuf[k];
        EnterTrnQ(tdNo, val);
    }
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len * 2 + 9 + 2, 0 /*data*/);
    dsd[tdNo].commFlag = 1;

    float rate = (((float) fileGroupNo + 1) / ((float) fileGroupNum)) * 100;
    SetYcValue_float(rtupara->rtuNo, 1226, (BYTE *) (&rate), 1);

    fileGroupNo++;
    if (fileGroupNo == fileGroupNum) {
        fileFlag = 0;
        SetYcValue_short(rtupara->rtuNo, 1225, (BYTE *) &fileFlag, 1);
        free(filebuf);
        filebuf = NULL;
        fileFlag = 0;
        fileType = 0;
        fileGroupNo = 0;
        fileGroupNum = 0;
        fileReadNum = 0;
    }
}

void CModbus::MakeSetTimeFrame(int tdNo, RTUPARA *rtupara, WORD Addr) {
    dsd[tdNo].shmbuf[0] = (unsigned char) (rtupara->rtuAddr);
    dsd[tdNo].shmbuf[1] = 0x10;
    dsd[tdNo].shmbuf[2] = Addr / 256;
    dsd[tdNo].shmbuf[3] = Addr % 256;
    dsd[tdNo].shmbuf[4] = 0;
    dsd[tdNo].shmbuf[5] = 6;
    dsd[tdNo].shmbuf[6] = 12;
    FETIME st;
    getTime(&st);
    dsd[tdNo].shmbuf[7] = (st.aYear + 1900) / 256;
    dsd[tdNo].shmbuf[8] = (st.aYear + 1900) % 256;
    dsd[tdNo].shmbuf[9] = st.aMon / 256;
    dsd[tdNo].shmbuf[10] = st.aMon % 256;
    dsd[tdNo].shmbuf[11] = st.aDay / 256;
    dsd[tdNo].shmbuf[12] = st.aDay % 256;
    dsd[tdNo].shmbuf[13] = st.aHour / 256;
    dsd[tdNo].shmbuf[14] = st.aHour % 256;
    dsd[tdNo].shmbuf[15] = st.aMin / 256;
    dsd[tdNo].shmbuf[16] = st.aMin % 256;
    dsd[tdNo].shmbuf[17] = st.aSec / 256;
    dsd[tdNo].shmbuf[18] = st.aSec % 256;


    BYTE len = 19;
    unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
    dsd[tdNo].shmbuf[len] = crc % 256;
    dsd[tdNo].shmbuf[len + 1] = crc / 256;
    for (int i = 0; i < len + 2; i++) {
        BYTE val = dsd[tdNo].shmbuf[i];
        EnterTrnQ(tdNo, val);
    }
    ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
    dsd[tdNo].commFlag = 1;
}
// void CModbus::MakeSetWriteFrame(int tdNo,RTUPARA *rtupara,WORD Addr)
//{
//         shmbuf[tdNo][0] = (unsigned char)(rtupara->rtuAddr);
//	shmbuf[tdNo][1] = 0x10;
//	shmbuf[tdNo][2] = Addr/256;
//	shmbuf[tdNo][3] = Addr%256;
//	shmbuf[tdNo][4] = 0;
//	shmbuf[tdNo][5] = 1;
//	shmbuf[tdNo][6] = 2;
//	shmbuf[tdNo][7] = 0;
//	shmbuf[tdNo][8] = 2;

//	BYTE len = 9;
//		unsigned short crc = CRC16(shmbuf[tdNo],len);
//		shmbuf[tdNo][len] = crc%256;
//		shmbuf[tdNo][len+1] = crc/256;
//		for(int i=0;i<len+2;i++)
//		{
//			BYTE val = shmbuf[tdNo][i];
//			EnterTrnQ(tdNo,val);
//		}
////		ShowData( tdNo, 1/*send*/, shmbuf[tdNo], len+2, 0/*data*/ );
//}
void CModbus::EditCallDataFrm(int tdNo, RTUPARA *rtupara) {
    int modbusrcnt = 0;
    if (pmod)
        modbusrcnt = pmod->ordernum;
    else
        return;
    if (rtudata[rtupara->rtuNo].txPtr >= modbusrcnt) {
        rtudata[rtupara->rtuNo].txPtr = 0;
        // chanloop[tdNo]++;
        rtuloop[rtupara->rtuNo]++;
    }
    int i;
    for (i = rtudata[rtupara->rtuNo].txPtr; i < modbusrcnt; i++) {
        MODBUS *modbus = pmod + i;
        if ((modbus->rtuaddr != rtupara->type) || (rtuloop[rtupara->rtuNo] % modbus->freshtime) != 0)
            continue;
        if (rtupara->rtuNo == 1 && fileFlag == 1) // bcu通道开始升级程序，采集停止召唤
        {
            MakeFileExeFrame_BCU(tdNo, rtupara);
        } else {
            dsd[tdNo].shmbuf[0] = rtupara->rtuAddr;
            dsd[tdNo].shmbuf[1] = modbus->order;
            dsd[tdNo].shmbuf[2] = modbus->startaddr / 256;
            dsd[tdNo].shmbuf[3] = modbus->startaddr % 256;
            dsd[tdNo].shmbuf[4] = modbus->length / 256;
            dsd[tdNo].shmbuf[5] = modbus->length % 256;
            int len = 6;
            unsigned short crc = CRC16(dsd[tdNo].shmbuf, len);
            dsd[tdNo].shmbuf[len] = crc % 256;
            dsd[tdNo].shmbuf[len + 1] = crc / 256;
            for (int j = 0; j < len + 2; j++) {
                BYTE val = dsd[tdNo].shmbuf[j];
                EnterTrnQ(tdNo, val);
            }
            dsd[tdNo].txCmd = modbus->type;
            dsd[tdNo].commFlag = 1;
            ShowData(tdNo, 1 /*send*/, dsd[tdNo].shmbuf, len + 2, 0 /*data*/);
        }
        rtudata[rtupara->rtuNo].txPtr = i;
        break;
    }
    if (i >= modbusrcnt) {
        rtudata[rtupara->rtuNo].txPtr = 0;
        // chanloop[tdNo]++;
        rtuloop[rtupara->rtuNo]++;
        ChangeRtuNo(tdNo);
    }
}

/*
int CModbus::procdata_20(int rtuno, int pModeNo,unsigned char cmdtype,int datalen,unsigned char *buf)
{
    if(pModeNo < 0) return 0;

    short startAddr = (pmod+rtudata[rtuno].txPtr)->startaddr;
    short endAddr = startAddr + MIN(datalen/2,(pmod+rtudata[rtuno].txPtr)->length);
    int i,j,k,ycNum = 0,yxNum = 0,kwhNum = 0;
    for(i=0;i<(pmode+pModeNo)->procNum;i++)
    {
        if((pmode+pModeNo)->modbusproc[i].order == cmdtype)
        {
            if((pmode+pModeNo)->modbusproc[i].regAddr < endAddr)
            {
                if((pmode+pModeNo)->modbusproc[i].regAddr >= startAddr)
                {
                    WORD tempword = 0;
                    unsigned char tempchar = 0x00;
                    if((pmode+pModeNo)->modbusproc[i].dataType == 0x00)//Bit
                    {
                        k = ((pmode+pModeNo)->modbusproc[i].regAddr-startAddr)*2;
                        for(j=0;j<(pmode+pModeNo)->modbusproc[i].bitNum;j++)
                        {
                            if(j%16==0)
                            {
                                if((pmode+pModeNo)->modbusproc[i].byteOrder == 0x00)
                                    tempword = (*(buf + k))*256 + *(buf + k + 1);
                                else
                                    tempword = *(buf + k) + (*(buf + k + 1))*256;
                                k+=2;
                            }
                            tempchar = tempword>>((pmode+pModeNo)->modbusproc[i].bitOffset+j%16) & 0x01;
                            SetSingleYxValue(rtuno,yxNum+j,tempchar,0);

                        }
                        yxNum+=(pmode+pModeNo)->modbusproc[i].bitNum;
                    }
                    else if((pmode+pModeNo)->modbusproc[i].dataType == 0x01)//Bits
                    {
                        k = ((pmode+pModeNo)->modbusproc[i].regAddr-startAddr)*2;
                        if((pmode+pModeNo)->modbusproc[i].byteOrder == 0x00)
                            tempword = (*(buf + k))*256 + *(buf + k + 1);
                        else
                            tempword = *(buf + k) + (*(buf + k + 1))*256;
                        tempword = tempword>>((pmode+pModeNo)->modbusproc[i].bitOffset) &
((0x01<<((pmode+pModeNo)->modbusproc[i].bitNum)) - 1); SetYcValue_short(rtuno,ycNum,(BYTE *)&tempword,1); ycNum+=1;
                    }
                    else if((pmode+pModeNo)->modbusproc[i].dataType == 0x02)//UINT16
                    {
                        k = ((pmode+pModeNo)->modbusproc[i].regAddr-startAddr)*2;
                        for(j=0;j<(pmode+pModeNo)->modbusproc[i].regNum;j++)
                        {
                            int ycval = 0;
                            if((pmode+pModeNo)->modbusproc[i].byteOrder)//小端
                            {
                                ycval = *(buf+k) + *(buf+k+1)*256;
                            }
                            else//大端
                            {
                                ycval = *(buf+k)*256 + *(buf+k+1);
                            }
                            SetYcValue_int(rtuno,ycNum,(BYTE *)&ycval,1);
                            k+=2;
                            ycNum++;
                        }
                    }
                    else if((pmode+pModeNo)->modbusproc[i].dataType == 0x03)//SINT16
                    {
                        k = ((pmode+pModeNo)->modbusproc[i].regAddr-startAddr)*2;
                        for(j=0;j<(pmode+pModeNo)->modbusproc[i].regNum;j++)
                        {
                            short ycval = 0;
                            if((pmode+pModeNo)->modbusproc[i].byteOrder)//小端
                            {
                                ycval = *(buf+k) + *(buf+k+1)*256;
                            }
                            else//大端
                            {
                                ycval = *(buf+k)*256 + *(buf+k+1);
                            }
                            SetYcValue_short(rtuno,ycNum,(BYTE *)&ycval,1);
                            k+=2;
                            ycNum++;
                        }
                    }
                    else if((pmode+pModeNo)->modbusproc[i].dataType == 0x04 || (pmode+pModeNo)->modbusproc[i].dataType
== 0x05)//UINT32 SINT32
                    {
                        k = ((pmode+pModeNo)->modbusproc[i].regAddr-startAddr)*2;
                        for(j=0;j<(pmode+pModeNo)->modbusproc[i].regNum/2;j++)
                        {
                            int ycval = 0;
                            if((pmode+pModeNo)->modbusproc[i].wordOrder)//小端
                            {
                                if((pmode+pModeNo)->modbusproc[i].byteOrder)//小端 0123
                                {
                                    ycval = *(buf+k) + *(buf+k+1)*256 + *(buf+k+2)*256*256 + *(buf+k+3)*256*256*256;
                                }
                                else//大端 1032
                                {
                                    ycval = *(buf+k)*256 + *(buf+k+1) + *(buf+k+2)*256*256*256 + *(buf+k+3)*256*256;
                                }
                            }
                            else
                            {
                                if((pmode+pModeNo)->modbusproc[i].byteOrder)//小端 2301
                                {
                                    ycval = *(buf+k)*256*256 + *(buf+k+1)*256*256*256 + *(buf+k+2) + *(buf+k+3)*256;
                                }
                                else//大端3210
                                {
                                    ycval = *(buf+k)*256*256*256 + *(buf+k+1)*256*256 + *(buf+k+2)*256 + *(buf+k+3);
                                }
                            }
                            SetYcValue_int(rtuno,ycNum,(BYTE *)&ycval,1);
                            k+=4;
                            ycNum++;
                        }
                    }
                    else if((pmode+pModeNo)->modbusproc[i].dataType == 0x06)//FLOAT
                    {
                        k = ((pmode+pModeNo)->modbusproc[i].regAddr-startAddr)*2;
                        for(j=0;j<(pmode+pModeNo)->modbusproc[i].regNum/2;j++)
                        {
                            float ycval_f = 0;int ycval = 0;
                            if((pmode+pModeNo)->modbusproc[i].wordOrder)//小端
                            {
                                if((pmode+pModeNo)->modbusproc[i].byteOrder)//小端 0123
                                {
                                    ycval = *(buf+k) + *(buf+k+1)*256 + *(buf+k+2)*256*256 + *(buf+k+3)*256*256*256;
                                }
                                else//大端 1032
                                {
                                    ycval = *(buf+k)*256 + *(buf+k+1) + *(buf+k+2)*256*256*256 + *(buf+k+3)*256*256;
                                }
                            }
                            else
                            {
                                if((pmode+pModeNo)->modbusproc[i].byteOrder)//小端 2301
                                {
                                    ycval = *(buf+k)*256*256 + *(buf+k+1)*256*256*256 + *(buf+k+2) + *(buf+k+3)*256;
                                }
                                else//大端3210
                                {
                                    ycval = *(buf+k)*256*256*256 + *(buf+k+1)*256*256 + *(buf+k+2)*256 + *(buf+k+3);
                                }
                            }
                            ycval_f = *((float *)&ycval);
                            SetYcValue_float(rtuno,ycNum,(BYTE *)&ycval_f,1);
                            k+=4;
                            ycNum++;
                        }
                    }
                    else if((pmode+pModeNo)->modbusproc[i].dataType == 0x07)//kwh
                    {
                        k = ((pmode+pModeNo)->modbusproc[i].regAddr-startAddr)*2;
                        for(j=0;j<(pmode+pModeNo)->modbusproc[i].regNum/2;j++)
                        {
                            unsigned int kwhval = 0;
                            if((pmode+pModeNo)->modbusproc[i].wordOrder)//小端
                            {
                                if((pmode+pModeNo)->modbusproc[i].byteOrder)//小端 0123
                                {
                                    kwhval = *(buf+k) + *(buf+k+1)*256 + *(buf+k+2)*256*256 + *(buf+k+3)*256*256*256;
                                }
                                else//大端 1032
                                {
                                    kwhval = *(buf+k)*256 + *(buf+k+1) + *(buf+k+2)*256*256*256 + *(buf+k+3)*256*256;
                                }
                            }
                            else
                            {
                                if((pmode+pModeNo)->modbusproc[i].byteOrder)//小端 2301
                                {
                                    kwhval = *(buf+k)*256*256 + *(buf+k+1)*256*256*256 + *(buf+k+2) + *(buf+k+3)*256;
                                }
                                else//大端3210
                                {
                                    kwhval = *(buf+k)*256*256*256 + *(buf+k+1)*256*256 + *(buf+k+2)*256 + *(buf+k+3);
                                }
                            }
                            SetKwhValue(rtuno,kwhNum,kwhval);
                            k+=4;
                            kwhNum++;
                        }
                    }
                }
                else
                {
                    if((pmode+pModeNo)->modbusproc[i].dataType == 0x00)
                    {
                        yxNum += (pmode+pModeNo)->modbusproc[i].bitNum;
                    }
                    else if((pmode+pModeNo)->modbusproc[i].dataType == 0x01 || (pmode+pModeNo)->modbusproc[i].dataType
== 0x02 || (pmode+pModeNo)->modbusproc[i].dataType == 0x03)
                    {
                        ycNum +=(pmode+pModeNo)->modbusproc[i].regNum;
                    }
                    else if((pmode+pModeNo)->modbusproc[i].dataType == 0x04 || (pmode+pModeNo)->modbusproc[i].dataType
== 0x05 || (pmode+pModeNo)->modbusproc[i].dataType == 0x06)
                    {
                        ycNum +=(pmode+pModeNo)->modbusproc[i].regNum/2;
                    }
                    else
                    {
                        kwhNum +=(pmode+pModeNo)->modbusproc[i].regNum/2;
                    }
                }
            }
            else
            {
                break;
            }
        }
        else
        {
            if((pmode+pModeNo)->modbusproc[i].dataType == 0x00)
            {
                yxNum += (pmode+pModeNo)->modbusproc[i].bitNum;
            }
            else if((pmode+pModeNo)->modbusproc[i].dataType == 0x01 || (pmode+pModeNo)->modbusproc[i].dataType == 0x02
|| (pmode+pModeNo)->modbusproc[i].dataType == 0x03)
            {
                ycNum +=(pmode+pModeNo)->modbusproc[i].regNum;
            }
            else if((pmode+pModeNo)->modbusproc[i].dataType == 0x04 || (pmode+pModeNo)->modbusproc[i].dataType == 0x05
|| (pmode+pModeNo)->modbusproc[i].dataType == 0x06)
            {
                ycNum +=(pmode+pModeNo)->modbusproc[i].regNum/2;
            }
            else
            {
                kwhNum +=(pmode+pModeNo)->modbusproc[i].regNum/2;
            }
        }

    }
    return 1;
}
*/

unsigned short CModbus::CRC16(unsigned char *Array, unsigned int Len) {
    unsigned short IX, IY, CRC;

    CRC = 0xFFFF;
    if (Len == 0)
        return CRC;
    unsigned char Temp;
    Len--;
    for (IX = 0; IX <= Len; IX++) {
        Temp = LOBYTE(CRC) ^ Array[IX];
        CRC = (HIBYTE(CRC) * 256) + Temp;
        for (IY = 0; IY <= 7; IY++)
            if ((CRC & 1) != 0)
                CRC = (CRC >> 1) ^ 0xA001;
            else
                CRC = CRC >> 1;
    };
    return CRC;
}


int CModbus::getoffset(int cur_id, int type, int rtutype) {
    /*
0://遥信0..7-8..15
1://遥测四字节整数(低字前1032)
2://遥信8..15-0..7
3://二字节有符号遥测帧(10)
4://二字节有符号遥测帧(10)高位符号其余绝对值
5://二字节无符号遥测帧(10)
7://遥测四字节整数(高字前3210)反序
8://遥测四字节浮点(高字前3210)反序
9://遥测四字节整数(低字前0123)正序
10://遥测四字节浮点(低字前0123)正序
11://电度帧整数(0123正序)
12://电度帧整数(3210反序)
13://电度帧整数(低字前1032)
14://电度帧浮点(0123正序)
15://电度帧浮点(3210反序)
16://电度帧浮点(1032低字前)
23://二字节有符号遥测帧(01)
24://二字节有符号遥测帧(01)高位符号其余绝对值
25://二字节无符号遥测帧(01)
*/

    MODBUS *modbus = pmod;
    int num = 0;
    int i;

    for (int i = 0; i < cur_id; i++, modbus++) {
        if (rtutype != modbus->rtuaddr)
            continue;
        switch (modbus->type) {

            case 0: // 遥信0..7-8..15
            case 2: // 遥信8..15-0..7
                if (type == 0)
                    num += modbus->length;
                continue;
                break;
            case 1: // 遥测四字节整数(低字前1032)
            case 7: // 遥测四字节整数(高字前3210)反序
            case 8: // 遥测四字节浮点(高字前3210)反序
            case 9: // 遥测四字节整数(低字前0123)正序
            case 10: // 遥测四字节浮点(低字前0123)正序
                if (type == 1)
                    num += modbus->length / 2;
                break;
            case 3: // 二字节有符号遥测帧(10)
            case 4: // 二字节有符号遥测帧(10)高位符号其余绝对值
            case 5: // 二字节无符号遥测帧(10)
            case 23: // 二字节有符号遥测帧(01)
            case 24: // 二字节有符号遥测帧(01)高位符号其余绝对值
            case 25: // 二字节无符号遥测帧(01)
                if (type == 1)
                    num += modbus->length;
                break;
            case 11: // 电度帧整数(0123正序)
            case 12: // 电度帧整数(3210反序)
            case 13: // 电度帧整数(低字前1032)
            case 14: // 电度帧浮点(0123正序)
            case 15: // 电度帧浮点(3210反序)
            case 16: // 电度帧浮点(1032低字前)
                if (type == 2)
                    num += modbus->length / 2;
                break;
        }
    }
    return num;
}
