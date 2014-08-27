#ifndef __LOCK_FREE_QUEUE_HPP__
#define __LOCK_FREE_QUEUE_HPP__

#include <atomic>
#include <stddef.h>
#include <windows.h>

struct LFQueueNode;
struct pointer_t 
{
	LFQueueNode *ptr;
	unsigned int tag;
	pointer_t() 
	{
		ptr = NULL;
		tag = 0;
	}
	pointer_t(LFQueueNode *a_ptr, unsigned int a_tag)
	{
		ptr = a_ptr; tag=a_tag;
	}

	friend bool operator==(pointer_t const &l, pointer_t const &r)
	{
		return l.ptr == r.ptr && l.tag == r.tag;
	}

	friend bool operator!=(pointer_t const &l, pointer_t const &r)
	{
		return !(l == r);
	}
};




struct LFQueueNode 
{ 
	LFQueueNode* next; 
	void* value; 
	LFQueueNode()
	{
		value = NULL;
		next=  NULL;
	}
};



#ifndef _WIN32
#ifdef __x86_64__
inline bool CAS2(pointer_t *addr, pointer_t &old_value, pointer_t &new_value)
{
	bool  ret;
	__asm__ __volatile__(
		"lock cmpxchg16b %1;\n"
		"sete %0;\n"
		:"=m"(ret),"+m" (*(volatile pointer_t *) (addr))
		:"a" (old_value.ptr), "d" (old_value.tag), "b" (new_value.ptr), "c" (new_value.tag));
	return ret;
}

#else
inline bool CAS2(pointer_t *addr, pointer_t &old_value, pointer_t &new_value)
{
	bool  ret;
	__asm__ __volatile__(
		"lock cmpxchg8b %1;\n"
		"sete %0;\n"
		:"=m"(ret),"+m" (*(volatile pointer_t *) (addr))
		:"a" (old_value.ptr), "d" (old_value.tag), "b" (new_value.ptr), "c" (new_value.tag));
	return ret;
}
#endif

class JK_LockFreeQueue 
{
	pointer_t m_tailNode;
	pointer_t m_headNode;

public:
	JK_LockFreeQueue() 
	{

	}


	void init() 
	{
		LFQueueNode *nd = new LFQueueNode();
		nd->next = pointer_t(NULL, 0);
		m_headNode = pointer_t(nd, 0);
		m_tailNode = pointer_t(nd, 0);
	}


	void enqueue(data_type val) 
	{
		pointer_t tail, next;
		LFQueueNode* nd = new LFQueueNode();
		nd->value = val;
		while(true)
		{
			tail = this->m_tailNode;
			next = tail.ptr->next;
			if (tail == this->m_tailNode)
			{
				if(next.ptr == NULL)
				{
					pointer_t new_pt(nd, next.tag+1);
					if(CAS2(&(this->m_tailNode.ptr->next), next, new_pt))
					{
						break; // Enqueue done!
					}
				}
				else 
				{
					pointer_t new_pt(next.ptr, tail.tag+1);
					CAS2(&(this->m_tailNode), tail, new_pt);
				}
			}
		}
		pointer_t new_pt(nd, tail.tag+1);
		CAS2(&(this->m_tailNode), tail, new_pt);
	}


	data_type dequeue()
	{
		pointer_t tail, head, next;
		data_type val=NULL;
		while(true)
		{
			head = this->m_headNode;
			tail = this->m_tailNode;
			next = (head.ptr)->next;
			if (head != this->m_headNode) continue;

			if(head.ptr == tail.ptr)
			{
				if (next.ptr == NULL)
				{
					return NULL;
				}
				pointer_t new_pt(next.ptr, tail.tag+1);
				CAS2(&(this->m_tailNode), tail, new_pt);
			} 
			else
			{ 
				val = next.ptr->value;
				pointer_t new_pt(next.ptr, head.tag+1);
				if(CAS2(&(this->m_headNode), head, new_pt))
				{
					break;
				}
			}
		}
		delete head.ptr;
		return val;
	}
};

// lock free queue( windows ) 
#else

class JK_LockFreeQueue 
{
	LFQueueNode* m_headNode;
	LFQueueNode* m_tailNode;


public:
	JK_LockFreeQueue() 
	{
		Init();
	}


	void Init() 
	{
		m_headNode = new LFQueueNode();
		m_headNode->next = NULL;;
		m_tailNode = m_headNode;
	}


	void Enqueue( void* val ) 
	{
		LFQueueNode* targetNode = NULL; 
		LFQueueNode* pNode = new LFQueueNode();
		pNode->value = val;
		pNode->next = NULL;
		do 
		{ 
			targetNode = m_tailNode; 
		}
		//pNode->next = targetNode;
		//m_headNode->next = pNode;
		while( __InlineInterlockedCompareExchangePointer( (PVOID*)&targetNode->next, (PVOID)pNode, 0 ) != targetNode->next );
		__InlineInterlockedCompareExchangePointer((PVOID*)&m_tailNode, (PVOID)pNode, targetNode ); 
	}


	void* Dequeue()
	{
		LFQueueNode* tempNode = NULL; 
		do 
		{ 
			tempNode = m_headNode->next; 
			if ( tempNode == NULL) 
			{
				return NULL; 
			}
		}while (  __InlineInterlockedCompareExchangePointer((PVOID*)&(m_headNode->next), tempNode->next, tempNode ) != tempNode ); 

		//LFQueueNode* sxxxx  = (LFQueueNode*)__InlineInterlockedCompareExchangePointer((PVOID*)&(m_headNode->next), tempNode->next, tempNode ); 
		return tempNode->value; 
	}
};

#endif

#endif //__LOCK_FREE_QUEUE_HPP__
