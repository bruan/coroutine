#pragma once

#include "coroutine_impl.h"

#include <map>
#include <list>

namespace coroutine
{
	class CCoroutineMgr
	{
	public:
		CCoroutineMgr();
		~CCoroutineMgr();

		static CCoroutineMgr* Inst();

		bool			init(uint32_t nStackSize);

		char*			getMainStack() const;

		CCoroutineImpl*	createCoroutine(uint32_t nStackSize, const std::function<void(uint64_t)>& callback);
		CCoroutineImpl*	getCoroutine(uint64_t nID) const;
		void			addRecycleCoroutine(CCoroutineImpl* pCoroutineImpl);

		void			setCurrentCoroutine(CCoroutineImpl* pCoroutineImpl);
		CCoroutineImpl*	getCurrentCoroutine() const;

		context*		getMainContext() const;

		static char*	allocStack(uint32_t& nStackSize, uint32_t& nValgrindID);
		static void		freeStack(char* pStack, uint32_t nStackSize, uint32_t nValgrindID);

	private:
		void			recycle();
		
	private:
		uint64_t								m_nNextCoroutineID;
		std::map<uint64_t, CCoroutineImpl*>		m_mapCoroutine;
		std::list<CCoroutineImpl*>				m_listRecycleCoroutine;
		CCoroutineImpl*							m_pCurrentCoroutine;
		context*								m_pMainContext;
		char*									m_pMainStack;
		uint32_t								m_nMainStackSize;
#ifndef _WIN32
		int32_t									m_nValgrindID;
#endif
	};
}