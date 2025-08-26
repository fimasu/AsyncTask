#pragma once
#include "base/base_IAwaiter.h"

// --------------------------------------------------------------

namespace base::detail
{

// --------------------------------------------------------------

enum class PromiseStatus
{
    Executing, // 実行中
    Succeeded, // 実行完了
    Faulted,   // 例外発生
    Consumed,  // 結果を取得済み
};

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
    // 例外のハンドリング
public:
    void unhandled_exception()
    {
        m_Status = PromiseStatus::Faulted;
        // TODO: 例外が使える環境では必要に応じてハンドリング
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

    // --------------------
    // Statusの制御
public:
    PromiseStatus get_status() const noexcept { return m_Status; }

    void mark_succeeded() noexcept
    {
        m_Status = PromiseStatus::Succeeded;
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
    IAwaiter*       m_SubAwaiter    = nullptr;
    PromiseStatus   m_Status        = PromiseStatus::Executing;
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

template <class ReturnObjectT>
class promise final : public promise_base
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
    void return_value(const result_type& value)
    {
        m_ResultHolder = std::make_unique<result_type>(value);
        mark_succeeded();
    }

    void return_value(result_type&& value)
    {
        m_ResultHolder = std::make_unique<result_type>(std::move(value));
        mark_succeeded();
    }

    template <class... Args>
    void return_value(Args&&... args)
    {
        m_ResultHolder = std::make_unique<result_type>(std::forward<Args>(args)...);
        mark_succeeded();
    }

    // --------------------
    // 結果の取得
public:
    result_type consume_result()
    {
        // TODO: 未完了、使用済みをはじく

        mark_consumed();

        // TODO:例外時は一応通し消費済みにする？
        // その後に再スロー
        return std::move(*m_ResultHolder);
    }

private:
    std::unique_ptr<result_type> m_ResultHolder = {};    // TODO : アロケート抑制
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
    // 結果の取得
public:
    result_type consume_result()
    {
        // TODO: 未完了、使用済みをはじく

        mark_consumed();

        // TODO:例外時は一応通し消費済みにする？
        // その後に再スロー
    }
};

// --------------------------------------------------------------

} // namespace base::detail
