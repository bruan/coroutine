#pragma once

#include "coroutine.h"

#include <map>

namespace coroutine
{
	union context
	{
		struct
		{
			int64_t rbx;
			int64_t rsp;
			int64_t rbp;
			int64_t r12;
			int64_t r13;
			int64_t r14;
			int64_t r15;
			int64_t rip;
		};
		int64_t regs[8];
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
		void			setLocalData(const char* szName, void* pData);
		void*			getLocalData(const char* szName) const;
		void			delLocalData(const char* szName);

		uint32_t		getStackSize() const;

		void			setCallback(const std::function<void(uint64_t)>& callback);

	private:
#ifndef _WIN32
		void			saveStack();
		static void		onCallback();
#endif
		static void		onCallback(void* pParm);

	private:
		uint64_t						m_nID;
		std::function<void(uint64_t)>	m_callback;
		uint64_t						m_nContext;
		ECoroutineState					m_eState;
		std::map<std::string, void*>	m_mapLocalData;
		void*							m_pContext;
		uintptr_t						m_nStackSize;

#ifndef _WIN32
		bool							m_bOwnerStack;
		char*							m_pStack;
		uintptr_t						m_nStackCap;
		int32_t							m_nValgrindID;
#endif
	};
}