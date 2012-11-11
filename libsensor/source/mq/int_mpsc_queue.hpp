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
//		Node *next = head.load(std::memory_order_relaxed);
//		Node *node = next;
//		if (node) {
//			next = node->next;
//			delete node;
//		}
//		while((node = next)) {
//			next = node->next;
//			clearData(node->data);
//			delete node;
//		}
	}

	void push(Node *next) {
		next->next = NULL;
		Node *prev = tail.exchange(next, std::memory_order_acq_rel);
		prev->next = next;
	}

//	Node *pop() {
//		Node *h = head.load(std::memory_order_relaxed);
//		Node *next = h->next;
//		if (next) {
//			head.store(next, std::memory_order_release);
//		}
//		return next;
//	}

	Node *pop() {
		Node *s = &stub;
		Node *hd = head.load(std::memory_order_relaxed);
		Node *nx = hd->next;

		if (hd == s) {
			if (nx == nullptr)
				return nullptr;
			head.store(nx, std::memory_order_relaxed);
			hd = nx;
			nx = nx->next;
		}

		if (nx) {
			head.store(nx, std::memory_order_relaxed);
			return hd;
		}

		Node *tl = tail.load(std::memory_order_relaxed);
		if (tl != hd)
			return nullptr;

		push(s);

		nx = hd->next;
		if (nx) {
			head.store(nx, std::memory_order_relaxed);
			return hd;
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
