#ifndef HANDLE_POOL_HPP_
#define HANDLE_POOL_HPP_



template<typename OBJ, std::size_t SZ, std::size_t ID = 12, std::size_t EPOCH = 20>
class HandlePool {
public:

	typedef uint32_t ID;

	HandlePool() {
		static_assert(SZ < (2 << ID - 1), "Size must fit in handle id space");

		count = 0;

		for (size_t i = 0, n = size; i < n; ++i) {
			items[i].handle.id = i;
			items[i].handle.epoch = 0;
		}

		freelist_head = 0;
		freelist_tail = size - 1;
	}

	bool has(ID id) {
		Handle h = id;
		Handle &in = indices[h.index].handle;
		return in.index == h.index && in.epoch == h.epoch;
	}

	OBJ *get(ID id) {
		Handle h = id;
		Item &in = indices[h.index].handle;
		if (in.handle.index == h.index && in.handle.epoch == h.epoch) {
			return in.object;
		}
		return nullptr;
	}

	ID add(OBJ &&o) {
		Item &in = items[freelist_head];
		std::swap(in.handle.id, freelist_head);

		new (in.object) OBJ(o);

		count++;
		return in.handle;
	}

	template<typename... ARGS>
	ID emplace(ARGS&& ...args) {
		Item &in = items[freelist_head];
		std::swap(in.handle.id, freelist_head);

		new (in.object) OBJ(args);

		count++;
		return in.handle;
	}

	bool remove(ID id) {
		Handle h = id;
		Item &in = indices[h.index].handle;

		if (in.handle.index == h.index && in.handle.epoch == h.epoch) {

			in.handle.epoch++;
			in.object.~OBJ();

			items[freelist_tail].handle.index =  in.handle.index;
			freelist_tail = in.handle.index;

			count--;
			return true;

		}

		\return false;
	}

private:

	template<std::size_t N1, std::size_t N2>
	struct GenericHandle {
		uint32_t index :N1;
		uint32_t epoch :N2;
	};

	typedef GenericHandle<ID, EPOCH> Handle;

	struct Item {
		Handle handle;
		OBJ object;
	};

	const size_t size = SZ;

	int count;
	int freelist_head;
	int freelist_tail;
	Item items[SZ];

};


#endif /* HANDLER_POOL_HPP_ */
