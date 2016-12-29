#pragma once

#include "coroutine.h"

#include <list>

namespace coroutine
{
	union context
	{
		struct
		{
			int64_t rbx;
			int64_t rsp;
			int64_t rbp;
#ifdef _WIN32
			int64_t gs;
#endif
			int64_t r12;
			int64_t r13;
			int64_t r14;
			int64_t r15;
			int64_t rip;
		};
#ifdef _WIN32
		int64_t regs[9];
#else
		int64_t regs[8];
#endif
	};

	class CCoroutineMgr;
	class CCoroutineImpl
	{
	public:
		CCoroutineImpl();
		~CCoroutineImpl();

		bool			init(uint64_t nID, uint32_t nStackSize, const std::function<void(uint64_t)>& callback);
		uint64_t		yield();
		void			resume(uint64_t nContext);
		uint32_t		getState() const;
		void			setState(uint32_t nState);
		uint64_t		getCoroutineID() const;
		void			sendMessage(void* pData);
		void*			recvMessage();

		void			setCallback(const std::function<void(uint64_t)>& callback);

	private:
		void			saveStack();
		static void		onCallback();

	private:
		uint64_t						m_nID;
		std::function<void(uint64_t)>	m_callback;
		uint64_t						m_nContext;
		context							m_ctx;
		bool							m_bOwnerStack;
#ifndef _WIN32
		int32_t							m_nValgrindID;
#endif
		char*							m_pStack;
		uintptr_t						m_nStackSize;
		uintptr_t						m_nStackCap;
		ECoroutineState					m_eState;
		std::list<void*>				m_listMessage;
	};
}