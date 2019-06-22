//
// Created by andreybocharnikov on 22.06.2019.
//

#ifndef EXEM_LIST_LIST_H
#define EXEM_LIST_LIST_H

template<class T>
struct list {

    struct base_node;
    struct node;

    template<class U>
    struct my_iterator {

        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = U;
        using pointer = U *;
        using reference = U &;
        friend struct list;

        template<class V> // от не const к const
        my_iterator(my_iterator<V> const &other,
                    typename std::enable_if<std::is_same<U, V const>::value && std::is_const<U>::value>::type * = nullptr)
                    : ptr(other.ptr) {}

        my_iterator operator++() {
            ptr = ptr->next;
            return my_iterator(ptr);
        }

        my_iterator operator++(int) {
            base_node *copy = ptr;
            ptr = ptr->next;
            return copy;
        }

        my_iterator operator--() {
            ptr = ptr->prev;
            return ptr;
        }

        my_iterator operator--(int) {
            base_node *copy = ptr;
            ptr = ptr->prev;
            return copy;
        }

        reference operator*() const {
            return static_cast<node *>(ptr)->val;
        }

        pointer operator->() const {
            return &(static_cast<node*>(ptr)->val);
        }

        friend bool operator==(my_iterator lhs, my_iterator rhs) {
            return lhs.ptr == rhs.ptr;
        }

        friend bool operator!=(my_iterator lhs, my_iterator rhs) {
            return lhs.ptr != rhs.ptr;
        }

    private:
        base_node *ptr;
        my_iterator(base_node *other) : ptr(other) {}
    };

    typedef my_iterator<T> iterator;
    typedef my_iterator<T const> const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    iterator begin() {
        return begin_;
    }

    iterator end() {
        return const_cast<base_node*>(&end_);
    }

    const_iterator begin() const {
        return begin_;
    }

    const_iterator end() const {
        return const_cast<base_node*>(&end_);
    }

    reverse_iterator rbegin() {
        return reverse_iterator(end());
    }

    reverse_iterator rend() {
        return reverse_iterator(begin());
    }

    const_reverse_iterator rbegin() const {
        return const_reverse_iterator(end());
    }

    const_reverse_iterator rend() const {
        return const_reverse_iterator(begin());
    }

    iterator prev(iterator const& other) const {
        return other.ptr->prev;
    }

    iterator next(iterator const& other) const {
        return other.ptr->next;
    }

    list() = default;

    list(list const &other) {
        for (auto it = other.begin_; it != &other.end_; it = it->next) {
            try {
                push_back(static_cast<node*>(it)->val);
            } catch(...) {
                if (it != other.begin_) {
                    for (auto it2 = begin_->next; it2 != &end_; it2 = it2->next) {
                        delete static_cast<node *>(it2->prev);
                    }
                    delete static_cast<node *>(end_.prev);
                }
                begin_ = &end_;
                end_.prev = nullptr;
                end_.next = nullptr;
                throw;
            }
        }
    }

    list& operator=(list const& other) {
        if (&other != this) {
            list(other).swap(*this);
        }
        return *this;
    }

    ~list() {
        clear();
    }

    void push_back(T const &val) {
        insert(end(), val);
    }

    void pop_back() {
        erase(prev(end()));
    }

    void push_front(T const &val) {
        insert(begin(), val);
    }

    void pop_front() {
        erase(begin());
    }

    T& front() {
        return static_cast<node*>(begin_)->val;
    }

    T const& front() const {
        return static_cast<node*>(begin_)->val;
    }

    T& back() {
        return static_cast<node*>(end_.prev)->val;
    }

    T const& back() const {
        return static_cast<node*>(end_.prev)->val;
    }

    iterator insert(const_iterator p, T const &val) { // before
        base_node *cur_prev = p.ptr->prev, *cur_next = p.ptr->next, *cur = p.ptr;
        base_node *new_node = new node(cur_prev, p.ptr, val);
        // prev  p next
        if (cur_prev != nullptr) {
            cur_prev->next = new_node;
            cur->prev = new_node;
        } else {
            begin_->prev = new_node;
            begin_ = new_node;
        }
        return new_node;
    }

    iterator erase(const_iterator p) {
        if (p == begin_) {
            begin_ = begin_->next;
            delete static_cast<node*>(begin_->prev);
            begin_->prev = nullptr;
            return begin_;
        }
        base_node *cur_prev = p.ptr->prev, *cur_next = p.ptr->next, *cur = p.ptr;
        delete static_cast<node*>(cur);
        cur_prev->next = cur_next;
        cur_next->prev = cur_prev;
        return cur_next;
    }

    bool empty() const {
        return begin_ == &end_;
    }

    void clear() {
        if (begin_ == &end_)
            return;
        for (auto it = begin_->next; it != end(); it = it->next)
            delete static_cast<node*>(it->prev);
        delete static_cast<node*>(end_.prev);
        begin_ = &end_;
        end_.prev = nullptr;
        end_.next = nullptr;
    }

    friend void swap(list &lhs, list &rhs) {
        lhs.swap(rhs);
    }

    void swap(list &rhs) {
        if (empty() && rhs.empty()) {
            return;
        }

        if (empty() && !rhs.empty()) {
            begin_ = rhs.begin_;
            rhs.end_.prev->next = &end_;
            end_.prev = rhs.end_.prev;
            rhs.begin_ = &rhs.end_;
            return;
        }

        if (!empty() && rhs.empty()) {
            rhs.begin_ = begin_;
            end_.prev->next = &rhs.end_;
            rhs.end_.prev = end_.prev;
            begin_ = &end_;
            return;
        }

        std::swap(begin_, rhs.begin_);

        base_node* save = rhs.end_.prev;

        end_.prev->next = &rhs.end_;
        rhs.end_.prev = end_.prev;

        save->next = &end_;
        end_.prev = save;
    }

    // before pos in other [first..last)
    // pos_prev  pos
    // from other to this
    void splice(const_iterator pos, list& other, const_iterator first, const_iterator last) {
        base_node* save_first_prev = first.ptr->prev, *pos_prev = pos.ptr->prev;

        if (first == last)
            return;

        if (pos_prev == nullptr && save_first_prev == nullptr) {
            base_node* save_begin = begin_;
            begin_ = first.ptr;
            begin_->prev = nullptr;
            last.ptr->prev->next = save_begin;
            save_begin->prev = last.ptr->prev;

            other.begin_ = last.ptr;
            other.begin_->prev = nullptr;
            return;
        }

        //pos - begin_ first != begin_
        if (pos_prev == nullptr && save_first_prev != nullptr) {
            base_node* save_begin = begin_;
            begin_ = first.ptr;
            begin_->prev = nullptr;
            last.ptr->prev->next = save_begin;
            save_begin->prev = last.ptr->prev;

            save_first_prev->next = last.ptr;
            last.ptr->prev = save_first_prev;
            return;
        }

        if (pos_prev != nullptr && save_first_prev == nullptr) {
            pos_prev->next = first.ptr;
            first.ptr->prev = pos_prev;
            last.ptr->prev->next = pos.ptr;
            pos.ptr->prev = last.ptr->prev;

            other.begin_ = last.ptr;
            other.begin_->prev = nullptr;
            return;
        }

        pos_prev->next = first.ptr;
        first.ptr->prev = pos_prev;

        pos.ptr->prev = last.ptr->prev;
        last.ptr->prev->next = pos.ptr;

        save_first_prev->next = last.ptr;
        last.ptr->prev = save_first_prev;
    }

    struct base_node {
        base_node() = default;

        base_node(base_node *prev, base_node *next) : next(next), prev(prev) {}

        base_node *next = nullptr, *prev = nullptr;
    };

    struct node : base_node {

        node(base_node *prev, base_node *next, T const &v) : base_node(prev, next), val(v) {}

        T val;
    };

private:
    base_node end_;
    base_node *begin_ = &end_;
};

#endif //EXEM_LIST_LIST_H
