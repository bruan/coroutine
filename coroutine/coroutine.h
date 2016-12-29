#pragma once

#include <stdint.h>
#include <functional>

namespace coroutine
{
	enum ECoroutineState
	{
		eCS_NONE,
		eCS_DEAD,	// 死亡状态
		eCS_READY,	// 等待执行（重来都没有被执行过）
		eCS_RUNNING,// 执行状态
		eCS_SUSPEND,// 挂起状态
	};

	void		init(uint32_t nStackSize);
	/**
	@brief: 启动协程，传入协程入口函数
	如果传入的栈大小是0的话就用共享栈，否则就用独立栈
	*/
	uint64_t	create(uint32_t nStackSize, const std::function<void(uint64_t)>& fn);
	/**
	@brief: 关闭指定协程
	*/
	void		close(uint64_t nID);
	/**
	@brief: 恢复某一个协程执行，可以传一个参数，yield函数返回后会取到，如果协程第一次执行就是入口函数的参数
	*/
	void		resume(uint64_t nID, uint64_t nContext);
	/**
	@brief: 挂起当前执行的协程，在协程下次会执行时会返回resume函数传给他的参数
	*/
	uint64_t	yield();
	/**
	@brief: 获取协程状态
	*/
	uint32_t	getState(uint64_t nID);
	/**
	@brief: 向指定协程发送消息
	*/
	void		sendMessage(uint64_t nID, void* pData);
	/**
	@brief: 取指定协程接受消息
	*/
	void*		recvMessage(uint64_t nID);
	/**
	@brief: 获取当前执行协程的ID
	*/
	uint64_t	getCurrentID();
};