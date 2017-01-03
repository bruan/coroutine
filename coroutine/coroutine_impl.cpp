#include "coroutine_impl.h"
#include "coroutine_mgr.h"

#include <algorithm>

#include <string.h>
#include <assert.h>

extern "C" int32_t	save_context(int64_t* reg);
extern "C" void		restore_context(int64_t* reg, int32_t);

namespace coroutine
{
	// 这段代码必须是单独的函数，如果放在save_context返回非0那里，就会出现局部变量栈溢出的问题
	void CCoroutineImpl::onCallback()
	{
		while (true)
		{
			CCoroutineImpl* pCoroutineImpl = CCoroutineMgr::Inst()->getCurrentCoroutine();
			pCoroutineImpl->m_callback(pCoroutineImpl->m_nContext);
			CCoroutineMgr::Inst()->addRecycleCoroutine(pCoroutineImpl);

			CCoroutineMgr::Inst()->setCurrentCoroutine(nullptr);

			pCoroutineImpl->m_eState = eCS_DEAD;
			pCoroutineImpl->m_nStackSize = 0;
			if (save_context(pCoroutineImpl->m_ctx.regs) == 0)
				restore_context(CCoroutineMgr::Inst()->getMainContext()->regs, 1);
		}
	}
	
	void CCoroutineImpl::saveStack()
	{
		char* pStack = CCoroutineMgr::Inst()->getMainStack();
		char nDummy = 0;

		if (this->m_nStackCap < (uintptr_t)(pStack - &nDummy))
		{
			delete [] this->m_pStack;
			this->m_nStackCap = pStack - &nDummy;
			this->m_pStack= new char[this->m_nStackCap];
		}
		this->m_nStackSize = pStack - &nDummy;
		memcpy(this->m_pStack, &nDummy, this->m_nStackSize);
	}

	CCoroutineImpl::CCoroutineImpl()
		: m_nID(0)
		, m_nContext(0)
		, m_eState(eCS_NONE)
		, m_nStackCap(0)
		, m_nStackSize(0)
		, m_pStack(nullptr)
		, m_bOwnerStack(false)
#ifndef _WIN32
		, m_nValgrindID(0)
#endif
	{

	}

	CCoroutineImpl::~CCoroutineImpl()
	{
		uint32_t nValgrindID = 0;
#ifndef _WIN32
		nValgrindID = this->m_nValgrindID;
#endif
		if (this->m_bOwnerStack)
			CCoroutineMgr::freeStack(this->m_pStack, (uint32_t)this->m_nStackSize, nValgrindID);
		else
			delete[]this->m_pStack;
	}

	bool CCoroutineImpl::init(uint64_t nID, uint32_t nStackSize, const std::function<void(uint64_t)>& callback)
	{
		if (this->m_eState != eCS_NONE)
			return false;

		if (callback == nullptr)
			return false;

		if (save_context(this->m_ctx.regs) != 0)
		{
			CCoroutineImpl::onCallback();

			// 不可能执行到这里的
			//assert(0);
		}
		if (nStackSize != 0)
			this->m_bOwnerStack = true;
		
		if (nStackSize == 0)
		{
			this->m_bOwnerStack = false;
			this->m_ctx.rsp = (uintptr_t)CCoroutineMgr::Inst()->getMainStack();
#ifdef _WIN32
			this->m_ctx.gs = this->m_ctx.rsp;
#endif
		}
		else
		{
			this->m_bOwnerStack = true;
			uint32_t nValgrindID = 0;
			char* pStack = CCoroutineMgr::allocStack(nStackSize, nValgrindID);
			if (nullptr == pStack)
				return false;
			this->m_pStack = pStack + nStackSize;
			this->m_nStackSize = nStackSize;
#ifdef _WIN32
			this->m_ctx.rsp = (uintptr_t)this->m_pStack - CCoroutineMgr::Inst()->getPageSize();
			this->m_ctx.gs = this->m_ctx.rsp;
#else
			this->m_ctx.rsp = (uintptr_t)this->m_pStack;
			this->m_nValgrindID = nValgrindID;
#endif
		}

		this->m_callback = callback;
		this->m_nContext = 0;
		this->m_eState = eCS_READY;
		this->m_nID = nID;
		
		return true;
	}

	uint64_t CCoroutineImpl::yield()
	{
		if (this->m_eState != eCS_RUNNING)
			return 0;

		CCoroutineMgr::Inst()->setCurrentCoroutine(nullptr);

		this->m_eState = eCS_SUSPEND;

		if (!this->m_bOwnerStack)
			this->saveStack();
		
		if (save_context(this->m_ctx.regs) == 0)
			restore_context(CCoroutineMgr::Inst()->getMainContext()->regs, 1);
		
		return this->m_nContext;
	}
	
	void CCoroutineImpl::resume(uint64_t nContext)
	{
		if (this->getState() != eCS_READY && this->getState() != eCS_SUSPEND)
			return;

		this->m_eState = eCS_RUNNING;
		
		CCoroutineMgr::Inst()->setCurrentCoroutine(this);
		this->m_nContext = nContext;

		if (!this->m_bOwnerStack)
			memcpy(CCoroutineMgr::Inst()->getMainStack() - this->m_nStackSize, this->m_pStack, this->m_nStackSize);

		if (save_context(CCoroutineMgr::Inst()->getMainContext()->regs) == 0)	
			restore_context(this->m_ctx.regs, 1);
	}

	uint32_t CCoroutineImpl::getState() const
	{
		return this->m_eState;
	}

	void CCoroutineImpl::setState(uint32_t nState)
	{
		this->m_eState = (ECoroutineState)nState;
	}

	uint64_t CCoroutineImpl::getCoroutineID() const
	{
		return this->m_nID;
	}

	void CCoroutineImpl::sendMessage(void* pData)
	{
		this->m_listMessage.push_back(pData);
	}

	void* CCoroutineImpl::recvMessage()
	{
		if (this->m_listMessage.empty())
			return nullptr;

		void* pData = this->m_listMessage.front();
		this->m_listMessage.pop_front();

		return pData;
	}

	void CCoroutineImpl::setCallback(const std::function<void(uint64_t)>& callback)
	{
		this->m_callback = callback;
	}

}