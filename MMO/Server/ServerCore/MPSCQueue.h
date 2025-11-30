#pragma once

template<typename T>
class MPSCQueue
{
public:
	MPSCQueue() : head(new Node()), tail(head.load()) {}
	~MPSCQueue() { Clear(); delete head.load(); }

	void Push(const T& item)
	{
		Node* node = new Node(item);
		Node* prev = head.exchange(node);
		prev->next.store(node);
	}

	bool Pop(T& out)
	{
		Node* next = tail.load()->next.load();
		if (!next) return false;
		out = next->data;
		Node* old = tail.load();
		tail.store(next);
		delete old;
		return true;
	}

	void Clear()
	{
		T dummy;
		while (Pop(dummy)) {}
	}

private:
	struct Node
	{
		T data;
		std::atomic<Node*> next{ nullptr };
		Node() = default;
		Node(const T& val) : data(val) {}
	};

	std::atomic<Node*> head; // producers push here
	std::atomic<Node*> tail; // consumer pops here
};