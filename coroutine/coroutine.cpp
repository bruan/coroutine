#include "coroutine.h"
#include "coroutine_impl.h"
#include "coroutine_mgr.h"

namespace coroutine
{
	void resume(uint64_t nID, uint64_t nContext)
	{
		CCoroutineImpl* pCoroutineImpl = CCoroutineMgr::Inst()->getCoroutine(nID);
		if (nullptr == pCoroutineImpl)
			return;

		pCoroutineImpl->resume(nContext);
	}

	uint64_t yield()
	{
		return CCoroutineMgr::Inst()->getCurrentCoroutine()->yield();
	}

	uint32_t getState(uint64_t nID)
	{
		CCoroutineImpl* pCoroutineImpl = CCoroutineMgr::Inst()->getCoroutine(nID);
		if (nullptr == pCoroutineImpl)
			return eCS_DEAD;

		return pCoroutineImpl->getState();
	}

	uint64_t getCurrentID()
	{
		return CCoroutineMgr::Inst()->getCurrentCoroutine()->getCoroutineID();
	}

	uint64_t create(uint32_t nStackSize, const std::function<void(uint64_t)>& fn)
	{
		CCoroutineImpl* pCoroutineImpl = CCoroutineMgr::Inst()->createCoroutine(nStackSize, fn);
		if (nullptr == pCoroutineImpl)
			return 0;

		return pCoroutineImpl->getCoroutineID();
	}

	void close(uint64_t nID)
	{
		CCoroutineImpl* pCoroutineImpl = CCoroutineMgr::Inst()->getCurrentCoroutine();
		if (nullptr == pCoroutineImpl)
			return;

		if (pCoroutineImpl->getCoroutineID() == nID)
			return;

		pCoroutineImpl = CCoroutineMgr::Inst()->getCoroutine(nID);
		if (nullptr == pCoroutineImpl)
			return;
	}

	void sendMessage(uint64_t nID, void* pData)
	{
		CCoroutineImpl* pCoroutineImpl = CCoroutineMgr::Inst()->getCoroutine(nID);
		if (nullptr == pCoroutineImpl)
			return;

		pCoroutineImpl->sendMessage(pData);
	}

	void* recvMessage(uint64_t nID)
	{
		CCoroutineImpl* pCoroutineImpl = CCoroutineMgr::Inst()->getCoroutine(nID);
		if (nullptr == pCoroutineImpl)
			return nullptr;

		return pCoroutineImpl->recvMessage();
	}

	void init(uint32_t nStackSize)
	{
		CCoroutineMgr::Inst()->init(nStackSize);
	}

}