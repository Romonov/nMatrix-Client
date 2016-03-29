//=================================================================================================
//
//  Copyright (C) 2012-2016, DigiStar Studio
//
//  This file is part of nMatrix client app (http://feather1231.myweb.hinet.net/)
//
//  All rights reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//
//  Above are some template claims copied from somewhere on the Internet that don't matter.
//  The code is written by c++ language and I know you have no fear of it. :D
//  Hope that you use the code to do good things like google's slogan, don't be evil.
//  Any feedback is welcomed.
//
//  Author: Pointer Huang <digistarstudio@google.com>
//
//
//=================================================================================================


#pragma once


#include <winioctl.h>
#include <iphlpapi.h>
#include "NetworkDataType.h"


#pragma comment(lib, "iphlpapi.lib")


// Define vendor byte of the vmac that used for fast index.
#define FIMAC_VB1 0x00
#define FIMAC_VB2 0x00
#define FIMAC_VB3 0x01
#define FIMAC_VB4 0x00


#define FIMAC_VALUE ((FIMAC_VB4 << 16) | (FIMAC_VB3 << 16) | (FIMAC_VB2 << 8) | FIMAC_VB1)


#define IOCTL_SET_IP_MAC   CTL_CODE(FILE_DEVICE_NETWORK, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_OPEN_UDP     CTL_CODE(FILE_DEVICE_NETWORK, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CLOSE_UDP    CTL_CODE(FILE_DEVICE_NETWORK, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_UDP_INFO CTL_CODE(FILE_DEVICE_NETWORK, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_ADD_ADDRESS    CTL_CODE(FILE_DEVICE_NETWORK, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_TABLE_INFO CTL_CODE(FILE_DEVICE_NETWORK, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CLEAN_TABLE    CTL_CODE(FILE_DEVICE_NETWORK, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SET_TABLE_DATA CTL_CODE(FILE_DEVICE_NETWORK, 0x807, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SET_TABLE_ITEM CTL_CODE(FILE_DEVICE_NETWORK, 0x808, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SET_ENTRY_DATA CTL_CODE(FILE_DEVICE_NETWORK, 0x809, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_GET_TRAFFIC_INFO CTL_CODE(FILE_DEVICE_NETWORK, 0x80A, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SET_TRAFFIC_INFO CTL_CODE(FILE_DEVICE_NETWORK, 0x80B, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ATTACH_USER_MODE CTL_CODE(FILE_DEVICE_NETWORK, 0x80C, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SEND     CTL_CODE(FILE_DEVICE_NETWORK, 0x821, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_RECEIVE  CTL_CODE(FILE_DEVICE_NETWORK, 0x822, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_REG_SEND_BUFFER  CTL_CODE(FILE_DEVICE_NETWORK, 0x823, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_REG_RECV_BUFFER  CTL_CODE(FILE_DEVICE_NETWORK, 0x824, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_PRINT_DATA       CTL_CODE(FILE_DEVICE_NETWORK, 0x825, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_NEITHER_IO_INFO  CTL_CODE(FILE_DEVICE_NETWORK, 0x826, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_GET_ADDRESS      CTL_CODE(FILE_DEVICE_NETWORK, 0x827, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SET_ADAPTER_FUNC CTL_CODE(FILE_DEVICE_NETWORK, 0x828, METHOD_BUFFERED, FILE_ANY_ACCESS)


// For debug.
#define IOCTL_DRIVER_VERSION   CTL_CODE(FILE_DEVICE_NETWORK, 0x891, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_PRINT_TABLE      CTL_CODE(FILE_DEVICE_NETWORK, 0x892, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DRIVER_STATE     CTL_CODE(FILE_DEVICE_NETWORK, 0x893, METHOD_BUFFERED, FILE_ANY_ACCESS)
//#define IOCTL_READ_STRUCT_SIZE CTL_CODE(FILE_DEVICE_NETWORK, 0x893, METHOD_BUFFERED, FILE_ANY_ACCESS)


#define ADAPTER_NAME "\\\\.\\nMatrix"

#define ADAPTER_HARDWARE_ID "nMatrix" //"root\\nMatrix"
#define MAX_NETWORK_CLIENT 256
#define WRITE_FILE_MAX_PACKET 32
#define ADAPTER_DESC "nMatrix VPN Interface"


#define DRIVER_FILE_NAME "nMatrix.sys"
#define INF_FILE_NAME    "nMatrix.inf"
#define CAT_FILE_NAME    "nMatrix.cat"

#define DEVINS_FILE_NAME "devins.exe"

//#define DRIVER_UM_READ_EVENT  "nMatrixUMReadEvent"
//#define DRIVER_UM_WRITE_EVENT "nMatrixUMWriteEvent"
#define DRIVER_UM_READ_EVENT  "Global\\nMatrixUMReadEvent"
#define DRIVER_UM_WRITE_EVENT "Global\\nMatrixUMWriteEvent"
#define DRIVER_RELAY_EVENT    "Global\\nMatrixRelayEvent"


#define     ETH_HEADER_SIZE             14
#define     ETH_MAX_DATA_SIZE           1400 // Can't use 1440.
#define     ETH_MAX_PACKET_SIZE         (ETH_HEADER_SIZE + ETH_MAX_DATA_SIZE)
#define     ETH_MIN_PACKET_SIZE         60


typedef struct _mac
{
	BYTE m[6];
} mac;

enum ADDRESS_INFO_FLAG
{
	AIF_RESERVED     = 0x01,
	AIF_START_PT     = 0x01 << 1,
	AIF_ACK_RECEIVED = 0x01 << 2,
	AIF_PT_FIN       = 0x01 << 3,
	AIF_RH_RIGHT     = 0x01 << 4, // The peer host has right to use service of the relay host.
	AIF_RELAY_HOST   = 0x01 << 5, // Access peer using relay host.

	AIF_SERVER_RELAY = 0x01 << 6,
	AIF_TCP_TUNNEL   = 0x01 << 7,

	AIF_OK           = 0x01 << 8,
	AIF_PENDING      = 0x01 << 9,
	AIF_IPV4         = 0x01 << 10,
	AIF_IPV6         = 0x01 << 11,
};


enum ADAPTER_CHECKSUM_OFFLOAD_TYPE
{
	ACOT_IPV4_IP_SEND      = 0x01,
	ACOT_IPV4_IP_RECV      = (0x01 << 1),
	ACOT_IPV4_IP_OPT_SEND  = (0x01 << 2),
	ACOT_IPV4_IP_OPT_RECV  = (0x01 << 3),
	ACOT_IPV4_TCP_SEND     = (0x01 << 4),
	ACOT_IPV4_TCP_RECV     = (0x01 << 5),
	ACOT_IPV4_TCP_OPT_SEND = (0x01 << 6),
	ACOT_IPV4_TCP_OPT_RECV = (0x01 << 7),
	ACOT_IPV4_UDP_SEND     = (0x01 << 8),
	ACOT_IPV4_UDP_RECV     = (0x01 << 9),

	ACOT_IPV6_IP_EXT_SEND  = (0x01 << 10),
	ACOT_IPV6_IP_EXT_RECV  = (0x01 << 11),
	ACOT_IPV6_TCP_OPT_SEND = (0x01 << 12),
	ACOT_IPV6_TCP_OPT_RECV = (0x01 << 13),
	ACOT_IPV6_TCP_SEND     = (0x01 << 14),
	ACOT_IPV6_TCP_RECV     = (0x01 << 15),
	ACOT_IPV6_UDP_SEND     = (0x01 << 16),
	ACOT_IPV6_UDP_RECV     = (0x01 << 17),
};


enum ADAPTER_FUNC_FLAG
{
	AFF_USE_FMAC_INDEX = 1,
};


typedef struct _stEntry
{
	BYTE      mac[6]; // Virtual adapter mac.
	USHORT    flag;
	IpAddress pip;
	DWORD     uid, vip;     // UID used as key when punching through tunnel.
	USHORT    port, dindex; // UDP port (Network byte order).
	USHORT    rhindex;      // The following two index is used for relaying.
	USHORT    destindex;
	BYTE      Key[32];      // Encode key 256bit.
} stEntry;

typedef struct _stMapTable
{
	UINT m_Count;
	stEntry Entry[256];

	UINT DataIn[MAX_NETWORK_CLIENT], DataOut[MAX_NETWORK_CLIENT];

} stMapTable;


typedef struct _stSetItemData // Driver use this too.
{
	UINT index, count;
	stEntry entry[MAX_NETWORK_CLIENT];
} stSetItemData;


typedef struct _stDirectSendInfo // Driver use this too.
{
	IpAddress ip;
	USHORT port, datasize, bIsIPV6;
	BYTE  data[1400];
} stDirectSendInfo;


#pragma pack(1)

typedef struct _stHelloPacket
{
	DWORD  key;
	USHORT Index;
	BYTE   vmac[6];
} stHelloPacket;

typedef struct _sIPHeader
{
	BYTE ver:4;
	BYTE len:4;
} sIPHeader;

typedef struct _sEthernetFrame
{
	UCHAR  DestMac[6];     // Sender hardware address.
	UCHAR  SourMac[6];  // Sender hardware address.
	USHORT Type;
} sEthernetFrame;

typedef struct _sArpPacket
{
	SHORT HardwareType;
	SHORT ProtocolType;
	UCHAR HLen;
	UCHAR PLen;
	SHORT Op;
	UCHAR SHA[6];   // Sender hardware address.
	UINT  SPA;      // Sender protocol address.
	UCHAR THA[6];   // Target hardware address.
	UINT  TPA;      // Sender protocol address.
} sArpPacket;

typedef struct _IpHeader
{
	BYTE   version, services;
	USHORT Length, ID, Flags;
	BYTE   TTL, Protocol;
	USHORT Checksum;
	DWORD  SourIP, DestIP;
} stIpHeader;

typedef struct _stPacketHeader // The size of the struct can't large than PACKET_SKIP_SIZE.
{
	USHORT dindex;
	BYTE flag1, flag2;

	union
	{
		DWORD dw;
		struct
		{
			USHORT RHDestIndex, SrcHostDestIndex;
		};
		BYTE  reserved[4];
	};

} stPacketHeader;

typedef struct _stCtrlPacket // 20 Bytes.
{
	stPacketHeader PacketHeader;

	USHORT CtrlType, Reserved;
	DWORD  dw1, dw2;

} stCtrlPacket;

#pragma pack()


#define PACKET_SKIP_SIZE 12


enum PACKET_HEADER_FLAG
{
	PHF_NULL      = 0x00,
	PHF_BROADCAST = 0x01,
	PHF_MULTICAST = 0x01 << 1,
	PHF_USE_RELAY = 0x01 << 2, // Client send data to relay host.
	PHF_RH_SEND   = 0x01 << 3, // Relay host send data to dest host.


	PHF_CTRL_PKT  = 0x01 << 6,
	PHF_ENCRYPT   = 0x01 << 7,
};


typedef struct _stAdapterInfo
{
	DWORD ipv4;
	IpAddress ipv6;
	BYTE mac[6];
	BOOL bTaskOffload;
} stAdapterInfo;


typedef struct _stDriverVerInfo
{
	USHORT Flag;
	DWORD  DriVersion;
	DWORD  DriBuildDate;
	DWORD  MinAppVersion;
} stDriverVerInfo;


typedef struct _stDriverInternalState
{
	UINT SendThreadID, RecvThreadID;
	UINT TDISend, TDICom, TDISyncSend, TDISyncCom, TDISecWait, TDIExitSecWait, TDIError;

	UINT nFreeSendSlot, nFreeSendSlotDC, nBusySendSlot, nTotalSendSlot;
	UINT nFreeRecvSlot, nFreeRecvSlotDC;

	LARGE_INTEGER DriverStartTime;
} stDriverInternalState;


typedef struct _stDriverNotifyHeader
{
	USHORT DMIndex; // Should be INVALID_DM_INDEX.
	USHORT type;
} stDriverNotifyHeader;


typedef struct _stPingHostNotifyInfo
{
	stDriverNotifyHeader header;

	UINT   nIndex;
	LARGE_INTEGER liTime;
} stPingHostNotifyInfo;


#define MAX_READ_BUFFER_SLOT 128
typedef struct _Slot
{
	volatile UINT Len;
	BYTE Buffer[1500];
} stBufferSlot;
typedef struct _stReadBuffer
{
	volatile LONG DataCount;
	BYTE BufferSize, StartIndex, byReserved, byReserved2;
	stBufferSlot BufferSlot[MAX_READ_BUFFER_SLOT];
} stReadBuffer;

template <size_t N>
struct stKBuffer
{
public:
	stKBuffer()
	{
		DataCount = 0;
		BufferSize = N;
		StartIndex = 0;
		memset(BufferSlot, 0, sizeof(BufferSlot));
	}

	volatile LONG DataCount;
	BYTE BufferSize, StartIndex, byReserved, byReserved2;
	stBufferSlot BufferSlot[N];

};


HANDLE OpenAdapter(BOOL bOverlapped = FALSE); // Return INVALID_HANDLE_VALUE if failed.
BOOL CloseAdapter(HANDLE hAdapter);

BOOL GetAdapterDriverVersion(CString DriverDesc, CString &version);
void GetAdapterRegInfo(BYTE Mac[], DWORD *vip, DWORD *TaskOffload);
BOOL SetAdapterRegInfo(BYTE Mac[], DWORD vip, DWORD TaskOffload);

BOOL OpenUDP(HANDLE hAdapter, DWORD ip, USHORT port, USHORT ipcport);
BOOL CloseUDP(HANDLE hAdapter);
BOOL GetUDPInfo(HANDLE hAdapter, DWORD &ip, USHORT &port, BOOL &bOpen);

UINT GetTableData(HANDLE hAdapter, stMapTable *pTable);
BOOL SetTableData(HANDLE hAdapter, stMapTable *pTable);
BOOL CleanTable(HANDLE hAdapter);
BOOL SetTableItem(HANDLE hAdapter, UINT count, UINT index, stEntry *pEntry); // Used to modify items that exist.
BOOL AddTableEntry(HANDLE hAdapter, stEntry *pEntry, UINT count);
BOOL SetEntryData(HANDLE hAdapter, INT Index, INT nType, INT value);

BOOL EnableUserMode(HANDLE hAdapter, DWORD dwMode, BOOL bEnable); // Mode 1: send. Mode 2: receive.

BOOL ReadTrafficInfo(HANDLE hAdapter, UINT *pDataIn, UINT *pDataOut, UINT ReadCount);
BOOL WriteTrafficInfo(HANDLE hAdapter, UINT *pDataIn, UINT *pDataOut, UINT Count);
BOOL DirectSend(HANDLE hAdapter, IpAddress ip, BOOL bIsIPV6, USHORT port, void *pData, UINT size);

BOOL RegKernelBuffer(HANDLE hAdapter, OVERLAPPED *pwo, void *pBuffer, UINT size, BOOL bSendBuffer, BOOL bOpt = FALSE);

BOOL SetAdapterFunc(HANDLE hAdapter, DWORD dwFuncFlags);
BOOL GetAdapterParam(HANDLE hAdapter, BYTE Mac[], DWORD *vip, BOOL *TaskOffload);
BOOL SetAdapterParam(HANDLE hAdapter, BYTE Mac[], DWORD vip);
BOOL GetAdapterDriverVersion(HANDLE hAdapter, stDriverVerInfo *pOut);

BOOL PrintData(HANDLE hAdapte);
void GetDriverState(HANDLE hAdapter, stDriverInternalState *pDriverState);


BOOL GetVirtualAdapterInfo(BOOL *pbDHCP = 0, CString *pIP = 0, CString *pGUID = 0, DWORD *pAdapterIndex = 0);
inline BOOL FindAdapter() { return GetVirtualAdapterInfo(); }
BOOL RenewAdapter(UINT nReleaseAddressMode); // 1 for releasing address, 2 for releasing only.
BOOL ResetAdapter();
BOOL SetNetworkConnectionName(CString TargetAdapterDesc, CString NewName);
void SetNT6NetworkName(CString NewName);
void SwitchNT6NetworkLocation();

INT  InstallDriver(TCHAR *strHardwareID, TCHAR *strInfPath, HWND hWnd = 0);
BOOL UninstallDriver(BOOL &bNeedReboot);


// Debug.
void DebugCheck();


class CVirtualAdapter
{
public:
	CVirtualAdapter()
	:m_hAdapter(0)
	{
	}
	~CVirtualAdapter()
	{
	}


protected:
	HANDLE m_hAdapter;


};


