#pragma once
#include "net_common.h"

namespace kim
{
    namespace net
    {
        template<typename T>
        // (Thread-safe)
        class tsqueue
        {
        public:
            // Default constructor
            tsqueue() = default;

            // Delete the copy constructor
            tsqueue(const tsqueue<T>&) = delete;

            // Destructor
            virtual ~tsqueue() { clear(); }

            // Returns and maintains item at front of queue
            const T& front()
            {
                std::scoped_lock lock(muxQueue);
                return deQueue.front();
            }

            // Returns and maintains item at back of queue
            const T& back()
            {
                std::scoped_lock lock(muxQueue);
                return deqQueue.back()
            }

            // Adds an item to back of queue
            void push_back(const T& item)
            {
                std::scoped_lock lock(muxQueue);
                deqQueue.emplace_back(std::move(item));
            }

            // Ands an item to front of queue
            void push_front(const T& item)
            {
                std::scoped_lock lock(muxQueue);
                deqQueue.emplace_front(std::move(item));
            }

            // Returns true of queue has no items
            bool empty()
            {
                std::scoped_lock lock(muxQueue);
                return deqQueue.empty();
            }

            // Returns number of items in queue
            size_t count()
            {
                std::scoped_lock lock(muxQueue);
                return deqQueue.size();
            }

            // Clears queue
            void clear()
            {
                std::scoped_lock lock(muxQueue);
                deqQueue.clear();
            }

            // Removes and returns item from front of queue
            T pop_front()
            {
                std::scoped_lock lock(muxQueue);
                auto t = std::move(deqQueue.front());
                deqQueue.pop_front();
                return t;
            }

            // Removes and returns item from back of queue
            T pop_back()
            {
                std::scoped_lock lock(muxQueue);
                auto t = std::move(deqQueue.back());
                deqQueue.pop_back();
                return t;
            }

        protected:
            // Mutex to protect the double ended queue
            std::mutex muxQueue;

            // Double ended queue
            std::deque<T> deqQueue;
        };
    }
}
