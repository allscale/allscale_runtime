
#ifndef ALLSCALE_TREETURE_BUFFER_HPP
#define ALLSCALE_TREETURE_BUFFER_HPP

#include <allscale/treeture.hpp>

#include <vector>

namespace allscale { namespace components {

    struct treeture_buffer
    {
        treeture_buffer()
          : current_(0)
        {}

        treeture_buffer(std::size_t max_size)
          : buffer_(max_size)
          , current_(0)
        {}

        template <typename Lock>
        void add(Lock l, treeture<void> t)
        {
            treeture<void>* buffered = nullptr;
            {
                buffered = &buffer_[current_];
                current_ = (current_ + 1) % buffer_.size();
            }

            l.unlock();

            if (buffered->valid())
            {
                buffered->wait();
            }
            *buffered = std::move(t);
        }

        std::vector<treeture<void>> buffer_;
        std::size_t current_;
    };

}}

#endif
