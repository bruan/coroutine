#include "coroutine_mgr.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/mman.h>
//#define __USE_VALGRIND__
#ifdef __USE_VALGRIND__
#include "valgrind/valgrind.h"
#endif
#endif

#define _MAX_CO_RECYCLE_COUNT	100

namespace coroutine
{
	CCoroutineMgr::CCoroutineMgr()
		: m_pCurrentCoroutine(nullptr)
		, m_pMainContext(nullptr)
		, m_nNextCoroutineID(1)
		, m_nTotalStackSize(0)
#ifndef _WIN32
		, m_pMainStack(nullptr)
		, m_nMainStackSize(0)
		, m_nValgrindID(0)
#endif
	{
	}

	CCoroutineMgr::~CCoroutineMgr()
	{
#ifdef _WIN32
		ConvertFiberToThread();
		this->m_pMainContext = nullptr;
#else
		delete this->m_pMainContext;
		uint32_t nValgrindID = this->m_nValgrindID;
		CCoroutineMgr::freeStack(this->m_pMainStack, this->m_nMainStackSize, nValgrindID);
#endif
	}

	bool CCoroutineMgr::init(uint32_t nStackSize)
	{
#ifdef _WIN32
		this->m_pMainContext = ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH);
		if (this->m_pMainContext == nullptr)
			return false;
#else
		this->m_pMainContext = new context();
		uint32_t nValgrindID = 0;
		this->m_pMainStack = CCoroutineMgr::allocStack(nStackSize, nValgrindID);
		if (nullptr == this->m_pMainStack)
			return false;
		this->m_nMainStackSize = nStackSize;
		this->m_nValgrindID = nValgrindID;
#endif
		this->m_pCurrentCoroutine = nullptr;

		return true;
	}

#ifndef _WIN32
	char* CCoroutineMgr::getMainStack() const
	{
		return this->m_pMainStack + this->m_nMainStackSize;
	}
#endif

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

	void* CCoroutineMgr::getMainContext() const
	{
		return this->m_pMainContext;
	}

	CCoroutineImpl* CCoroutineMgr::createCoroutine(uint32_t nStackSize, const std::function<void(uint64_t)>& callback)
	{
		if (callback == nullptr)
			return nullptr;

#ifdef _WIN32
		if (nStackSize == 0)
			nStackSize = 64 * 1024;
#endif

		uint32_t nPageSize = CCoroutineMgr::getPageSize();
		nStackSize = (nStackSize + nPageSize - 1) / nPageSize * nPageSize;

		for (auto iter = this->m_listRecycleCoroutine.begin(); iter != this->m_listRecycleCoroutine.end(); ++iter)
		{
			CCoroutineImpl* pCoroutineImpl = *iter;
			if ((pCoroutineImpl->getStackSize() == 0 && nStackSize == 0) || std::abs((int32_t)pCoroutineImpl->getStackSize() - (int32_t)nStackSize) <= (int32_t)(10 * nPageSize))
			{
				pCoroutineImpl->setCallback(callback);
				pCoroutineImpl->setState(eCS_SUSPEND);
				this->m_listRecycleCoroutine.erase(iter);
				this->recycle();
				this->m_mapCoroutine[pCoroutineImpl->getCoroutineID()] = pCoroutineImpl;
				this->m_nTotalStackSize += nStackSize;
				return pCoroutineImpl;
			}
		}

		CCoroutineImpl* pCoroutineImpl = new CCoroutineImpl();
		if (!pCoroutineImpl->init(this->m_nNextCoroutineID++, nStackSize, callback))
		{
			delete pCoroutineImpl;
			return nullptr;
		}

		this->m_mapCoroutine[pCoroutineImpl->getCoroutineID()] = pCoroutineImpl;
		this->m_nTotalStackSize += nStackSize;
		return pCoroutineImpl;
	}

	CCoroutineImpl* CCoroutineMgr::getCoroutine(uint64_t nID) const
	{
		auto iter = this->m_mapCoroutine.find(nID);
		if (iter == this->m_mapCoroutine.end())
			return nullptr;

		return iter->second;
	}

	uint32_t CCoroutineMgr::getCoroutineCount() const
	{
		return (uint32_t)this->m_mapCoroutine.size();
	}

	uint64_t CCoroutineMgr::getTotalStackSize() const
	{
		return (uint64_t)this->m_nTotalStackSize;
	}

	void CCoroutineMgr::addRecycleCoroutine(CCoroutineImpl* pCoroutineImpl)
	{
		if (pCoroutineImpl == nullptr)
			return;

		this->m_mapCoroutine.erase(pCoroutineImpl->getCoroutineID());
		this->m_nTotalStackSize -= pCoroutineImpl->getStackSize();

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

#ifndef _WIN32
	char* CCoroutineMgr::allocStack(uint32_t& nStackSize, uint32_t& nValgrindID)
	{
		uint32_t nPageSize = CCoroutineMgr::getPageSize();
		nStackSize = (nStackSize + nPageSize - 1) / nPageSize * nPageSize;

		char* pStack = reinterpret_cast<char*>(mmap(NULL, nStackSize + 2 * nPageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0));
		if (pStack == (void*)MAP_FAILED)
			return nullptr;

#ifdef __USE_VALGRIND__
		nValgrindID = VALGRIND_STACK_REGISTER(pStack + nPageSize, pStack + nPageSize + nStackSize);
#endif

		mprotect(pStack, nPageSize, PROT_NONE);
		mprotect(pStack + nPageSize + nStackSize, nPageSize, PROT_NONE);

		return pStack + nPageSize;
	}

	void CCoroutineMgr::freeStack(char* pStack, uint32_t nStackSize, uint32_t nValgrindID)
	{
		uint32_t nPageSize = CCoroutineMgr::getPageSize();

		munmap(pStack - nPageSize, nStackSize + 2 * nPageSize);

#ifdef __USE_VALGRIND__
		VALGRIND_STACK_DEREGISTER(nValgrindID);
#endif
	}
#endif

#if defined(_MSC_VER) && _MSC_VER < 1900
# define thread_local	__declspec(thread)
#endif

	CCoroutineMgr* getCoroutineMgr()
	{
		static thread_local CCoroutineMgr* s_Inst = nullptr;

		if (nullptr == s_Inst)
			s_Inst = new CCoroutineMgr();

		return s_Inst;
	}
}