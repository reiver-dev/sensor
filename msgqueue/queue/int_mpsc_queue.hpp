#ifndef INT_MPSC_QUEUE_HPP_
#define INT_MPSC_QUEUE_HPP_

#include <cstring>
#include <atomic>

class IntMpscQueue {
public:

	struct Node {
		Node *next;
		void *self;
	};

	IntMpscQueue() {
		head.store(&stub);
		tail.store(&stub);
		stub.next = 0;
	}

	~IntMpscQueue() {

	}

	void push(Node *next) {
		next->next = NULL;
		Node *prev = tail.exchange(next, std::memory_order_acq_rel);
		prev->next = next;
	}

	Node *pop() {
		Node *s = &stub;
		Node *h = head.load(std::memory_order_relaxed);
		Node *n = h->next;

		if (h == s) {
			if (n == nullptr)
				return nullptr;
			head.store(n, std::memory_order_relaxed);
			h = n;
			n = n->next;
		}

		if (n) {
			head.store(n, std::memory_order_relaxed);
			return h;
		}

		Node *t = tail.load(std::memory_order_relaxed);
		if (t != h)
			return nullptr;

		push(s);

		n = h->next;
		if (n) {
			head.store(n, std::memory_order_relaxed);
			return h;
		}

		return nullptr;
	}

private:

	std::atomic<Node *> head;
	char cache_padding0[64 - sizeof(std::atomic<Node *>)];
	std::atomic<Node *> tail;
	char cache_padding1[64 - sizeof(std::atomic<Node *>)];
	Node stub;

	template<typename T>
	void clearData(T data) {
		//
	}

	template<typename T>
	void clearData(T *data) {
		delete data;
	}

};

#endif /* INT_MPSC_QUEUE_HPP_ */
