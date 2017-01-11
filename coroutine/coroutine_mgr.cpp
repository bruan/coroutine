#include "coroutine_mgr.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/mman.h>
#include "valgrind/valgrind.h"
#endif

#define _MAX_CO_RECYCLE_COUNT	100

namespace coroutine
{
	CCoroutineMgr::CCoroutineMgr()
		: m_pCurrentCoroutine(nullptr)
		, m_pMainContext(new context())
		, m_pMainStack(nullptr)
		, m_nMainStackSize(0)
		, m_nNextCoroutineID(1)
#ifndef _WIN32
		, m_nValgrindID(0)
#endif
	{
	}

	CCoroutineMgr::~CCoroutineMgr()
	{
		delete this->m_pMainContext;

		uint32_t nValgrindID = 0;
#ifndef _WIN32
		nValgrindID = this->m_nValgrindID;
#endif
		CCoroutineMgr::freeStack(this->m_pMainStack, this->m_nMainStackSize, nValgrindID);
	}

	bool CCoroutineMgr::init(uint32_t nStackSize)
	{
		uint32_t nValgrindID = 0;
		this->m_pMainStack = CCoroutineMgr::allocStack(nStackSize, nValgrindID);
		if (nullptr == this->m_pMainStack)
			return false;
		this->m_nMainStackSize = nStackSize;
#ifndef _WIN32
		this->m_nValgrindID = nValgrindID;
#endif
		this->m_pCurrentCoroutine = nullptr;

		return true;
	}

	char* CCoroutineMgr::getMainStack() const
	{
#ifdef _WIN32
		return this->m_pMainStack + this->m_nMainStackSize - CCoroutineMgr::getPageSize();
#else
		return this->m_pMainStack + this->m_nMainStackSize;
#endif
	}

	CCoroutineImpl* CCoroutineMgr::getCurrentCoroutine() const
	{
		return this->m_pCurrentCoroutine;
	}

	void CCoroutineMgr::setCurrentCoroutine(CCoroutineImpl* pCoroutineImpl)
	{
		if (pCoroutineImpl == nullptr)
			return;

		this->m_pCurrentCoroutine = pCoroutineImpl;
	}

	context* CCoroutineMgr::getMainContext() const
	{
		return this->m_pMainContext;
	}

	CCoroutineImpl* CCoroutineMgr::createCoroutine(uint32_t nStackSize, const std::function<void(uint64_t)>& callback)
	{
		if (callback == nullptr)
			return nullptr;

		CCoroutineImpl* pCoroutine = nullptr;
		if (!this->m_listRecycleCoroutine.empty())
		{
			this->recycle();

			pCoroutine = this->m_listRecycleCoroutine.front();
			this->m_listRecycleCoroutine.pop_front();

			pCoroutine->setCallback(callback);
			pCoroutine->setState(eCS_SUSPEND);
		}
		else
		{
			pCoroutine = new CCoroutineImpl();
			if (!pCoroutine->init(this->m_nNextCoroutineID++, nStackSize, callback))
			{
				delete pCoroutine;
				return nullptr;
			}
		}

		this->m_mapCoroutine[pCoroutine->getCoroutineID()] = pCoroutine;

		return pCoroutine;
	}

	CCoroutineImpl* CCoroutineMgr::getCoroutine(uint64_t nID) const
	{
		auto iter = this->m_mapCoroutine.find(nID);
		if (iter == this->m_mapCoroutine.end())
			return nullptr;

		return iter->second;
	}

	void CCoroutineMgr::addRecycleCoroutine(CCoroutineImpl* pCoroutineImpl)
	{
		if (pCoroutineImpl == nullptr)
			return;

		this->m_mapCoroutine.erase(pCoroutineImpl->getCoroutineID());

		this->m_listRecycleCoroutine.push_back(pCoroutineImpl);
	}

	void CCoroutineMgr::recycle()
	{
		while (this->m_listRecycleCoroutine.size() > _MAX_CO_RECYCLE_COUNT)
		{
			CCoroutineImpl* pCoroutineImpl = *this->m_listRecycleCoroutine.begin();
			delete(pCoroutineImpl);

			this->m_listRecycleCoroutine.pop_front();
		}
	}

	uint32_t CCoroutineMgr::getPageSize()
	{
		struct SPageSize 
		{
			SPageSize()
			{
#ifdef _WIN32
				SYSTEM_INFO systemInfo;
				GetSystemInfo(&systemInfo);
				nPageSize = systemInfo.dwPageSize;
#else
				nPageSize = getpagesize();
#endif
			}

			uint32_t	nPageSize;
		};

		static SPageSize sPageSize;

		return sPageSize.nPageSize;
	}

	char* CCoroutineMgr::allocStack(uint32_t& nStackSize, uint32_t& nValgrindID)
	{
		uint32_t nPageSize = CCoroutineMgr::getPageSize();
		nStackSize = (nStackSize + nPageSize - 1) / nPageSize * nPageSize;

#ifdef _WIN32
		char* pStack = reinterpret_cast<char*>(::VirtualAlloc(nullptr, nStackSize + 2 * nPageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
		if (nullptr == pStack)
			return nullptr;

		DWORD dwProtect = 0;
		if (!::VirtualProtect(pStack, nPageSize, PAGE_NOACCESS, &dwProtect))
		{
			::VirtualFree(pStack, nStackSize + 2 * nPageSize, MEM_RELEASE);
			return nullptr;
		}

		if (!::VirtualProtect(pStack + nPageSize + nStackSize, nPageSize, PAGE_NOACCESS, &dwProtect))
		{
			::VirtualFree(pStack, nStackSize + 2 * nPageSize, MEM_RELEASE);
			return nullptr;
		}
		return pStack + nPageSize;
#else
		char* pStack = reinterpret_cast<char*>(mmap(NULL, nStackSize + 2 * nPageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0));
		if (pStack == (void*)MAP_FAILED)
			return nullptr;

		nValgrindID = VALGRIND_STACK_REGISTER(pStack + nPageSize, pStack + nPageSize + nStackSize);

		mprotect(pStack, nPageSize, PROT_NONE);
		mprotect(pStack + nPageSize + nStackSize, nPageSize, PROT_NONE);

		return pStack + nPageSize;
#endif
	}

	void CCoroutineMgr::freeStack(char* pStack, uint32_t nStackSize, uint32_t nValgrindID)
	{
		uint32_t nPageSize = CCoroutineMgr::getPageSize();

#ifdef _WIN32
		::VirtualFree(pStack - nPageSize, nStackSize + 2 * nPageSize, MEM_RELEASE);
#else
		munmap(pStack - nPageSize, nStackSize + 2 * nPageSize);
		VALGRIND_STACK_DEREGISTER(nValgrindID);
#endif
	}

#if defined(_MSC_VER) && _MSC_VER < 1900
# define thread_local	__declspec(thread)
#endif

	CCoroutineMgr* getCoroutineMgr()
	{
		static thread_local CCoroutineMgr* s_Inst = nullptr;

		if (nullptr == s_Inst)
			s_Inst = new thread_local CCoroutineMgr();

		return s_Inst;
	}
}