#pragma once
#include <core/memory/allocator_interface.h>
#include <core/memory/allocator_adapters.h>
#include <core/defines.h>
#include <initializer_list>
#include <iterator>

namespace ds::core::containers
{
	/**
	 * DVector - Dynamic array container implementation
	 * 
	 * A container that stores elements in a contiguous memory block and
	 * supports dynamic resizing. Integrated with Destan's custom memory allocators.
	 * 
	 * @tparam T Type of the elements
	 * @tparam Allocator Allocator type used for memory management
	 */
	template <typename T, typename Allocator = memory::Default_Allocator<T>>
	class DVector
	{
	public:
		// Type definitions
		using value_type      = T;
		using allocator_type  = Allocator;
		using size_type       = ds_u64;
		using difference_type = ds_i64;
		using reference       = value_type&;
		using const_reference = const value_type&;
		using pointer         = T*;
		using const_pointer   = const T*;

		/**
		 * Iterator for DVector - Provides random access to elements
		 */
		class iterator
		{
		public:
			using iterator_category = std::random_access_iterator_tag;
			using value_type      = T;
			using difference_type = ds_i64;
			using pointer         = T*;
			using reference       = T&;

			iterator() noexcept : m_ptr(nullptr) {}
			iterator(pointer ptr) noexcept : m_ptr(ptr) {}

			reference operator*() const { return *m_ptr; }
			pointer operator->() const { return m_ptr; }

			iterator& operator++() { ++m_ptr; return *this; }
			iterator operator++(int) { iterator tmp = *this; ++m_ptr; return tmp; }
			iterator& operator--() { --m_ptr; return *this; }
			iterator operator--(int) { iterator tmp = *this; --m_ptr; return tmp; }

			iterator operator+(difference_type n) const { return iterator(m_ptr + n); }
			iterator operator-(difference_type n) const { return iterator(m_ptr - n); }
			iterator& operator+=(difference_type n) { m_ptr += n; return *this; }
			iterator& operator-=(difference_type n) { m_ptr -= n; return *this; }

			difference_type operator-(const iterator& other) const { return m_ptr - other.m_ptr; }

			reference operator[](difference_type n) const { return m_ptr[n]; }

			bool operator==(const iterator& other) const { return m_ptr == other.m_ptr; }
			bool operator!=(const iterator& other) const { return m_ptr != other.m_ptr; }
			bool operator<(const iterator& other) const { return m_ptr < other.m_ptr; }
			bool operator>(const iterator& other) const { return m_ptr > other.m_ptr; }
			bool operator<=(const iterator& other) const { return m_ptr <= other.m_ptr; }
			bool operator>=(const iterator& other) const { return m_ptr >= other.m_ptr; }

		private:
			pinter m_ptr;

			friend class DVector;
		};

		/**
		 * Const Iterator for DVector - Provides read-only random access to elements
		 */
		class const_iterator
		{
		public:
			using iterator_category = std::random_access_iterator_tag;
			using value_type = T;
			using difference_type = ds_i64;
			using pointer = const T*;
			using reference = const T&;

			const_iterator() noexcept : m_ptr(nullptr) {}
			const_iterator(pointer ptr) noexcept : m_ptr(ptr) {}
			const_iterator(const iterator& other) noexcept : m_ptr(other.m_ptr) {}

			reference operator*() const { return *m_ptr; }
			pointer operator->() const { return m_ptr; }

			const_iterator& operator++() { ++m_ptr; return *this; }
			const_iterator operator++(int) { const_iterator tmp = *this; ++m_ptr; return tmp; }
			const_iterator& operator--() { --m_ptr; return *this; }
			const_iterator operator--(int) { const_iterator tmp = *this; --m_ptr; return tmp; }

			const_iterator operator+(difference_type n) const { return const_iterator(m_ptr + n); }
			const_iterator operator-(difference_type n) const { return const_iterator(m_ptr - n); }
			const_iterator& operator+=(difference_type n) { m_ptr += n; return *this; }
			const_iterator& operator-=(difference_type n) { m_ptr -= n; return *this; }

			difference_type operator-(const const_iterator& other) const { return m_ptr - other.m_ptr; }

			reference operator[](difference_type n) const { return m_ptr[n]; }

			bool operator==(const const_iterator& other) const { return m_ptr == other.m_ptr; }
			bool operator!=(const const_iterator& other) const { return m_ptr != other.m_ptr; }
			bool operator<(const const_iterator& other) const { return m_ptr < other.m_ptr; }
			bool operator>(const const_iterator& other) const { return m_ptr > other.m_ptr; }
			bool operator<=(const const_iterator& other) const { return m_ptr <= other.m_ptr; }
			bool operator>=(const const_iterator& other) const { return m_ptr >= other.m_ptr; }

		private:
			pointer m_ptr;

			friend class DVector;
		};

		// Reverse iterator types
		using reverse_iterator       = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		//====================
		// Constructors/Destructor
		//====================

		/**
		 * Default constructor - creates an empty vector
		 */
		DVector();

		/**
		 * Constructor with allocator
		 * @param alloc The allocator to use
		 */
		explicit DVector(const allocator_type& alloc);

		/**
		 * Fill constructor - creates a vector with count copies of value
		 * @param count Number of elements
		 * @param value Value to initialize elements with
		 * @param alloc The allocator to use
		 */
		explicit DVector(size_type count, const T& value = T(), const allocator_type& alloc = allocator_type());

		/**
		 * Copy constructor
		 * @param other DVector to copy from
		 */
		DVector(const DVector& other);

		/**
		 * Move constructor
		 * @param other DVector to move from
		 */
		DVector(DVector&& other) noexcept;

		/**
		 * Initializer list constructor
		 * @param init Initializer list to copy elements from
		 * @param alloc The allocator to use
		 */
		DVector(std::initializer_list<T> init, const allocator_type& alloc = allocator_type());

		/**
		 * Destructor
		 */
		~DVector();

		//====================
		// Assignment operators
		//====================

		/**
		 * Copy assignment operator
		 * @param other DVector to copy from
		 * @return Reference to this vector
		 */
		DVector& operator=(const DVector& other);

		/**
		 * Move assignment operator
		 * @param other DVector to move from
		 * @return Reference to this vector
		 */
		DVector& operator=(DVector&& other) noexcept;

		/**
		 * Initializer list assignment operator
		 * @param ilist Initializer list to copy elements from
		 * @return Reference to this vector
		 */
		DVector& operator=(std::initializer_list<T> ilist);

		//====================
		// Element access
		//====================

		/**
		 * Access element with bounds checking
		 * @param pos Position of the element
		 * @return Reference to the requested element
		 * @throws std::out_of_range if pos >= size()
		 */
		reference at(size_type pos);

		/**
		 * Access element with bounds checking (const version)
		 * @param pos Position of the element
		 * @return Const reference to the requested element
		 * @throws std::out_of_range if pos >= size()
		 */
		const_reference at(size_type pos) const;

		/**
		 * Access element without bounds checking
		 * @param pos Position of the element
		 * @return Reference to the requested element
		 */
		reference operator[](size_type pos);

		/**
		 * Access element without bounds checking (const version)
		 * @param pos Position of the element
		 * @return Const reference to the requested element
		 */
		const_reference operator[](size_type pos) const;

		/**
		 * Access the first element
		 * @return Reference to the first element
		 */
		reference front();

		/**
		 * Access the first element (const version)
		 * @return Const reference to the first element
		 */
		const_reference front() const;

		/**
		 * Access the last element
		 * @return Reference to the last element
		 */
		reference back();

		/**
		 * Access the last element (const version)
		 * @return Const reference to the last element
		 */
		const_reference back() const;

		/**
		 * Get pointer to the underlying array
		 * @return Pointer to the array
		 */
		T* data() noexcept;

		/**
		 * Get const pointer to the underlying array
		 * @return Const pointer to the array
		 */
		const T* data() const noexcept;


		//====================
		// Iterators
		//====================

		iterator begin() noexcept;
		const_iterator begin() const noexcept;
		const_iterator cbegin() const noexcept;

		iterator end() noexcept;
		const_iterator end() const noexcept;
		const_iterator cend() const noexcept;

		reverse_iterator rbegin() noexcept;
		const_reverse_iterator rbegin() const noexcept;
		const_reverse_iterator crbegin() const noexcept;

		reverse_iterator rend() noexcept;
		const_reverse_iterator rend() const noexcept;
		const_reverse_iterator crend() const noexcept;

		//====================
		// Capacity
		//====================

		/**
		 * Check if the container is empty
		 * @return true if the container is empty, false otherwise
		 */
		[[nodiscard]] bool empty() const noexcept;

		/**
		 * Get the number of elements
		 * @return The number of elements in the container
		 */
		size_type size() const noexcept;

		/**
		 * Get the maximum possible number of elements
		 * @return Maximum number of elements the container can hold
		 */
		size_type max_size() const noexcept;

		/**
		 * Reserve storage for future growth
		 * @param new_cap New capacity of the container
		 */
		void reserve(size_type new_cap);

		/**
		 * Get the number of elements that can be held in currently allocated storage
		 * @return Capacity of the container
		 */
		size_type capacity() const noexcept;

		/**
		 * Reduce memory usage by freeing unused capacity
		 */
		void shrink_to_fit();

		//====================
		// Modifiers
		//====================

		void clear() noexcept;
		iterator insert(const_iterator pos, const T& value);
		iterator insert(const_iterator pos, T&& value);
		iterator insert(const_iterator pos, size_type count, const T& value);

		template <typename... Args>
		iterator emplace(const_iterator pos, Args&&... args);

		iterator erase(const_iterator pos);
		iterator erase(const_iterator first, const_iterator last);
		void push_back(const T& value);
		void push_back(T&& value);

		template <typename... Args>
		reference emplace_back(Args&&... args);

		void pop_back();
		void resize(size_type count);
		void resize(size_type count, const value_type& value);
		void swap(DVector& other) noexcept;

	private:
		/**
		 * Reallocate the vector storage to a new capacity
		 * @param new_cap New capacity
		 */
		void reallocate(size_type new_cap);

		// Member variables
		pointer m_data = nullptr;       ///< Pointer to the storage
		size_type m_size = 0;           ///< Number of elements
		size_type m_capacity = 0;       ///< Storage capacity
		allocator_type m_allocator;     ///< Allocator instance
	};

	//====================
	// Non-member functions
	//====================

	template <typename T, typename Allocator>
	bool operator==(const DVector<T, Allocator>& lhs, const DVector<T, Allocator>& rhs);

	template <typename T, typename Allocator>
	bool operator!=(const DVector<T, Allocator>& lhs, const DVector<T, Allocator>& rhs);

	template <typename T, typename Allocator>
	bool operator<(const DVector<T, Allocator>& lhs, const DVector<T, Allocator>& rhs);

	template <typename T, typename Allocator>
	bool operator>(const DVector<T, Allocator>& lhs, const DVector<T, Allocator>& rhs);

	template <typename T, typename Allocator>
	bool operator<=(const DVector<T, Allocator>& lhs, const DVector<T, Allocator>& rhs);

	template <typename T, typename Allocator>
	bool operator>=(const DVector<T, Allocator>& lhs, const DVector<T, Allocator>& rhs);

	template <typename T, typename Allocator>
	void swap(DVector<T, Allocator>& lhs, DVector<T, Allocator>& rhs) noexcept;


    // *********************************************************************** //
    // ************************** IMPLEMENTATION ***************************** //
    // *********************************************************************** //


    /////////////////////////////////////////////////////////
    // Constructors and Destructor
    /////////////////////////////////////////////////////////
    template <typename T, typename Allocator>
    DVector<T, Allocator>::DVector()
        : m_data(nullptr), m_size(0), m_capacity(0), m_allocator()
    {
        // Default constructor creates an empty vector with no allocation
    }

    template <typename T, typename Allocator>
    DVector<T, Allocator>::DVector(const allocator_type& alloc)
        : m_data(nullptr), m_size(0), m_capacity(0), m_allocator(alloc)
    {
        // Constructor with custom allocator
    }

    template <typename T, typename Allocator>
    DVector<T, Allocator>::DVector(size_type count, const T& value, const allocator_type& alloc)
        : m_data(nullptr), m_size(0), m_capacity(0), m_allocator(alloc)
    {
        // Fill constructor
        resize(count, value);
    }

    template <typename T, typename Allocator>
    DVector<T, Allocator>::DVector(const DVector& other)
        : m_data(nullptr), m_size(0), m_capacity(0), m_allocator(other.m_allocator)
    {
        // Copy constructor
        reserve(other.m_size);

        // Copy construct elements from other vector
        for (size_type i = 0; i < other.m_size; ++i) {
            m_allocator.construct(m_data + i, other.m_data[i]);
        }

        m_size = other.m_size;
    }

    template <typename T, typename Allocator>
    DVector<T, Allocator>::DVector(DVector&& other) noexcept
        : m_data(other.m_data), m_size(other.m_size), m_capacity(other.m_capacity), m_allocator(std::move(other.m_allocator))
    {
        // Move constructor - take ownership of other's resources
        // Clear the moved-from vector
        other.m_data = nullptr;
        other.m_size = 0;
        other.m_capacity = 0;
    }

    template <typename T, typename Allocator>
    DVector<T, Allocator>::DVector(std::initializer_list<T> init, const allocator_type& alloc)
        : m_data(nullptr), m_size(0), m_capacity(0), m_allocator(alloc)
    {
        // Initializer list constructor
        reserve(init.size());

        // Copy elements from initializer list
        for (const auto& item : init) {
            push_back(item);
        }
    }

    template <typename T, typename Allocator>
    DVector<T, Allocator>::~DVector()
    {
        // Destructor - clean up all resources
        clear(); // Destroy all elements

        // Deallocate storage if it exists
        if (m_data) {
            m_allocator.deallocate(m_data, m_capacity);
            m_data = nullptr;
            m_capacity = 0;
        }
    }

    /////////////////////////////////////////////////////////
    // Assignment Operators
    /////////////////////////////////////////////////////////

    template <typename T, typename Allocator>
    DVector<T, Allocator>& DVector<T, Allocator>::operator=(const DVector& other)
    {
        // Copy assignment operator
        if (this != &other) {
            // Clear existing elements
            clear();

            // Ensure we have enough capacity
            if (m_capacity < other.m_size) {
                if (m_data) {
                    m_allocator.deallocate(m_data, m_capacity);
                }

                m_capacity = other.m_size;
                m_data = m_allocator.allocate(m_capacity);
            }

            // Copy construct elements
            for (size_type i = 0; i < other.m_size; ++i) {
                m_allocator.construct(m_data + i, other.m_data[i]);
            }

            m_size = other.m_size;
        }

        return *this;
    }

    template <typename T, typename Allocator>
    DVector<T, Allocator>& DVector<T, Allocator>::operator=(DVector&& other) noexcept
    {
        // Move assignment operator
        if (this != &other) {
            // Clear and deallocate existing elements
            clear();
            if (m_data) {
                m_allocator.deallocate(m_data, m_capacity);
            }

            // Take ownership of other's resources
            m_data = other.m_data;
            m_size = other.m_size;
            m_capacity = other.m_capacity;
            m_allocator = std::move(other.m_allocator);

            // Reset other
            other.m_data = nullptr;
            other.m_size = 0;
            other.m_capacity = 0;
        }

        return *this;
    }

    template <typename T, typename Allocator>
    DVector<T, Allocator>& DVector<T, Allocator>::operator=(std::initializer_list<T> ilist)
    {
        // Initializer list assignment operator
        clear();

        // Ensure capacity
        if (m_capacity < ilist.size()) {
            if (m_data) {
                m_allocator.deallocate(m_data, m_capacity);
            }

            m_capacity = ilist.size();
            m_data = m_allocator.allocate(m_capacity);
        }

        // Copy elements
        size_type i = 0;
        for (const auto& item : ilist) {
            m_allocator.construct(m_data + i, item);
            ++i;
        }

        m_size = ilist.size();
        return *this;
    }

    /////////////////////////////////////////////////////////
    // Element Access Methods
    /////////////////////////////////////////////////////////

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::reference DVector<T, Allocator>::at(size_type pos)
    {
        // Access with bounds checking
        if (pos >= m_size) {
            throw std::out_of_range("DVector::at - Index out of range");
        }
        return m_data[pos];
    }

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::const_reference DVector<T, Allocator>::at(size_type pos) const
    {
        // Const access with bounds checking
        if (pos >= m_size) {
            throw std::out_of_range("DVector::at - Index out of range");
        }
        return m_data[pos];
    }

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::reference DVector<T, Allocator>::operator[](size_type pos)
    {
        // Access without bounds checking (but with assert in debug)
        DS_ASSERT(pos < m_size, "DVector::operator[] - Index out of bounds");
        return m_data[pos];
    }

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::const_reference DVector<T, Allocator>::operator[](size_type pos) const
    {
        // Const access without bounds checking (but with assert in debug)
        DS_ASSERT(pos < m_size, "DVector::operator[] - Index out of bounds");
        return m_data[pos];
    }

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::reference DVector<T, Allocator>::front()
    {
        // Access first element
        DS_ASSERT(m_size > 0, "DVector::front - Vector is empty");
        return m_data[0];
    }

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::const_reference DVector<T, Allocator>::front() const
    {
        // Const access first element
        DS_ASSERT(m_size > 0, "DVector::front - Vector is empty");
        return m_data[0];
    }

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::reference DVector<T, Allocator>::back()
    {
        // Access last element
        DS_ASSERT(m_size > 0, "DVector::back - Vector is empty");
        return m_data[m_size - 1];
    }

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::const_reference DVector<T, Allocator>::back() const
    {
        // Const access last element
        DS_ASSERT(m_size > 0, "DVector::back - Vector is empty");
        return m_data[m_size - 1];
    }

    template <typename T, typename Allocator>
    T* DVector<T, Allocator>::data() noexcept
    {
        // Get raw pointer to data
        return m_data;
    }

    template <typename T, typename Allocator>
    const T* DVector<T, Allocator>::data() const noexcept
    {
        // Get const raw pointer to data
        return m_data;
    }

    /////////////////////////////////////////////////////////
    // Iterator Methods
    /////////////////////////////////////////////////////////

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::iterator DVector<T, Allocator>::begin() noexcept
    {
        return iterator(m_data);
    }

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::const_iterator DVector<T, Allocator>::begin() const noexcept
    {
        return const_iterator(m_data);
    }

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::const_iterator DVector<T, Allocator>::cbegin() const noexcept
    {
        return const_iterator(m_data);
    }

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::iterator DVector<T, Allocator>::end() noexcept
    {
        return iterator(m_data + m_size);
    }

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::const_iterator DVector<T, Allocator>::end() const noexcept
    {
        return const_iterator(m_data + m_size);
    }

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::const_iterator DVector<T, Allocator>::cend() const noexcept
    {
        return const_iterator(m_data + m_size);
    }

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::reverse_iterator DVector<T, Allocator>::rbegin() noexcept
    {
        return reverse_iterator(end());
    }

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::const_reverse_iterator DVector<T, Allocator>::rbegin() const noexcept
    {
        return const_reverse_iterator(end());
    }

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::const_reverse_iterator DVector<T, Allocator>::crbegin() const noexcept
    {
        return const_reverse_iterator(end());
    }

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::reverse_iterator DVector<T, Allocator>::rend() noexcept
    {
        return reverse_iterator(begin());
    }

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::const_reverse_iterator DVector<T, Allocator>::rend() const noexcept
    {
        return const_reverse_iterator(begin());
    }

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::const_reverse_iterator DVector<T, Allocator>::crend() const noexcept
    {
        return const_reverse_iterator(begin());
    }

    /////////////////////////////////////////////////////////
    // Capacity Methods
    /////////////////////////////////////////////////////////

    template <typename T, typename Allocator>
    bool DVector<T, Allocator>::empty() const noexcept
    {
        return m_size == 0;
    }

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::size_type DVector<T, Allocator>::size() const noexcept
    {
        return m_size;
    }

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::size_type DVector<T, Allocator>::max_size() const noexcept
    {
        // Return a reasonable maximum size constrained by addressable memory
        return std::numeric_limits<size_type>::max() / sizeof(T);
    }

    template <typename T, typename Allocator>
    void DVector<T, Allocator>::reserve(size_type new_cap)
    {
        // Increase capacity if requested capacity is greater than current
        if (new_cap > m_capacity) {
            reallocate(new_cap);
        }
    }

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::size_type DVector<T, Allocator>::capacity() const noexcept
    {
        return m_capacity;
    }

    template <typename T, typename Allocator>
    void DVector<T, Allocator>::shrink_to_fit()
    {
        // Reduce capacity to match size
        if (m_size < m_capacity) {
            if (m_size > 0) {
                reallocate(m_size);
            }
            else {
                // If empty, free all memory
                if (m_data) {
                    m_allocator.deallocate(m_data, m_capacity);
                    m_data = nullptr;
                    m_capacity = 0;
                }
            }
        }
    }

    /////////////////////////////////////////////////////////
    // Modifier Methods
    /////////////////////////////////////////////////////////

    template <typename T, typename Allocator>
    void DVector<T, Allocator>::clear() noexcept
    {
        // Destroy all elements but keep capacity
        for (size_type i = 0; i < m_size; ++i) {
            m_allocator.destroy(m_data + i);
        }
        m_size = 0;
    }

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::iterator DVector<T, Allocator>::insert(const_iterator pos, const T& value)
    {
        // Calculate position index
        size_type index = pos - cbegin();
        DS_ASSERT(index <= m_size, "DVector::insert - Invalid position");

        // Check if we need to reallocate
        if (m_size == m_capacity) {
            size_type new_capacity = m_capacity == 0 ? 1 : m_capacity * 2;
            reallocate(new_capacity);
        }

        // Shift elements to make space
        for (size_type i = m_size; i > index; --i) {
            m_allocator.construct(m_data + i, std::move(m_data[i - 1]));
            m_allocator.destroy(m_data + i - 1);
        }

        // Insert the new element
        m_allocator.construct(m_data + index, value);
        ++m_size;

        return iterator(m_data + index);
    }

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::iterator DVector<T, Allocator>::insert(const_iterator pos, T&& value)
    {
        // Calculate position index
        size_type index = pos - cbegin();
        DS_ASSERT(index <= m_size, "DVector::insert - Invalid position");

        // Check if we need to reallocate
        if (m_size == m_capacity) {
            size_type new_capacity = m_capacity == 0 ? 1 : m_capacity * 2;
            reallocate(new_capacity);
        }

        // Shift elements to make space
        for (size_type i = m_size; i > index; --i) {
            m_allocator.construct(m_data + i, std::move(m_data[i - 1]));
            m_allocator.destroy(m_data + i - 1);
        }

        // Insert the new element
        m_allocator.construct(m_data + index, std::move(value));
        ++m_size;

        return iterator(m_data + index);
    }

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::iterator DVector<T, Allocator>::insert(const_iterator pos, size_type count, const T& value)
    {
        // Calculate position index
        size_type index = pos - cbegin();
        DS_ASSERT(index <= m_size, "DVector::insert - Invalid position");

        if (count == 0) {
            return iterator(m_data + index);
        }

        // Check if we need to reallocate
        if (m_size + count > m_capacity) {
            size_type new_capacity = std::max(m_capacity * 2, m_size + count);
            reallocate(new_capacity);
        }

        // Shift elements to make space
        for (size_type i = m_size + count - 1; i >= index + count; --i) {
            m_allocator.construct(m_data + i, std::move(m_data[i - count]));
            m_allocator.destroy(m_data + i - count);

            // Handle potential underflow for unsigned type
            if (i == index + count) break;
        }

        // Insert the new elements
        for (size_type i = 0; i < count; ++i) {
            m_allocator.construct(m_data + index + i, value);
        }

        m_size += count;
        return iterator(m_data + index);
    }

    template <typename T, typename Allocator>
    template <typename... Args>
    typename DVector<T, Allocator>::iterator DVector<T, Allocator>::emplace(const_iterator pos, Args&&... args)
    {
        // Calculate position index
        size_type index = pos - cbegin();
        DS_ASSERT(index <= m_size, "DVector::emplace - Invalid position");

        // Check if we need to reallocate
        if (m_size == m_capacity) {
            size_type new_capacity = m_capacity == 0 ? 1 : m_capacity * 2;
            reallocate(new_capacity);
        }

        // Shift elements to make space
        for (size_type i = m_size; i > index; --i) {
            m_allocator.construct(m_data + i, std::move(m_data[i - 1]));
            m_allocator.destroy(m_data + i - 1);
        }

        // Construct the new element in-place
        m_allocator.construct(m_data + index, std::forward<Args>(args)...);
        ++m_size;

        return iterator(m_data + index);
    }

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::iterator DVector<T, Allocator>::erase(const_iterator pos)
    {
        // Calculate position index
        size_type index = pos - cbegin();
        DS_ASSERT(index < m_size, "DVector::erase - Invalid position");

        // Destroy the element at position
        m_allocator.destroy(m_data + index);

        // Shift remaining elements
        for (size_type i = index; i < m_size - 1; ++i) {
            m_allocator.construct(m_data + i, std::move(m_data[i + 1]));
            m_allocator.destroy(m_data + i + 1);
        }

        --m_size;
        return iterator(m_data + index);
    }

    template <typename T, typename Allocator>
    typename DVector<T, Allocator>::iterator DVector<T, Allocator>::erase(const_iterator first, const_iterator last)
    {
        // Calculate position indices
        size_type start_index = first - cbegin();
        size_type end_index = last - cbegin();

        DS_ASSERT(start_index <= end_index && end_index <= m_size, "DVector::erase - Invalid range");

        if (start_index == end_index) {
            return iterator(m_data + start_index);
        }

        // Number of elements to erase
        size_type count = end_index - start_index;

        // Destroy elements in the range
        for (size_type i = start_index; i < end_index; ++i) {
            m_allocator.destroy(m_data + i);
        }

        // Shift remaining elements
        for (size_type i = start_index; i < m_size - count; ++i) {
            m_allocator.construct(m_data + i, std::move(m_data[i + count]));
            m_allocator.destroy(m_data + i + count);
        }

        m_size -= count;
        return iterator(m_data + start_index);
    }

    template <typename T, typename Allocator>
    void DVector<T, Allocator>::push_back(const T& value)
    {
        // Add element at the end - copy version
        if (m_size == m_capacity) {
            // Grow the vector - common strategy is to double the capacity
            size_type new_capacity = m_capacity == 0 ? 1 : m_capacity * 2;
            reallocate(new_capacity);
        }

        // Construct new element
        m_allocator.construct(m_data + m_size, value);
        ++m_size;
    }

    template <typename T, typename Allocator>
    void DVector<T, Allocator>::push_back(T&& value)
    {
        // Add element at the end - move version
        if (m_size == m_capacity) {
            // Grow the vector
            size_type new_capacity = m_capacity == 0 ? 1 : m_capacity * 2;
            reallocate(new_capacity);
        }

        // Move-construct new element
        m_allocator.construct(m_data + m_size, std::move(value));
        ++m_size;
    }

    template <typename T, typename Allocator>
    template <typename... Args>
    typename DVector<T, Allocator>::reference DVector<T, Allocator>::emplace_back(Args&&... args)
    {
        // Add element at the end - in-place construction
        if (m_size == m_capacity) {
            // Grow the vector
            size_type new_capacity = m_capacity == 0 ? 1 : m_capacity * 2;
            reallocate(new_capacity);
        }

        // Construct in-place
        m_allocator.construct(m_data + m_size, std::forward<Args>(args)...);
        return m_data[m_size++];
    }

    template <typename T, typename Allocator>
    void DVector<T, Allocator>::pop_back()
    {
        // Remove last element
        DS_ASSERT(m_size > 0, "DVector::pop_back - Vector is empty");

        --m_size;
        m_allocator.destroy(m_data + m_size);
    }

    template <typename T, typename Allocator>
    void DVector<T, Allocator>::resize(size_type count)
    {
        // Resize with default-constructed elements
        resize(count, T());
    }

    template <typename T, typename Allocator>
    void DVector<T, Allocator>::resize(size_type count, const value_type& value)
    {
        if (count > m_size) {
            // Grow the vector
            if (count > m_capacity) {
                reallocate(count);
            }

            // Construct new elements
            for (size_type i = m_size; i < count; ++i) {
                m_allocator.construct(m_data + i, value);
            }
        }
        else if (count < m_size) {
            // Shrink - destroy excess elements
            for (size_type i = count; i < m_size; ++i) {
                m_allocator.destroy(m_data + i);
            }
        }

        m_size = count;
    }

    template <typename T, typename Allocator>
    void DVector<T, Allocator>::swap(DVector& other) noexcept
    {
        // Swap all member variables
        std::swap(m_data, other.m_data);
        std::swap(m_size, other.m_size);
        std::swap(m_capacity, other.m_capacity);
        std::swap(m_allocator, other.m_allocator);
    }

    /////////////////////////////////////////////////////////
    // Helper Methods
    /////////////////////////////////////////////////////////

    template <typename T, typename Allocator>
    void DVector<T, Allocator>::reallocate(size_type new_cap)
    {
        // Reallocate vector with larger capacity
        // Allocate new buffer
        pointer new_data = m_allocator.allocate(new_cap);

        // Move elements to new buffer if we had existing elements
        if (m_size > 0) {
            for (size_type i = 0; i < m_size; ++i) {
                m_allocator.construct(new_data + i, std::move(m_data[i]));
                m_allocator.destroy(m_data + i);
            }
        }

        // Deallocate old buffer if it existed
        if (m_data) {
            m_allocator.deallocate(m_data, m_capacity);
        }

        // Update members
        m_data = new_data;
        m_capacity = new_cap;
    }

    /////////////////////////////////////////////////////////
    // Non-member functions
    /////////////////////////////////////////////////////////

    template <typename T, typename Allocator>
    bool operator==(const DVector<T, Allocator>& lhs, const DVector<T, Allocator>& rhs)
    {
        if (lhs.size() != rhs.size()) {
            return false;
        }

        for (typename DVector<T, Allocator>::size_type i = 0; i < lhs.size(); ++i) {
            if (!(lhs[i] == rhs[i])) {
                return false;
            }
        }

        return true;
    }

    template <typename T, typename Allocator>
    bool operator!=(const DVector<T, Allocator>& lhs, const DVector<T, Allocator>& rhs)
    {
        return !(lhs == rhs);
    }

    template <typename T, typename Allocator>
    bool operator<(const DVector<T, Allocator>& lhs, const DVector<T, Allocator>& rhs)
    {
        return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    template <typename T, typename Allocator>
    bool operator>(const DVector<T, Allocator>& lhs, const DVector<T, Allocator>& rhs)
    {
        return rhs < lhs;
    }

    template <typename T, typename Allocator>
    bool operator<=(const DVector<T, Allocator>& lhs, const DVector<T, Allocator>& rhs)
    {
        return !(rhs < lhs);
    }

    template <typename T, typename Allocator>
    bool operator>=(const DVector<T, Allocator>& lhs, const DVector<T, Allocator>& rhs)
    {
        return !(lhs < rhs);
    }

    template <typename T, typename Allocator>
    void swap(DVector<T, Allocator>& lhs, DVector<T, Allocator>& rhs) noexcept
    {
        lhs.swap(rhs);
    }


} // namespace ds::core::containers