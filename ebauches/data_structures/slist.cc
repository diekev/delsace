#pragma once

#include <atomic>
#include <memory>
#include <utility>

template <typename T>
class slist {
	struct Node {
		T t;
		std::shared_ptr<Node> next;
	};

	std::atomic<Node *> head{nullptr};

	slist(slist&) = delete;
	void operator=(slist&) = delete;

public:
	slist() = default;
	~slist() = default;

	class reference {
		std::shared_ptr<Node> p;

	public:
		reference(std::shared_ptr<Node> p_)
			: p{p_}
		{}

		T &operator*() { return p->t; }
		T *operator->() { return &p->t; }
	};

	T *find(T t) const;
	void push_front(T t);
	void pop_front();
};

template <typename T>
T *slist<T>::find(T t) const
{
	auto p = head.load();

	while (p && p->t != t) {
		p = p->next;
	}

	return reference(std::move(p));
}

template <typename T>
void slist<T>::push_front(T t)
{
	auto p = std::make_shared<Node>();
	p->t = t;
	p->next = head;

	while (!head.compare_exchange_weak(p->next, p)) {}
}

template <typename T>
void slist<T>::pop_front()
{
	auto p = head.load();
	while (p && !head.compare_exchange_weak(p, p->next)) {}
}
