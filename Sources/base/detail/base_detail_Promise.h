#pragma once
#include "base/base_Cancellation.h"

// --------------------------------------------------------------

namespace base::detail
{

// --------------------------------------------------------------

enum class PromiseStatus
{
	Executing, // 実行中
	Succeeded, // 実行完了
	Faulted,   // 例外発生
	Canceled,  // キャンセル終了
	Consumed,  // 結果を取得済み
};

// --------------------------------------------------------------

class promise_base
{
protected:
	class continuation_resumer;

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
	inline auto initial_suspend() noexcept;
	inline auto final_suspend() noexcept;

	// --------------------
	// 例外のハンドリング
public:
	void unhandled_exception()
	{
		m_Status = PromiseStatus::Faulted;
		// TODO: 例外が使える環境では必要に応じてハンドリング
	}

	// --------------------
	// 継続(戻り先)の制御
public:
	void set_continuation(std::coroutine_handle<> continuation) noexcept
	{
		m_Continuation = continuation;
	}

	// --------------------
	// Statusの制御
public:
	PromiseStatus get_status() const noexcept { return m_Status; }

	void mark_succeeded() noexcept
	{
		m_Status = PromiseStatus::Succeeded;
	}

	void mark_canceled() noexcept
	{
		m_Status = PromiseStatus::Canceled;
	}

	void mark_consumed() noexcept
	{
		//　TODO： Succeededであることをアサート？
		m_Status = PromiseStatus::Consumed;
	}

	bool is_executing() const noexcept { return m_Status == PromiseStatus::Executing; }
	bool is_succeeded() const noexcept { return m_Status == PromiseStatus::Succeeded; }
	bool is_faulted() const noexcept { return m_Status == PromiseStatus::Faulted; }

private:
	std::coroutine_handle<> m_Continuation  = std::noop_coroutine();
	PromiseStatus           m_Status        = PromiseStatus::Executing;
};

// --------------------------------------------------------------

class promise_base::continuation_resumer final
{
public:
	constexpr bool await_ready() const noexcept 
	{
		return false; 
	}

	template <class PromiseT>
	inline auto await_suspend(std::coroutine_handle<PromiseT> h) noexcept
	{
		promise_base& promise = h.promise();
		return promise.m_Continuation;
	}

	void await_resume() const noexcept {}
};

// --------------------------------------------------------------
promise_base::promise_base() noexcept
	: m_Continuation{ std::noop_coroutine() }
{
}

promise_base::~promise_base() noexcept
{
}

promise_base::promise_base(promise_base&& source) noexcept
	: m_Continuation{ std::exchange(source.m_Continuation, std::noop_coroutine()) }
{
}

// --------------------
// suspend

auto promise_base::initial_suspend() noexcept
{
	return std::suspend_always{};
}

auto promise_base::final_suspend() noexcept
{
	return continuation_resumer{};
}

// --------------------------------------------------------------

template <class ReturnObjectT>
class promise final : public promise_base
{
public:
	using return_object_type   = ReturnObjectT;
	using result_value         = return_object_type::result_value;
	using result_type          = return_object_type::result_type;
	using handle_type          = return_object_type::handle_type;

public:
	promise() noexcept  = default;
	~promise() noexcept = default;

public:
	promise(const promise&) noexcept            = delete;
	promise& operator=(const promise&) noexcept = delete;
	promise(promise&&) noexcept                 = default; // TODO: 実装検討
	promise& operator=(promise&&) noexcept      = delete; // TODO : ムーブ代入は許可する？

public:
	static auto get_return_object_on_allocation_failure() 
	{ 
		return return_object_type{ nullptr }; 
	}
	
	auto get_return_object() 
	{ 
		return return_object_type{ handle_type::from_promise(*this) };
	}

	// --------------------
	// co_return サポート
public:
	void return_value(const result_value& value)
	{
		m_ResultHolder = std::make_unique<result_value>(value);
		mark_succeeded();
	}

	void return_value(result_value&& value)
	{
		m_ResultHolder = std::make_unique<result_value>(std::move(value));
		mark_succeeded();
	}

	template <class... Args>
	void return_value(Args&&... args)
	{
		m_ResultHolder = std::make_unique<result_value>(std::forward<Args>(args)...);
		mark_succeeded();
	}

	// --------------------
	// co_yield サポート
public:
	auto yield_value(cancellation_t)
	{
		mark_canceled();
		return continuation_resumer{};
	}

	// --------------------
	// 結果の取得
public:
	// TODO: 参照返し版も必要？
	// TODO: 戻り値型を検討
	result_type consume_result()
	{
		// TODO: 未完了、使用済み、例外時はちゃんとはじく
		// 現状はnulloptで返すだけで異常検知はしていない
		// キャンセル時は現状のnulloptでもよいかも…？
		if(!is_succeeded())
		{
			return std::nullopt;
		}
		
		mark_consumed();

		// TODO:例外時は一応通し消費済みにする？
		// その後に再スロー
		return std::move(*m_ResultHolder);
	}

private:
	std::unique_ptr<result_value> m_ResultHolder = {};    // TODO : アロケート抑制
};

// --------------------------------------------------------------

template <class ReturnObjectT>
requires std::is_void_v<typename ReturnObjectT::result_type>
class promise<ReturnObjectT> final : public promise_base
{
public:
	using return_object_type   = ReturnObjectT;
	using result_type          = return_object_type::result_type;
	using handle_type          = return_object_type::handle_type;

public:
	promise() noexcept  = default;
	~promise() noexcept = default;

public:
	promise(const promise&) noexcept            = delete;
	promise& operator=(const promise&) noexcept = delete;
	promise(promise&&) noexcept                 = default; // TODO: 実装検討
	promise& operator=(promise&&) noexcept      = delete; // TODO : ムーブ代入は許可する？

public:
	static auto get_return_object_on_allocation_failure()
	{
		return return_object_type{ nullptr };
	}

	auto get_return_object()
	{
		return return_object_type{ handle_type::from_promise(*this) };
	}

	// --------------------
	// co_return サポート
public:
	void return_void()
	{
		mark_succeeded();
	}

	// --------------------
	// co_yield サポート
public:
	auto yield_value(cancellation_t)
	{
		mark_canceled();
		return continuation_resumer{};
	}

	// --------------------
	// 結果の取得
public:
	// TODO: 戻り値型を検討（voidでもエラーやcancelは発生しうるので）
	result_type consume_result()
	{
		// TODO: 未完了、使用済み、例外時、キャンセル時の対応

		mark_consumed();

		// TODO:例外時は一応通し消費済みにする？
		// その後に再スロー
	}
};

// --------------------------------------------------------------

} // namespace base::detail
