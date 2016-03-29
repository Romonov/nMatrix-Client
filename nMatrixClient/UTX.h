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


// Compiling time print macro for visual studio.
#define STRING2(x) #x
#define STRING(x) STRING2(x)
#define FILE_LOC __FILE__ "(" STRING(__LINE__) ") : "
// #pragma message(FILE_LOC "Add your message here")


#define ENCRYPT_PACKET_DATA 1
#define AES_KEY_LENGTH  256
#define static_assert ASSERT


#define SAFE_DELETE(p) if((p) != NULL) { delete (p); (p) = NULL; }
#define SAFE_DELETE_ARRAY(p) if((p) != NULL) { delete[] (p); (p) = NULL; }
#define SAFE_RELEASE(p) if((p) != NULL) { (p)->Release(); (p) = NULL; }
#define SAFE_CLOSE_HANDLE(h) if((h) != NULL) { CloseHandle(h); (h) = NULL; }


#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))


#define MAX_UINT_VALUE 4294967295
#define MAX_INT_VALUE  2147483647
#define MAX_BYTE_VALUE 255


BOOL UTXLibraryInit();
void UTXLibraryEnd();

int printx(const char * format, ...);
int printx(const wchar_t * format, ...);
void PrintMACaddress(BYTE *MacAddress);
BOOL Is64BitsOs();
void ParseDateTime(CHAR *pDate, CHAR *pTime, INT &year, INT &month, INT &date, INT &hour, INT &minute);
BOOL AppAutoStart(const TCHAR *pItemName, const TCHAR *pExePath, const TCHAR *pParam, BOOL bStart);
BOOL SaveResourceToFile(const TCHAR *strPath, WORD ResID, HMODULE hInstance = 0);


#define IS_ALIGNED(addr, byte_count) IsAligned((void*)(addr), (byte_count))
#define POINTER_IS_ALIGNED(addr)     IsAligned((void*)(addr), sizeof(void*))

inline BOOL IsAligned(const void *__restrict address, size_t byte_count)
{
	return ((uintptr_t)address % byte_count) == 0;
}


#if defined(DEBUG_ATOMIC_FUNCTION) || defined(DEBUG)

template<class T>
__forceinline T InterlockedIncrementEx(volatile T *addr)
{
	ASSERT(sizeof(T) == 4 && IS_ALIGNED(addr, 4)); // 32-bit only.
	return (T)InterlockedIncrement((volatile LONG*)addr);
}

template<class T>
__forceinline T InterlockedDecrementEx(volatile T *addr)
{
	ASSERT(sizeof(T) == 4 && IS_ALIGNED(addr, 4)); // 32-bit only.
	return (T)InterlockedDecrement((volatile LONG*)addr);
}

template<class T>
__forceinline void* InterlockedExchangePointerEx(T **Target, void *Value)
{
	ASSERT(POINTER_IS_ALIGNED(Target));
	return InterlockedExchangePointer((void**)Target, Value);
}

template<class T>
__forceinline void* InterlockedCompareExchangePointerEx(T **Destination, PVOID Exchange, PVOID Comparand)
{
	ASSERT(POINTER_IS_ALIGNED(Destination));
	return InterlockedCompareExchangePointer((void**)Destination, Exchange, Comparand);
}

template<class T>
__forceinline T InterlockedExchangeEx(T volatile* Target, INT value)
{
	ASSERT(sizeof(T) == 4);
	return (T)InterlockedExchange((volatile LONG*)Target, value);
}

#else

#define InterlockedIncrementEx(addr) InterlockedIncrement((volatile LONG*)addr)
#define InterlockedDecrementEx(addr) InterlockedDecrement((volatile LONG*)addr)
#define InterlockedExchangePointerEx(addr, value) InterlockedExchangePointer((void**)(addr), value)
#define InterlockedCompareExchangePointerEx(addr, value, comparand) InterlockedCompareExchangePointer((void**)(addr), value, comparand)
#define InterlockedExchangeEx(Target, value) InterlockedExchange((volatile LONG*)(Target), value)

#endif // Debug


template<typename int_t>
UINT GetVarIntByteCount(int_t value)
{
	UINT nOutputSize = 0;
	while(value > 127)
	{
		value >>= 7; // Remove the seven bits we just wrote.
		nOutputSize++;
	}
	return ++nOutputSize;
}

template<typename int_t>
UINT EncodeVarInt(int_t value, BYTE* output)
{
	UINT outputSize = 0;
	// While more than 7 bits of data are left, occupy the last output byte and set the next byte flag.
	for (; value > 127; value >>= 7) // Remove the seven bits we just wrote.
		output[outputSize++] = ((BYTE)(value & 127)) | 128; // |128: Set the next byte flag.
	output[outputSize++] = ((BYTE)value) & 127;
	return outputSize;
}

/*
 * Decodes an unsigned variable-length integer using the MSB algorithm.
 */
template<typename int_t>
UINT DecodeVarInt(int_t &out, BYTE* input, UINT MaxByteCount = 0)
{
	BYTE by;
	UINT ByteCount;
	if(!MaxByteCount)
		MaxByteCount = (UINT)(-1);
	for(out = 0, ByteCount = 0; ByteCount < MaxByteCount; ++ByteCount)
	{
		by = input[ByteCount];
		out |= (by & 127) << (7 * ByteCount);
		if(!(by & 128)) // If the next-byte flag is set.
			break;
	}
	++ByteCount;
	return ByteCount;
}

inline void ChangeByteOrder(DWORD &data)
{
	DWORD t1 = ((data & 0x000000FF) << 24) | ((data & 0x0000FF00) << 8) | ((data & 0x00FF0000) >> 8) | ((data & 0xFF000000) >> 24);
	data = t1;
}

inline void UTXGetWorkingPath(TCHAR *buffer, DWORD size)
{
	GetModuleFileName(0, buffer, size);
}

inline INT Abs(INT n)
{
	if(n < 0)
		return 0 - n;
	return n;
}

template <typename DataType>
inline void SetRange(DataType &in, DataType low, DataType high)
{
	if(in < low)
		in = low;
	else if(in > high)
		in = high;
}

inline USHORT NBPort(USHORT port)
{
	return (port << 8) | (port >> 8);
}


class CCriticalSectionUTX
{
public:

	CCriticalSectionUTX(DWORD dwSpinCount = 0, DWORD Flags = 0) { ::InitializeCriticalSectionAndSpinCount(&m_cs, dwSpinCount); }
	~CCriticalSectionUTX()	{ ::DeleteCriticalSection(&m_cs); }


	DWORD SetCriticalSectionSpinCount(DWORD dwSpinCount) { return ::SetCriticalSectionSpinCount(&m_cs, dwSpinCount); }
	BOOL  TryEnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection) { return ::TryEnterCriticalSection(&m_cs); }

	void EnterCriticalSection() { ::EnterCriticalSection(&m_cs); }
	void LeaveCriticalSection() { ::LeaveCriticalSection(&m_cs); }


protected:

	CRITICAL_SECTION m_cs;


};


class CPerformanceCounter
{
public:

	__forceinline void Start()
	{
		QueryPerformanceCounter(&t);
	}
	__forceinline void End()
	{
		LARGE_INTEGER et;
		QueryPerformanceCounter(&et);
		t.QuadPart = et.QuadPart - t.QuadPart;
	}

	DOUBLE Get(LARGE_INTEGER f)
	{
		return (DOUBLE)t.QuadPart / f.QuadPart * 1000;
	}
	DOUBLE Get()
	{
		return (DOUBLE)t.QuadPart * factor; // This won't lose precision. (format token: %f)
	}

	static LARGE_INTEGER GetFreq() { return freq; }


protected:

	friend BOOL UTXLibraryInit();

	static LARGE_INTEGER freq;
	static DOUBLE factor;

	LARGE_INTEGER t;


};


typedef struct _list_entry
{
	_list_entry *prev, *next;
} list_entry, list_head;


#define INIT_LIST_HEAD(p) {(p)->prev = (p)->next = p;}
#define IS_VALID_ENTRY(p, head) ((list_entry*)(p) != &(head))
#define LIST_IS_EMPTY(head) ((head).next == &(head)) // Must test next pointer to be compatible with list_add_single.
#define IS_NULL_ENTRY(entry) ((entry).next == NULL)


// Insert a new entry between two known consecutive entries.
// This is only for internal list manipulation where we know the prev/next entries already!
 static inline void __list_add(list_entry *newnode, list_entry *prev, list_entry *next)
{
	next->prev = newnode;
	newnode->next = next;
	newnode->prev = prev;
	prev->next = newnode;
}

static inline void __list_del(list_entry *prev, list_entry *next)
{
	next->prev = prev;
	prev->next = next;
}

static inline INT list_count(list_head *phead)
{
	INT i = 0;
	for(list_entry *entry = phead->next; entry != phead; entry = entry->next, ++i);
	return i;
}

static inline void list_add(list_entry *newnode, list_head *head)
{
	__list_add(newnode, head, head->next);
}
static inline void list_add_tail(list_entry *newnode, list_head *head)
{
	__list_add(newnode, head->prev, head);
}
static inline void list_insert_after(list_entry *newnode, list_entry *pos)
{
	__list_add(newnode, pos, pos->next);
}
static inline void list_insert_before(list_entry *newnode, list_entry *pos)
{
	__list_add(newnode, pos->prev, pos);
}
static inline void list_remove(list_entry *entry)
{
	__list_del(entry->prev, entry->next);
}
static inline void list_add_single(list_entry *newnode, list_head *head) // Single direction link. (FILO)
{
	newnode->next = head->next;
	head->next = newnode;
}


class XStack
{
public:

	enum
	{
		OPERATION_LOCK_VALUE = 1, // Must sure no object is in this address.
	};

	XStack()
	{
		m_pStackHead = NULL;
		ASSERT(IsAligned(&m_pStackHead, sizeof(void*)));
	}

	struct stRefObj
	{
		void *pNext;
	};

	void* PopObject()
	{
		stRefObj *pRefObj;

		for(; 1;)
		{
			pRefObj = (stRefObj*)InterlockedExchangePointer((void**)&m_pStackHead, (void*)OPERATION_LOCK_VALUE);
			if(!pRefObj)
			{
				m_pStackHead = 0;
				break;
			}
			if(pRefObj == (void*)OPERATION_LOCK_VALUE)
				continue;

			m_pStackHead = pRefObj->pNext;
			return pRefObj;
		}

		return 0;
	}

	void PushObj(void *pObjEntry)
	{
		stRefObj *pRefHead;
	//	ASSERT(IsAddressValid(pObjEntry));

		for( ; 1;)
		{
			pRefHead = (stRefObj*)m_pStackHead;
			if(pRefHead == (stRefObj*)OPERATION_LOCK_VALUE) // One thead calls GetFreeObj and runs into protected code.
				continue;

			((stRefObj*)pObjEntry)->pNext = pRefHead;
			if(InterlockedCompareExchangePointer((PVOID*)&m_pStackHead, pObjEntry, pRefHead) == pRefHead)
				break;
		}
	}


protected:

	volatile void *m_pStackHead;


};


/* 
// This queue is thread-safe with multiple producer, but works only with one consumer.
// It's user's responsibility to keep enough pointer buffer, or cause bug when two new data wait for the same pointer.
*/ 

class XQueue
{
public:

	enum { QUEUE_SIZE = 64 };

	XQueue()
	{
		UnusedPos = ReadPos = 0;
		ZeroMemory(data, sizeof(data));

		ASSERT(IsAligned((void*)&UnusedPos, sizeof(UnusedPos)));
		ASSERT(IsAligned(data, sizeof(void*)));
	}

	void Add(void *dataIn)
	{
		UINT pos, nextpos;
		for(;;)
		{
			pos = UnusedPos;
			nextpos = (pos == QUEUE_SIZE - 1) ? 0 : pos + 1;
			if(InterlockedCompareExchange((volatile LONG*)&UnusedPos, nextpos, pos) != pos)
				continue;
			while(data[pos] != NULL);
			data[pos] = dataIn;
			break;
		}
	}
	void* Get() // One consumer only.
	{
		void *pOut = (void*)data[ReadPos];
		if(pOut == NULL)
			return NULL;
		data[ReadPos] = NULL;
		if(++ReadPos == QUEUE_SIZE)
			ReadPos = 0;
		return pOut;
	}


protected:

	volatile UINT UnusedPos, ReadPos;
	volatile void *data[QUEUE_SIZE];


};


template <class T>
class TObjectPool
{
public:
	TObjectPool()
	:m_nPool(0), m_nObjCountPerPool(0), m_pPoolArray(0)
	{
	}
	~TObjectPool()
	{
		ASSERT(!m_pPoolArray);
	}

	enum
	{
		OPERATION_LOCK_VALUE = 1, // Must sure no object is in this address.
		DEFAULT_POOL_COUNT = 20,
	};


	struct stObjPool;

	struct stObjEntry
	{
		stObjEntry *pNext;
		stObjPool  *pPool;
		T TData;
	};

	struct stObjPool
	{
	//	stObjPool() {}   // This obj is allocated by malloc so constructor will never be called.
	//	~stObjPool() {}

		void Init(UINT nArraySize)
		{
			stObjEntry *pBaseEntry = GetBaseEntryAddress(), *pEntry = pBaseEntry;
			m_nArraySize = nArraySize--;
			for(; nArraySize--; ++pEntry)
			{
				ASSERT(pEntry != (stObjEntry*)OPERATION_LOCK_VALUE);
				pEntry->pNext = pEntry + 1;
				pEntry->pPool = this;
			}
			pEntry->pNext = 0;
			pEntry->pPool = this;
			m_pStackHead = pBaseEntry;
		}

		T* GetFreeObj()
		{
			stObjEntry *pRefObj;

			for(; 1;)
			{
				pRefObj = (stObjEntry*)InterlockedExchangePointer((PVOID*)&m_pStackHead, (void*)OPERATION_LOCK_VALUE);
				if(!pRefObj)
				{
					m_pStackHead = 0;
					break;
				}
				if(pRefObj == (stObjEntry*)OPERATION_LOCK_VALUE)
					continue;

				m_pStackHead = pRefObj->pNext;
				return &pRefObj->TData;
			}

			return 0;
		}

		void RecycleObj(stObjEntry *pObjEntry)
		{
			stObjEntry *pRefHead;
			ASSERT(IsAddressValid(pObjEntry));

			for( ; 1;)
			{
				pRefHead = (stObjEntry*)m_pStackHead;
				if(pRefHead == (stObjEntry*)OPERATION_LOCK_VALUE) // One thead calls GetFreeObj and runs into protected code.
					continue;

				pObjEntry->pNext = pRefHead;
				if(InterlockedCompareExchangePointer((PVOID*)&m_pStackHead, pObjEntry, pRefHead) == pRefHead)
					break;
			}
		}

		UINT CountFreeObj()
		{
			UINT nCount = 0;
			stObjEntry *pRefHead, *pObj;

			for(; ;)
			{
				pRefHead = (stObjEntry*)InterlockedExchangePointer((PVOID*)&m_pStackHead, (void*)OPERATION_LOCK_VALUE);
				if(!pRefHead)
				{
					m_pStackHead = 0;
					break;
				}
				if(pRefHead == (stObjEntry*)OPERATION_LOCK_VALUE)
					continue;

				for(pObj = pRefHead; pObj; ++nCount)
					pObj = pObj->pNext;

				m_pStackHead = pRefHead;
				break;
			}

			return nCount;
		}

		void InitAllObjects(void (*cb)(T*, UINT)) // This is not thread-safe and can be called only in initial phase.
		{
			stObjEntry *pObj = GetBaseEntryAddress();
			ASSERT(IsAddressValid(pObj) == TRUE);
			for(UINT index = 0; pObj != NULL; pObj = pObj->pNext)
				cb(&pObj->TData, index++);
		}
		BOOL IsAddressValid(stObjEntry *pEntry)
		{
			stObjEntry *pBase = (stObjEntry*)((DWORD_PTR)this + sizeof(stObjPool)), *pLastObj = pBase + (m_nArraySize - 1);
			if(pBase <= pEntry && pEntry <= pLastObj)
				return TRUE;
			return FALSE;
		}

		stObjEntry* GetBaseEntryAddress() { return (stObjEntry*)((DWORD_PTR)this + sizeof(stObjPool)); }


		UINT m_nArraySize;
		volatile stObjEntry *m_pStackHead;

	};


	T* AllocObj()
	{
		T *pOut;
		for(UINT i = 0; i < m_nPool; ++i)
		{
			if(!m_pPoolArray[i] && !AddPool(i))
				break;
			if(pOut = m_pPoolArray[i]->GetFreeObj())
				return pOut;
		}
		return 0;
	}
	void FreeObj(T* pObj)
	{
		stObjEntry *pObjEntry = CONTAINING_RECORD(pObj, stObjEntry, TData);
		pObjEntry->pPool->RecycleObj(pObjEntry);
	}
	T* NewObj()
	{
		T *pOut;
		for(UINT i = 0; i < m_nPool; ++i)
		{
			if(!m_pPoolArray[i] && !AddPool(i))
				break;
			if(pOut = m_pPoolArray[i]->GetFreeObj())
			{
				new(pOut) T();
				return pOut;
			}
		}
		return 0;
	}
	void DeleteObj(T* pObj)
	{
		stObjEntry *pObjEntry = CONTAINING_RECORD(pObj, stObjEntry, TData);
		pObj->~T();
		pObjEntry->pPool->RecycleObj(pObjEntry);
	}


	BOOL CreatePool(UINT nObjCountPerPool, UINT nMaxPoolCount = DEFAULT_POOL_COUNT)
	{
		ASSERT(!m_pPoolArray);
		m_nObjCountPerPool = nObjCountPerPool;
		m_nPool = nMaxPoolCount;

		UINT nSize = sizeof(stObjPool*) * m_nPool;
		m_pPoolArray = (stObjPool**)malloc(nSize);
		if(!m_pPoolArray)
			return FALSE;
		ZeroMemory(m_pPoolArray, nSize);

		return TRUE;
	}
	void ReleasePool()
	{
		if(!m_pPoolArray)
			return;
		for(UINT i = 0; i < m_nPool; i++)
			if(m_pPoolArray[i])
				free(m_pPoolArray[i]);
		free(m_pPoolArray);
		m_pPoolArray = 0;
		m_nPool = 0;
	}

	stObjPool* AddPool(UINT nIndex)
	{
		stObjPool *pPool = NULL;
		m_cs.EnterCriticalSection();
		if(!m_pPoolArray[nIndex])
			if((pPool = AllocPool(m_nObjCountPerPool)) != NULL)
				m_pPoolArray[nIndex] = pPool;
		m_cs.LeaveCriticalSection();
		return pPool;
	}

	stObjPool* AllocPool(UINT nObjCount)
	{
		//printx("Pool Size: %d Bytes.\n", sizeof(stObjPool) + sizeof(stObjEntry) * nObjCount);
		stObjPool *pPool = (stObjPool*)malloc(sizeof(stObjPool) + sizeof(stObjEntry) * nObjCount);
		if(pPool)
		{
			pPool->Init(nObjCount);
			return pPool;
		}
		return 0;
	}

	UINT GetAllocatedPoolCount()
	{
		UINT nCount;
		for(nCount = 0; nCount < m_nPool; ++nCount)
			if(!m_pPoolArray[nCount])
				break;
		return nCount;
	}
	UINT State(UINT &nTotalObj, UINT &nFreeObj)
	{
		UINT nPoolCount;

		for(nFreeObj = 0, nPoolCount = 0; nPoolCount < m_nPool; ++nPoolCount)
			if(m_pPoolArray[nPoolCount])
				nFreeObj += m_pPoolArray[nPoolCount]->CountFreeObj();
			else
				break;

		nTotalObj = nPoolCount * m_nObjCountPerPool;

		return nPoolCount;
	}

	UINT CountFreeObj(UINT nPoolIndex)
	{
		if(nPoolIndex >= m_nPool || !m_pPoolArray[nPoolIndex])
			return 0;
		return m_pPoolArray[nPoolIndex]->CountFreeObj();
	}


protected:

	CCriticalSectionUTX m_cs;
	UINT m_nPool, m_nObjCountPerPool;
	stObjPool **m_pPoolArray;


};


