#ifndef _LIBWMBUS_H_
#define _LIBWMBUS_H_

#define BUFFER_SIZE 1024
#define DLLNAME _T("WMBUSHCI.DLL")

//Modes
#define RADIOT2	4
#define RADIOS2	2

#define RADIOT2_AMB	0x08
#define RADIOS2_AMB	0x03


//Returns
#define APIOK   0
#define APIERROR (-1)


//Messages
#define WMBUS_MSG_HCI_MESSAGE_IND 0x00000004
#define WMBUS_MSGID_AES_DECRYPTIONERROR 0x27

#define WMBUS_MSGLENGTH_AESERROR 9
#define WMBUS_PAYLOADLENGTH_ENCRYPTED 30
#define WMBUS_PAYLOADLENGTH_DEFAULT 25


typedef void (*TWBus_CbMsgHandler)(UINT32 msg, UINT32 param);

#endif

