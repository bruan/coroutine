#include "coroutine.h"
#include "coroutine_impl.h"
#include "coroutine_mgr.h"

namespace coroutine
{
	void resume(uint64_t nID, uint64_t nContext)
	{
		CCoroutineImpl* pCoroutineImpl = getCoroutineMgr()->getCoroutine(nID);
		if (nullptr == pCoroutineImpl)
			return;

		pCoroutineImpl->resume(nContext);
	}

	uint64_t yield()
	{
		return getCoroutineMgr()->getCurrentCoroutine()->yield();
	}

	uint32_t getState(uint64_t nID)
	{
		CCoroutineImpl* pCoroutineImpl = getCoroutineMgr()->getCoroutine(nID);
		if (nullptr == pCoroutineImpl)
			return eCS_DEAD;

		return pCoroutineImpl->getState();
	}

	uint64_t getCurrentID()
	{
		CCoroutineImpl* pCoroutineImpl = getCoroutineMgr()->getCurrentCoroutine();
		if (nullptr == pCoroutineImpl)
			return 0;

		return pCoroutineImpl->getCoroutineID();
	}

	uint64_t create(uint32_t nStackSize, const std::function<void(uint64_t)>& fn)
	{
		CCoroutineImpl* pCoroutineImpl = getCoroutineMgr()->createCoroutine(nStackSize, fn);
		if (nullptr == pCoroutineImpl)
			return 0;

		return pCoroutineImpl->getCoroutineID();
	}

	void close(uint64_t nID)
	{
		CCoroutineMgr* pCoroutineMgr = getCoroutineMgr();
		CCoroutineImpl* pCoroutineImpl = pCoroutineMgr->getCurrentCoroutine();
		if (nullptr == pCoroutineImpl)
			return;

		if (pCoroutineImpl->getCoroutineID() == nID)
			return;

		pCoroutineImpl = pCoroutineMgr->getCoroutine(nID);
		if (nullptr == pCoroutineImpl)
			return;
	}

	void setLocalData(uint64_t nID, const char* szName, void* pData)
	{
		CCoroutineImpl* pCoroutineImpl = getCoroutineMgr()->getCoroutine(nID);
		if (nullptr == pCoroutineImpl)
			return;

		pCoroutineImpl->setLocalData(szName, pData);
	}

	void* getLocalData(uint64_t nID, const char* szName)
	{
		CCoroutineImpl* pCoroutineImpl = getCoroutineMgr()->getCoroutine(nID);
		if (nullptr == pCoroutineImpl)
			return nullptr;

		return pCoroutineImpl->getLocalData(szName);
	}

	void delLocalData(uint64_t nID, const char* szName)
	{
		CCoroutineImpl* pCoroutineImpl = getCoroutineMgr()->getCoroutine(nID);
		if (nullptr == pCoroutineImpl)
			return;

		pCoroutineImpl->delLocalData(szName);
	}

	void init(uint32_t nStackSize)
	{
		getCoroutineMgr()->init(nStackSize);
	}

}