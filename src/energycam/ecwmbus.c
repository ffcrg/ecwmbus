#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <conio.h>

#include <time.h>
#include <energycam/ecwmbus.h>
#include <energycam/wmbusext.h>

//Log Reading with date info to CSV File
int Log2CSVFile(const char *path,  double Value) {
    FILE    *hFile;
    uint32_t FileSize = 0;
    
    time_t t = time(NULL);
    struct tm tm;
    localtime_s(&tm,&t);

    if (fopen_s(&hFile, path, "rb") == NULL) {
        fseek(hFile, 0L, SEEK_END);
        FileSize = ftell(hFile);
        fseek(hFile, 0L, SEEK_SET);
        fclose(hFile);
    }

    if (fopen_s(&hFile, path, "a") == NULL) {
        if (FileSize == 0)  //start a new file with Header
            fprintf(hFile, "Date, Value \n");
        fprintf(hFile, "%d-%02d-%02d %02d:%02d, %.1f\r\n", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, Value);
        fclose(hFile);
    } else
        return APIERROR;

    return APIOK;
}

static int iTime;
static int iMinute;

int IsNewSecond(int iS) {
    int CurTime;
    time_t t = time(NULL);
    struct tm tm;
    localtime_s(&tm,&t);
    CurTime = tm.tm_hour*60*60+tm.tm_min*60+tm.tm_sec;
    if (iS > 0)
        CurTime = CurTime/iS;

    if (CurTime != iTime) {
        iTime = CurTime;
        return 1;
    }
    return 0;
}

int IsNewMinute(void) {
    int CurTime;
    time_t t = time(NULL);
    struct tm tm;
    localtime_s(&tm, &t);
    CurTime = tm.tm_hour*60+tm.tm_min;

    if (CurTime != iMinute) {
        iMinute = CurTime;
        return 1;
    }
    return 0;
}

void Intro(void) {
    printf("   \n");
    printf("############################\n");
    printf("## ecwmbus                ##\n");
    printf("############################\n");
    printf("   Usage\n");
    printf("   s   : Use S2 mode\n");
    printf("   t   : Use T2 mode\n");
    printf("   a   : Add meter\n");
    printf("   r   : Remove meter\n");
    printf("   l   : List meters\n");
    printf("   u   : Update - check for data\n");
    printf("   q   : Quit\n");
}

void IntroShowParam(void) {
    printf("   \n");
    printf("############################\n");
    printf("## ecwmbus                ##\n");
    printf("############################\n");

    printf("   Commandline options:\n");
    printf("   ecwmbus /c:3 /m:T /i\n");
    printf("   /c:3 : COM3\n");
    printf("   /m:T : T2 mode \n");
    printf("   /i    : show detailed infos \n\n");
}

void ErrorAndExit(const char *info) {
    printf("%s", info);
    exit(0);
}

unsigned int CalcUIntBCD(unsigned int ident) {
    int32_t identNumBCD=0;
    #define MAXIDENTLEN 12
    uint8_t  identchar[MAXIDENTLEN];
    memset(identchar,0,MAXIDENTLEN*sizeof(uint8_t));
    sprintf_s((char *)identchar ,MAXIDENTLEN, "%08d", ident);
    uint32_t uiMul=1;
    uint8_t uiX=0;
    uint8_t uiLen= (uint8_t)strlen((char const*)identchar);

    for(uiX=0; uiX < uiLen;uiX++) {
        identNumBCD += (identchar[uiLen-1-uiX] - '0')*uiMul;
        uiMul = uiMul*16;
    }
    return identNumBCD;
}

void DisplayListofMeters(int iMax, pecwMBUSMeter ecpiwwMeter) {
    int iX,iI;

    if(iMax == 0) printf("\nNo Meters defined.\n");
    else {
        iI=0;
        for(iX=0; iX<iMax; iX++) {
            if( 0 != ecpiwwMeter[iX].manufacturerID)
               iI++;
        }
        printf("\nList of active Meters (%d defined):\n", iI);
     }

    for(iX=0;iX<iMax;iX++) {
        if( 0 != ecpiwwMeter[iX].manufacturerID) {
            printf("Meter#%d : Manufactor = 0x%02X \r\n", iX+1, ecpiwwMeter[iX].manufacturerID);
            printf("Meter#%d : Ident      = 0x%08X \r\n", iX+1, ecpiwwMeter[iX].ident);
            printf("Meter#%d : Type       = 0x%02X \r\n", iX+1, ecpiwwMeter[iX].type);
            printf("Meter#%d : Version    = 0x%02X \r\n", iX+1, ecpiwwMeter[iX].version);
            printf("Meter#%d : Key        = 0x", iX+1);
            for(iI = 0; iI<AES_KEYLENGHT_IN_BYTES; iI++)
                printf("%02X",ecpiwwMeter[iX].key[iI]);
            printf("\r\n");
        }
    }
}

void UpdateMetersonStick(unsigned long handle, uint16_t stick, int iMax, pecwMBUSMeter ecpiwwMeter, uint16_t infoflag) {
    int iX;

    for(iX=0;iX<MAXMETER;iX++)
        wMBus_RemoveMeter(iX);

    for(iX=0;iX<iMax;iX++) {
        if( 0 != ecpiwwMeter[iX].manufacturerID)
            wMBus_AddMeter(handle, stick, iX, &ecpiwwMeter[iX], infoflag);
    }
}

#define XMLBUFFER (1*1024*1024)
#define TEMPBUFFER 250
unsigned int Log2XMLFile(const char *path, double Reading, ecMBUSData *rfData) {
    char szBuf[TEMPBUFFER];
    FILE    *hFile;
    unsigned char*  pXMLIN = NULL;
    unsigned char*  pXMLTop, *pXMLMem = NULL;
    unsigned char*  pXML;
    unsigned int   dwSize=0;
    unsigned int   dwSizeIn=0;
    char  CurrentTime[TEMPBUFFER];

    time_t t = time(NULL);
    struct tm tm;
    localtime_s(&tm, &t);

    if (fopen_s(&hFile, path, "rb") == NULL ) {
        fseek(hFile, 0L, SEEK_END);
        dwSizeIn = ftell(hFile);
        fseek(hFile, 0L, SEEK_SET);

        pXMLIN = (unsigned char*) malloc(dwSizeIn+4096);
        memset(pXMLIN,0,sizeof(unsigned char)*(dwSizeIn+4096));
        if(NULL == pXMLIN) {
            printf("Log2XMLFile - malloc failed \r\n");
            return 0;
        }

        fread(pXMLIN, dwSizeIn, 1, hFile);
        fclose(hFile);

        pXMLTop = (unsigned char *)strstr((const char*)pXMLIN,"<ENERGYCAMOCR>\r\n"); //search on start
        if(pXMLTop){
            pXMLTop += strlen("<ENERGYCAMOCR>\r\n");
            pXMLMem = (unsigned char *) malloc(max(4*XMLBUFFER, XMLBUFFER+dwSizeIn));
            if(NULL == pXMLMem) {
                printf("Log2XMLFile - malloc %d failed \r\n",max(4*XMLBUFFER, XMLBUFFER+dwSizeIn));
                return 0;
            }
            memset(pXMLMem,0,sizeof(unsigned char)*max(4*XMLBUFFER, XMLBUFFER+dwSizeIn));
            pXML = pXMLMem;
            dwSize=0;
            dwSizeIn -= pXMLTop-pXMLIN;

            sprintf_s(szBuf, TEMPBUFFER,"<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\r\n");
            memcpy(pXML, szBuf, strlen(szBuf)); dwSize+=strlen(szBuf); pXML+=strlen(szBuf);

            sprintf_s(szBuf, TEMPBUFFER, "<ENERGYCAMOCR>\r\n");
            memcpy(pXML, szBuf, strlen(szBuf)); dwSize+=strlen(szBuf); pXML+=strlen(szBuf);
        }
    }
    else {
        //new File
        pXMLMem = (unsigned char *) malloc(XMLBUFFER);
        memset(pXMLMem,0,sizeof(unsigned char)*XMLBUFFER);
        pXML = pXMLMem;
        dwSize=0;

        sprintf_s(szBuf, TEMPBUFFER,"<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\r\n");
        memcpy(pXML, szBuf, strlen(szBuf)); dwSize+=strlen(szBuf); pXML+=strlen(szBuf);

        sprintf_s(szBuf, TEMPBUFFER, "<ENERGYCAMOCR>\r\n");
        memcpy(pXML, szBuf,strlen(szBuf)); dwSize+=strlen(szBuf); pXML+=strlen(szBuf);
    }

    if(pXMLMem) {
        sprintf_s(szBuf, TEMPBUFFER,"<OCR>\r\n");                                                       memcpy(pXML, szBuf, strlen(szBuf)); dwSize+=strlen(szBuf); pXML+=strlen(szBuf);

        sprintf_s(CurrentTime, TEMPBUFFER, "%02d.%02d.%d %02d:%02d:%02d", tm.tm_mday, tm.tm_mon+1, tm.tm_year+1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
        sprintf_s(szBuf,       TEMPBUFFER, "<Date>%s</Date>\r\n", CurrentTime);                         memcpy(pXML, szBuf, strlen(szBuf)); dwSize+=strlen(szBuf); pXML+=strlen(szBuf);

        sprintf_s(    szBuf, TEMPBUFFER, "<Reading>%.1f</Reading>\r\n", Reading);                       memcpy(pXML, szBuf, strlen(szBuf)); dwSize+=strlen(szBuf); pXML+=strlen(szBuf);
        if(NULL != rfData) {
            sprintf_s(szBuf, TEMPBUFFER, "<RSSI>%d</RSSI>\r\n",                 rfData->rssiDBm);       memcpy(pXML, szBuf, strlen(szBuf)); dwSize+=strlen(szBuf); pXML+=strlen(szBuf);
            sprintf_s(szBuf, TEMPBUFFER, "<Pic>%d</Pic>\r\n",                   rfData->utcnt_pic);     memcpy(pXML, szBuf, strlen(szBuf)); dwSize+=strlen(szBuf); pXML+=strlen(szBuf);
            sprintf_s(szBuf, TEMPBUFFER, "<Tx>%d</Tx>\r\n",                     rfData->utcnt_tx);      memcpy(pXML, szBuf, strlen(szBuf)); dwSize+=strlen(szBuf); pXML+=strlen(szBuf);
            sprintf_s(szBuf, TEMPBUFFER, "<ConfigWord>%d</ConfigWord>\r\n",     rfData->configWord);    memcpy(pXML, szBuf, strlen(szBuf)); dwSize+=strlen(szBuf); pXML+=strlen(szBuf);
            sprintf_s(szBuf, TEMPBUFFER, "<wMBUSStatus>%d</wMBUSStatus>\r\n",   rfData->status);        memcpy(pXML, szBuf, strlen(szBuf)); dwSize+=strlen(szBuf); pXML+=strlen(szBuf);

        }
        sprintf_s(szBuf,TEMPBUFFER,"</OCR>\r\n");                                                       memcpy(pXML, szBuf, strlen(szBuf)); dwSize+=strlen(szBuf); pXML+=strlen(szBuf);


        if(dwSizeIn>0){
            memcpy(pXML, pXMLTop, dwSizeIn);
        }
        else {
            sprintf_s(szBuf, TEMPBUFFER,"</ENERGYCAMOCR>\r\n"); memcpy(pXML, szBuf, strlen(szBuf)); dwSize+=strlen(szBuf); pXML+=strlen(szBuf);
        }

        if (fopen_s(&hFile,path, "wb") == NULL ) {
            fwrite(pXMLMem, dwSizeIn+dwSize, 1, hFile);
            fclose(hFile);
        }
    }

    if(pXMLMem) free(pXMLMem);
    if(pXMLIN) free(pXMLIN);

    return true;
}

//support commandline
int parseparam(int argc, char *argv[], uint16_t *infoflag, uint16_t *Port, uint16_t *Mode) {
    int i;
    char *key, *value;

    if((NULL == infoflag) || (NULL == Port)  || (NULL == Mode) ) return 0;

    for( i = 1; i < argc; i++ ) {
        if( *argv[i] == '/' ) {
            key = argv[i] + 1;
            value = strchr(key, ':');
            if( value != NULL ) *value++ = 0;

            switch (*key){
            case 'i':
                *infoflag = SHOWDETAILS;
                break;
            case 'c':
                if( NULL != value)
                    *Port = (uint16_t)atoi(value);
                break;
            case 'm':
                if( NULL != value){
                    if(*value == 'T') *Mode = RADIOT2;
                    if(*value == 'S') *Mode = RADIOS2;
                }
                break;
            case 'h':
                IntroShowParam();
                exit (0);
                break;
            }
        }
    }
    return 0;
}

int getkey(void) {
    int character = 0;
    if(_kbhit()){ 
        character = _getch(); // Muss auf keine Eingabe warten, Taste ist bereits gedrückt 
    }
    return character;
}

//////////////////////////////////////////////
int main(int argc, char *argv[]) {
    int      key    = 0;
    int      iCheck = 0;
    int      iX;
    int      iK;
    char     KeyInput[_MAX_PATH];
    char     Key[3];
    double   csvValue;
    int      Meters =0;
    unsigned long ReturnValue;
    FILE    *hDatFile;

    uint16_t InfoFlag = SILENTMODE;
    uint16_t Port = 17;
    uint16_t Mode = RADIOT2;
    uint16_t wMBUSStick = iM871AIdentifier;
	
    char     comDeviceName[_MAX_PATH];
    int      hStick;

    ecwMBUSMeter ecpiwwMeter[MAXMETER];
    memset(ecpiwwMeter, 0, MAXMETER*sizeof(ecwMBUSMeter));

    if(argc > 1)
        parseparam(argc, argv, &InfoFlag, &Port, &Mode);

    //read config back
    if (fopen_s(&hDatFile,"meter.dat", "rb") == NULL) {
        Meters = fread((void*)ecpiwwMeter, sizeof(ecwMBUSMeter), MAXMETER, hDatFile);
        fclose(hDatFile);
    }

    Intro();

    //open wMBus Stick #1
    wMBUSStick = iM871AIdentifier;
    sprintf_s(comDeviceName,_MAX_PATH, "\\\\.\\COM%d", Port );
    hStick = wMBus_OpenDevice(comDeviceName ,wMBUSStick);

    if(NULL == hStick ){ //try 2.Stick
            wMBUSStick = iAMB8465Identifier;
            hStick = wMBus_OpenDevice(comDeviceName, wMBUSStick);
    }

    if(hStick <= 0) {
         ErrorAndExit("no wMBUS Stick not found\n");  
         }

    if((iM871AIdentifier == wMBUSStick) && (APIOK == wMBus_GetStickId(hStick, wMBUSStick, &ReturnValue, InfoFlag)) && (iM871AIdentifier == ReturnValue)){
        if(InfoFlag > SILENTMODE) {
            printf("iM871A Stick found");
        }
    }
    else
    {
        wMBus_CloseDevice(hStick,wMBUSStick);
        //
        wMBUSStick = iAMB8465Identifier;
        hStick = wMBus_OpenDevice(comDeviceName, wMBUSStick);
        if((iAMB8465Identifier == wMBUSStick) && (APIOK == wMBus_GetStickId(hStick, wMBUSStick, &ReturnValue, InfoFlag)) && (iAMB8465Identifier == ReturnValue)){
            if(InfoFlag > SILENTMODE) {
                printf("Amber Stick found");
            }
        }     
        else{
            wMBus_CloseDevice(hStick, wMBUSStick);
            ErrorAndExit("no wMBUS Stick not found\n");
            }

    }

    if(APIOK == wMBus_GetRadioMode(hStick, wMBUSStick, &ReturnValue,InfoFlag)){
        if(InfoFlag > SILENTMODE) {
            printf("wMBUS %s Mode", (ReturnValue == RADIOT2) ? "T2" : "S2");
        }          
        if (ReturnValue != Mode)
           wMBus_SwitchMode( hStick, wMBUSStick, (uint8_t)Mode, InfoFlag);
       }
    else
        ErrorAndExit("wMBUS Stick not found\n");

    wMBus_InitDevice(hStick,wMBUSStick,InfoFlag);

    UpdateMetersonStick(hStick, wMBUSStick, Meters, ecpiwwMeter, InfoFlag);

    IsNewMinute();

    while (!((key == 0x1B) || (key == 'q'))) {
        Sleep(1000);   //sleep 500ms

        key = getkey();

        /*key =fgetc(stdin);
        while(key!='\n' && fgetc(stdin) != '\n');
        printf("Key=%d",key);*/


        //add a new Meter
        if (key == 'a') {
            iX=0;
            while(0 != ecpiwwMeter[iX].manufacturerID) {
                iX++;
                if(iX == MAXMETER-1)
                  continue;
              }
            //check entry in list of meters
            if(iX < MAXMETER) {
                printf("\nAdding Meter #%d \n",iX+1);
                printf("Enter Meter Ident (12345678): ");
                if(fgets(KeyInput, _MAX_PATH,stdin))
                    ecpiwwMeter[iX].ident=CalcUIntBCD(atoi(KeyInput));

                printf("Enter Meter Type (2 = Electricity ; 3 = Gas ; 7 = Water) : ");
                if(fgets(KeyInput, _MAX_PATH,stdin)) {
                    switch(atoi(KeyInput))  {
                        case METER_GAS  :        ecpiwwMeter[iX].type = METER_GAS;          break;
                        case METER_WATER:        ecpiwwMeter[iX].type = METER_WATER;        break;
                        default: printf(" - wrong Type ; default to Electricity");
                        case METER_ELECTRICITY : ecpiwwMeter[iX].type = METER_ELECTRICITY;  break;
                    }

                }
                ecpiwwMeter[iX].manufacturerID = FASTFORWARD;
                ecpiwwMeter[iX].version        = 0x01;

                printf("Enter Key (0 = Zero ; 1 = Default ; 2 = Enter the 16 Bytes) : ");
                if(fgets(KeyInput, _MAX_PATH, stdin))
                {
                    switch(atoi(KeyInput)) {
                        case 0  : for(iK = 0; iK<AES_KEYLENGHT_IN_BYTES; iK++)
                                    ecpiwwMeter[iX].key[iK] = 0;
                                  break;
                        default:
                        case 1  : for(iK = 0; iK<AES_KEYLENGHT_IN_BYTES; iK++)
                                    ecpiwwMeter[iX].key[iK] = (uint8_t)(0x1C + 3*iK);
                                 break;
                        case 2  :
                                printf("Key:");
                                fgets(KeyInput,_MAX_PATH,stdin);
                                    for(iK = 0; iK<AES_KEYLENGHT_IN_BYTES; iK++)
                                        ecpiwwMeter[iX].key[iK] = 0;
                                if((strlen(KeyInput)-1) < AES_KEYLENGHT_IN_BYTES*2)
                                    printf("Key is too short - default to Zero\n");
                                else {
                                    memset(Key,0,sizeof(Key));
                                    for(iK = 0; iK<(int)(strlen(KeyInput)-1)/2; iK++) {
                                        Key[0] =  KeyInput[2*iK];
                                        Key[1] =  KeyInput[2*iK+1];
                                        ecpiwwMeter[iX].key[iK] = (uint8_t) strtoul(Key,NULL,16);
                                        }
                                }
                        break;
                    }
                }

                Meters++;
                Meters = min(Meters, MAXMETER);
                DisplayListofMeters(Meters, ecpiwwMeter);
                UpdateMetersonStick(hStick, wMBUSStick, Meters, ecpiwwMeter, InfoFlag);
            } else
                printf("All %d Meters defined\n", MAXMETER);
        }

        // display list of meters
        if(key == 'l')
            DisplayListofMeters(Meters, ecpiwwMeter);

        //remove a meter from the list
        if(key == 'r') {
            printf("Enter Meterindex to remove: ");
            if(fgets(KeyInput, _MAX_PATH, stdin)) {
                iX = atoi(KeyInput);
                if(iX-1 <= Meters-1) {
                    printf("Remove Meter #%d\r\n",iX);
                    memset(&ecpiwwMeter[iX-1], 0, sizeof(ecwMBUSMeter));
                    DisplayListofMeters(Meters, ecpiwwMeter);
                    UpdateMetersonStick(hStick, wMBUSStick, Meters, ecpiwwMeter, InfoFlag);
                } 
                else
                    printf(" Index not defined\n");
            }
        }

        // switch to S2 mode
        if(key == 's')
            wMBus_SwitchMode( hStick,wMBUSStick, RADIOS2, InfoFlag);

        // switch to T2 mode
        if(key == 't')
            wMBus_SwitchMode( hStick,wMBUSStick, RADIOT2, InfoFlag);

        //check whether there are new data from the EnergyCams
        if (IsNewMinute() || (key == 'u')) {
            if(wMBus_GetMeterDataList() > 0) {
                iCheck = 0;
                for(iX=0; iX<Meters; iX++){
                    if((0x01<<iX) & wMBus_GetMeterDataList()) {
                        ecMBUSData RFData;
                        int iMul=1;
                        int iDiv=1;
                        wMBus_GetData4Meter(iX, &RFData);
                        if(RFData.exp < 0) {  //GAS
                            for(iK=RFData.exp; iK<0; iK++)
                               iDiv=iDiv*10;
                            csvValue = ((double)RFData.value)/iDiv;
                        } else {
                            for(iK=0; iK<RFData.exp; iK++)
                                iMul=iMul*10;
                            csvValue = (double)RFData.value*iMul;
                        }
                        if(InfoFlag >= SILENTMODE) {
                            printf("Meter #%d : %4.1f %s", iX+1, csvValue, (ecpiwwMeter[iX].type == METER_ELECTRICITY) ? "Wh" : "m'3");
                        }
                        if(ecpiwwMeter[iX].type == METER_ELECTRICITY)
                            csvValue = csvValue/1000.0;

                        if(InfoFlag >= SILENTMODE) {
                            if((RFData.pktInfo & PACKET_WAS_ENCRYPTED)      == PACKET_WAS_ENCRYPTED)     printf(" Decryption OK");
                            if((RFData.pktInfo & PACKET_DECRYPTIONERROR)    == PACKET_DECRYPTIONERROR)   printf(" Decryption ERROR");
                            if((RFData.pktInfo & PACKET_WAS_NOT_ENCRYPTED)  == PACKET_WAS_NOT_ENCRYPTED) printf(" not encrypted");
                            if((RFData.pktInfo & PACKET_IS_ENCRYPTED)       == PACKET_IS_ENCRYPTED)      printf(" is encrypted");
                            printf(" RSSI=%i dbm, #%d \n", RFData.rssiDBm, RFData.accNo);
                        }
                        //Log2File(CommandlineDatPath,LogMode,iX,InfoFlag,csvValue,&RFData,ecpiwwMeter[iX].ident);
                    }
                }
            } 
            else {
                if(InfoFlag > SILENTMODE) {
                    printf("%02d ", iCheck);
                    if((++iCheck % 20) == 0) printf("\r\n");
                }
            }
        }
    } // end while

    if(hStick >0) wMBus_CloseDevice(hStick, wMBUSStick);

    //save Meter config to file
    if(Meters > 0) {
        if ( fopen_s(&hDatFile,"meter.dat", "wb") == NULL) {
            fwrite((void*)ecpiwwMeter, sizeof(ecwMBUSMeter), MAXMETER, hDatFile);
            fclose(hDatFile);
        }
    }
    return 0;
}
