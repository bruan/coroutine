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

		bool			init(uint32_t nStackSize);

		CCoroutineImpl*	createCoroutine(uint32_t nStackSize, const std::function<void(uint64_t)>& callback);
		CCoroutineImpl*	getCoroutine(uint64_t nID) const;
		void			addRecycleCoroutine(CCoroutineImpl* pCoroutineImpl);
		uint32_t		getCoroutineCount() const;
		uint64_t		getTotalStackSize() const;

		void			setCurrentCoroutine(CCoroutineImpl* pCoroutineImpl);
		CCoroutineImpl*	getCurrentCoroutine() const;

		void*			getMainContext() const;

		static uint32_t	getPageSize();

#ifndef _WIN32
		char*			getMainStack() const;

		static char*	allocStack(uint32_t& nStackSize, uint32_t& nValgrindID);
		static void		freeStack(char* pStack, uint32_t nStackSize, uint32_t nValgrindID);
#endif

	private:
		void			recycle();
		
	private:
		int64_t									m_nTotalStackSize;
		uint64_t								m_nNextCoroutineID;
		std::map<uint64_t, CCoroutineImpl*>		m_mapCoroutine;
		std::list<CCoroutineImpl*>				m_listRecycleCoroutine;
		CCoroutineImpl*							m_pCurrentCoroutine;
		void*									m_pMainContext;
#ifndef _WIN32
		char*									m_pMainStack;
		uint32_t								m_nMainStackSize;
		int32_t									m_nValgrindID;
#endif
	};

	CCoroutineMgr* getCoroutineMgr();
}