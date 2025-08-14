#pragma once
#include "base/base_IAwaiter.h"
#include "base/base_Task.h"

// --------------------------------------------------------------

namespace base
{

// --------------------------------------------------------------

class promise_base
{
protected:
	promise_base() noexcept;
	/* non-virtual */ ~promise_base() noexcept;

public:
	promise_base(const promise_base&) noexcept              = delete;
	promise_base& operator=(const promise_base&) noexcept   = delete;
	promise_base(promise_base&& source) noexcept;
	promise_base& operator=(promise_base&&) noexcept        = delete; // TODO : ムーブ代入は許可する？

	// --------------------
	// suspend
public:
	auto initial_suspend() { return std::suspend_always{}; }
	auto final_suspend() noexcept { return std::suspend_always{}; }

	// --------------------
	// TODO : 例外のサポート
public:
	void unhandled_exception()
	{
		// nop
	}

	// --------------------
	// 子のawaiterの管理
public:
	bool resume_sub_awaiter()
	{
		if (!m_SubAwaiter)
		{
			return false;
		}

		m_SubAwaiter->resume();

		if (m_SubAwaiter->done())
		{
			m_SubAwaiter = nullptr;
			return false;
		}

		return true;
	}

	void bind_sub_awaiter(IAwaiter* awaiter) noexcept
	{
		m_SubAwaiter = awaiter;
	}

private:
	IAwaiter* m_SubAwaiter = nullptr;
};

// --------------------------------------------------------------
promise_base::promise_base() noexcept
	: m_SubAwaiter(nullptr)
{
}

promise_base::~promise_base() noexcept
{
}

promise_base::promise_base(promise_base&& source) noexcept
{
}

// --------------------------------------------------------------

template <class ResultT>
class promise final : public promise_base
{
public:
	using result_type = ResultT;
	using task_type   = task<result_type>;
	using handle_type = task_type::handle_type;

public:
	promise() noexcept  = default;
	~promise() noexcept = default;

public:
	promise(const promise&) noexcept            = delete;
	promise& operator=(const promise&) noexcept = delete;
	promise(promise&&) noexcept                 = default; // TODO: 実装検討
	promise& operator=(promise&&) noexcept      = delete; // TODO : ムーブ代入は許可する？

public:
	static auto get_return_object_on_allocation_failure() { return task_type{ nullptr }; }
	auto get_return_object() { return task_type{ handle_type::from_promise(*this) }; }

	// --------------------
	// co_return サポート
public:
	void return_value(const result_type& value)
	{
		m_ResultHolder = std::make_unique<result_type>(value);
		m_HasResult = true;
	}

	void return_value(result_type&& value)
	{
		m_ResultHolder = std::make_unique<result_type>(std::move(value));
		m_HasResult = true;
	}

	template <class... Args>
	void return_value(Args&&... args)
	{
		m_ResultHolder = std::make_unique<result_type>(std::forward<Args>(args)...);
		m_HasResult = true;
	}

	// --------------------
	// 結果の取得
public:
	result_type consume_result()
	{
        // TODO: 未完了、使用済みをはじく
		
		m_IsResultConsumed = true;

		// TODO:例外時は一応通し消費済みにする？
		// その後に再スロー
		return std::move(*m_ResultHolder);
	}

private:
	std::unique_ptr<result_type>    m_ResultHolder      = {};    // TODO : アロケート抑制
	bool	                        m_HasResult         = false; // TODO : ステータス化？
	bool	                        m_IsResultConsumed  = false; // Result has been consumed
};

// --------------------------------------------------------------

template <>
class promise<void> final : public promise_base
{
public:
	using result_type	= void;
	using task_type		= task<result_type>;
	using handle_type	= task_type::handle_type;

public:
	promise() noexcept  = default;
	~promise() noexcept = default;

public:
	promise(const promise&) noexcept            = delete;
	promise& operator=(const promise&) noexcept = delete;
	promise(promise&&) noexcept                 = default; // TODO: 実装検討
	promise& operator=(promise&&) noexcept      = delete; // TODO : ムーブ代入は許可する？

public:
	static auto get_return_object_on_allocation_failure() { return task_type{ nullptr }; }
	auto get_return_object() { return task_type{ handle_type::from_promise(*this) }; }

	// --------------------
	// co_return サポート
public:
	void return_void()
	{
		m_HasResult = true;
	}

	// --------------------
	// 結果の取得
public:
	result_type consume_result()
	{
		// TODO: 未完了、使用済みをはじく

		m_IsResultConsumed = true;

		// TODO:例外時は一応通し消費済みにする？
		// その後に再スロー
	}

private:
	bool m_HasResult        = false; // TODO : ステータス化？
	bool m_IsResultConsumed = false; // Result has been consumed
};

// --------------------------------------------------------------

} // namespace base
