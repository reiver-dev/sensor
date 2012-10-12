#ifndef MPSC_QUEUE_HPP_
#define MPSC_QUEUE_HPP_

#include <cstring>
#include <atomic>

template<typename DataT>
class MpscQueue {
private:

	struct Node {
		Node *next;
		DataT data;
	};

	std::atomic<Node *> head;
	char cache_padding[64 - sizeof(std::atomic<Node *>)];
	std::atomic<Node *> tail;

	template<typename T>
	void clearData(T data) {
		//
	}

	template<typename T>
	void clearData(T *data) {
		delete data;
	}

public:

	MpscQueue() {
		Node *n = new Node();
		head.store(n);
		tail.store(n);
	}

	~MpscQueue() {
		Node *next = head.load(std::memory_order_relaxed);
		Node *node = next;
		if (node) {
			next = node->next;
			delete node;
		}
		while((node = next)) {
			next = node->next;
			clearData(node->data);
			delete node;
		}
	}

	void push(DataT &data) {
		Node *next = new Node();
		next->data = data;
		next->next = NULL;

		Node *prev = tail.exchange(next, std::memory_order_acq_rel);
		prev->next = next;
	}

	bool pop(DataT &data) {
		Node *h = head.load(std::memory_order_relaxed);
		Node *next = h->next;
		if (next) {
			head.store(next, std::memory_order_release);
			data = next->data;
			delete h;
			return true;
		}
		return false;
	}

};

#endif /* MPSC_QUEUE_HPP_ */
