#include <cstddef>
#include <ostream>
#include <istream>
#include <iterator>
#include <cstdint>
#include <memory>
#include <limits>
#include <initializer_list>

template <class T>
class Vector {
private:
    T* _array;       // Указатель на динамически выделенный массив элементов типа T
    size_t _size;    // Текущий размер вектора (количество элементов)
    size_t _capacity;// Текущая ёмкость (выделенная память)
    // Вспомогательная функция для разрушения элементов в диапазоне [first, last).
    // Мы вызываем деструкторы явно, потому что память выделена как сырые байты (int8_t[]),
    // и delete[] на T* вызвал бы деструкторы для всех _capacity элементов, включая неинициализированные,
    // что привело бы к UB (undefined behavior). Поэтому разрушаем только до _size.
    void destroy_elements(T* first, T* last) {
        while (first != last) {
            (--last)->~T(); //? явно вызываем деструктор, для объектов которые существуют
        }
    }
public:
    // Итератор: реализуем как random_access_iterator для полной совместимости.
    // Наследуем от std::iterator для автоматического определения trait'ов.
    // Это позволяет использовать наш вектор в алгоритмах STL.
    class iterator {
    private:
        T* ptr;
    public:
        iterator(T* p = nullptr) : ptr(p) {}
        T& operator*() const { return *ptr; }
        T* operator->() const { return ptr; }
        iterator& operator++() { ++ptr; return *this; }
        iterator operator++(int) { iterator tmp = *this; ++ptr; return tmp; }
        iterator& operator--() { --ptr; return *this; }
        iterator operator--(int) { iterator tmp = *this; --ptr; return tmp; }
        iterator& operator+=(ptrdiff_t n) { ptr += n; return *this; }
        iterator& operator-=(ptrdiff_t n) { ptr -= n; return *this; }
        iterator operator+(ptrdiff_t n) const { return iterator(ptr + n); }
        iterator operator-(ptrdiff_t n) const { return iterator(ptr - n); }
        ptrdiff_t operator-(const iterator& other) const { return ptr - other.ptr; }
        bool operator==(const iterator& other) const { return ptr == other.ptr; }
        bool operator!=(const iterator& other) const { return ptr != other.ptr; }
        bool operator<(const iterator& other) const { return ptr < other.ptr; }
        bool operator>(const iterator& other) const { return ptr > other.ptr; }
        bool operator<=(const iterator& other) const { return ptr <= other.ptr; }
        bool operator>=(const iterator& other) const { return ptr >= other.ptr; }
        T& operator[](ptrdiff_t n) const { return *(ptr + n); }
    };
    // Const-итератор: аналогично iterator, но для const-доступа.
    // Конструктор от не-const итератора позволяет implicit conversion.
    class const_iterator {
    private:
        const T* ptr;
    public:
        const_iterator(const T* p = nullptr) : ptr(p) {}
        const_iterator(const iterator& it) : ptr(it.operator->()) {}
        const T& operator*() const { return *ptr; }
        const T* operator->() const { return ptr; }
        const_iterator& operator++() { ++ptr; return *this; }
        const_iterator operator++(int) { const_iterator tmp = *this; ++ptr; return tmp; }
        const_iterator& operator--() { --ptr; return *this; }
        const_iterator operator--(int) { const_iterator tmp = *this; --ptr; return tmp; }
        const_iterator& operator+=(ptrdiff_t n) { ptr += n; return *this; }
        const_iterator& operator-=(ptrdiff_t n) { ptr -= n; return *this; }
        const_iterator operator+(ptrdiff_t n) const { return const_iterator(ptr + n); }
        const_iterator operator-(ptrdiff_t n) const { return const_iterator(ptr - n); }
        ptrdiff_t operator-(const const_iterator& other) const { return ptr - other.ptr; }
        bool operator==(const const_iterator& other) const { return ptr == other.ptr; }
        bool operator!=(const const_iterator& other) const { return ptr != other.ptr; }
        bool operator<(const const_iterator& other) const { return ptr < other.ptr; }
        bool operator>(const const_iterator& other) const { return ptr > other.ptr; }
        bool operator<=(const const_iterator& other) const { return ptr <= other.ptr; }
        bool operator>=(const const_iterator& other) const { return ptr >= other.ptr; }
        const T& operator[](ptrdiff_t n) const { return *(ptr + n); }
    };
    // Reverse итераторы: используем std::reverse_iterator для простоты и совместимости.
    // Это оборачивает наши итераторы, чтобы итерация шла в обратном порядке.
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    //! Конструкторы
    // Конструктор с размером и значением: выделяем память как сырые байты для избежания
    // вызова дефолтных конструкторов на всю ёмкость (что неэффективно для дорогих T).
    // Используем std::uninitialized_fill для правильной инициализации только _size элементов.
    // Try-catch для exception safety: если fill кинет, освобождаем память.
    Vector(size_t size = 0, const T& value = T())  : _size(size), _capacity(size), _array(nullptr) {
        if (_capacity > 0) {
            _array = reinterpret_cast<T*>(new int8_t[_capacity * sizeof(T)]);
            try {
                std::uninitialized_fill(_array, _array + _size, value); //? данная функция проделывает те же шаги что и написано ниже в комментарии
            } catch (...) {
                delete[] reinterpret_cast<int8_t*>(_array); //? при переинтерпретации к типу int_t деструктор не вызывается
                // просто delete[] _array; плохо, т.к. мы начнем вызывать деструктор от объектов которые еще не созданы (если _size < _capacity)
                throw;
            }
            // try {
            // for ( ; i < _size; ++i) {
            // new (newArray + i) T(_array[i]); //? placement new вызывает конструктор T в сырой памяти по адресу newArray + i, правильно инициализируя объект
            // }
            // } catch (...) {
            // for (size_t j = 0; j < i; --j) {
            // (newArray + j)->~T();
            // }
            // delete[] reinterpret_cast<int8_t*>(newArray);
            // throw;
            // }
        }
    }
    // Copy-конструктор: аналогично, копируем только _size элементов с uninitialized_copy.
    // Это обеспечивает strong exception guarantee: если копирование кинет на середине,
    // память освобождается, и объект не создаётся частично.
    Vector(const Vector& other)  : _size(other._size), _capacity(other._capacity), _array(nullptr) {
        if (_capacity > 0) {
            _array = reinterpret_cast<T*>(new int8_t[_capacity * sizeof(T)]);
            try {
                std::uninitialized_copy(other._array, other._array + _size, _array);
            } catch (...) {
                delete[] reinterpret_cast<int8_t*>(_array);
                throw;
            }
        }
    }
    // Move-конструктор: просто крадём ресурсы у other, оставляя его в валидном состоянии (пустом).
    // noexcept для оптимизаций (например, в контейнерах).
    Vector(Vector&& other)  noexcept : _size(other._size), _capacity(other._capacity), _array(other._array) {
        other._array = nullptr;
        other._size = 0;
        other._capacity = 0;
    }
    // Конструктор от initializer_list: копируем из списка, как из диапазона.
    Vector(std::initializer_list<T> init) : _size(init.size()), _capacity(init.size()), _array(nullptr) {
        if (_capacity > 0) {
            _array = reinterpret_cast<T*>(new int8_t[_capacity * sizeof(T)]);
            try {
                std::uninitialized_copy(init.begin(), init.end(), _array);
            } catch (...) {
                delete[] reinterpret_cast<int8_t*>(_array);
                throw;
            }
        }
    }

    // Деструктор: разрушаем только инициализированные элементы, затем освобождаем сырую память.
    // delete[] на int8_t* не вызывает деструкторы T, что правильно, т.к. мы уже вызвали ~T() вручную.
    ~Vector() {
        destroy_elements(_array, _array + _size);
        delete[] reinterpret_cast<int8_t*>(_array);
    }

    // Операторы присваивания
    // Copy assignment: используем copy-and-swap идиому для strong exception safety.
    // Создаём временную копию, затем swap'аем с ней. Если копия кинет, *this не изменится.
    Vector<T>& operator=(const Vector& other) {
        if (this != &other) {
            Vector tmp(other);
            swap(tmp);
        }
        return *this;
    }
    // Move assignment: разрушаем свои элементы, крадём у other, оставляем other пустым.
    Vector<T>& operator=(Vector&& other) noexcept {
        if (this != &other) {
            destroy_elements(_array, _array + _size);
            delete[] reinterpret_cast<int8_t*>(_array);
            _array = other._array;
            _size = other._size;
            _capacity = other._capacity;
            other._array = nullptr;
            other._size = 0;
            other._capacity = 0;
        }
        return *this;
    }
    // Assignment от initializer_list: делегируем в assign.
    Vector<T>& operator=(std::initializer_list<T> init) {
        assign(init.begin(), init.end());
        return *this;
    }

    // Размер и ёмкость
    // size и capacity: простые геттеры.
    size_t size() const { return _size; }
    size_t capacity() const { return _capacity; }
    // empty: проверка на нулевой размер.
    bool empty() const { return _size == 0; }
    // max_size: теоретический лимит, чтобы избежать overflow при выделении.
    size_t max_size() const { return std::numeric_limits<size_t>::max() / sizeof(T); }
    // reserve: расширяем ёмкость, если нужно. Копируем элементы в новый буфер с uninitialized_copy.
    // Если копирование кинет, освобождаем новый буфер, не трогаем старый (strong guarantee).
    // Затем разрушаем старые и освобождаем.
    void reserve(size_t newCapacity) {
        if (newCapacity <= _capacity) return;
        T* newArray = reinterpret_cast<T*>(new int8_t[newCapacity * sizeof(T)]);
        try {
            std::uninitialized_copy(_array, _array + _size, newArray);
        } catch (...) {
            delete[] reinterpret_cast<int8_t*>(newArray);
            throw;
        }
        destroy_elements(_array, _array + _size);
        delete[] reinterpret_cast<int8_t*>(_array);
        _array = newArray;
        _capacity = newCapacity;
    }
    // resize: если уменьшаем - разрушаем лишние; если увеличиваем - reserve + uninitialized_fill.
    void resize(size_t newSize, const T& value) {
        if (newSize < _size) {
            destroy_elements(_array + newSize, _array + _size);
        } else if (newSize > _size) {
            if (newSize > _capacity) reserve(newSize);
            try {
                std::uninitialized_fill(_array + _size, _array + newSize, value);
            } catch (...) {
                throw;
            }
        }
        _size = newSize;
    }
    // shrink_to_fit: создаём копию с минимальной ёмкостью и swap'аем.
    void shrink_to_fit() {
        if (_capacity > _size) {
            Vector tmp(*this);
            swap(tmp);
        }
    }
    // Доступ к элементам
    // operator[]: без проверки для скорости (как в std::vector).
    T& operator[](size_t i) { return _array[i]; }
    const T& operator[](size_t i) const { return _array[i]; }
    // at: с проверкой и out_of_range exception для безопасности.
    T& at(size_t i) {
        if (i >= _size) throw std::out_of_range("Index out of range");
        return _array[i];
    }
    const T& at(size_t i) const {
        if (i >= _size) throw std::out_of_range("Index out of range");
        return _array[i];
    }
    // front/back: доступ к первому/последнему, предполагаем !empty() (как в std).
    T& front() { return _array[0]; }
    const T& front() const { return _array[0]; }
    T& back() { return _array[_size - 1]; }
    const T& back() const { return _array[_size - 1]; }
    // data: сырой указатель для C-совместимости.
    T* data() { return _array; }
    const T* data() const { return _array; }
    // Модификаторы
    // push_back (const&): reserve если нужно (удваиваем для amortized O(1)), placement new.
    void push_back(const T& value) {
        if (_size == _capacity) {
            reserve(_capacity == 0 ? 1 : _capacity * 2);
        }
        new (_array + _size) T(value);
        ++_size;
    }
    // push_back (&&): move для эффективности.
    void push_back(T&& value) {
        if (_size == _capacity) {
            reserve(_capacity == 0 ? 1 : _capacity * 2);
        }
        new (_array + _size) T(std::move(value));
        ++_size;
    }
    // pop_back: уменьшаем размер, вызываем деструктор (не освобождаем память).
    void pop_back() {
        if (_size > 0) {
            --_size;
            (_array + _size)->~T();
        }
    }
    // emplace_back: perfect forwarding для конструирования на месте без temp-копий.
    template <class... Args>
    void emplace_back(Args&&... args) {
        if (_size == _capacity) {
            reserve(_capacity == 0 ? 1 : _capacity * 2);
        }
        new (_array + _size) T(std::forward<Args>(args)...);
        ++_size;
    }
    // insert (const&): сдвигаем элементы вправо (uninitialized_copy для хвоста), вставляем.
    // Reserve если нужно. Для простоты не full optimized (можно улучшить с move_if_noexcept).
    Vector<T>::iterator insert(const_iterator pos, const T& value) {
        ptrdiff_t offset = pos - cbegin();
        if (_size == _capacity) reserve(_capacity == 0 ? 1 : _capacity * 2);
        iterator it = begin() + offset;
        if (it != end()) {
            std::uninitialized_copy(it, end(), it + 1);
            *it = value;
        } else {
            new (it.operator->()) T(value);
        }
        ++_size;
        return it;
    }
    // insert (&&): аналогично, но move.
    Vector<T>::iterator insert(const_iterator pos, T&& value) {
        ptrdiff_t offset = pos - cbegin();
        if (_size == _capacity) reserve(_capacity == 0 ? 1 : _capacity * 2);
        iterator it = begin() + offset;
        if (it != end()) {
            std::uninitialized_copy(it, end(), it + 1);
            *it = std::move(value);
        } else {
            new (it.operator->()) T(std::move(value));
        }
        ++_size;
        return it;
    }
    // insert (count, value): заполняем диапазон value'ами после сдвига.
    Vector<T>::iterator insert(const_iterator pos, size_t count, const T& value) {
        if (count == 0) return iterator(const_cast<T*>(pos.operator->()));
        ptrdiff_t offset = pos - cbegin();
        if (_size + count > _capacity) reserve(_size + count);
        iterator it = begin() + offset;
        if (it != end()) {
            std::uninitialized_copy(it, end(), it + count);
        }
        std::uninitialized_fill(it, it + count, value);
        _size += count;
        return it;
    }
    // insert (range): копируем из диапазона после сдвига.
    template <class InputIt>
    Vector<T>::iterator insert(const_iterator pos, InputIt first, InputIt last) {
        ptrdiff_t count = std::distance(first, last);
        if (count <= 0) return iterator(const_cast<T*>(pos.operator->()));
        ptrdiff_t offset = pos - cbegin();
        if (_size + count > _capacity) reserve(_size + count);
        iterator it = begin() + offset;
        if (it != end()) {
            std::uninitialized_copy(it, end(), it + count);
        }
        std::uninitialized_copy(first, last, it);
        _size += count;
        return it;
    }
    // insert (init_list): делегируем в range-версию.
    Vector<T>::iterator insert(const_iterator pos, std::initializer_list<T> init) {
        return insert(pos, init.begin(), init.end());
    }
    // emplace: как insert, но конструируем на месте.
    template <class... Args>
    Vector<T>::iterator emplace(const_iterator pos, Args&&... args) {
        ptrdiff_t offset = pos - cbegin();
        if (_size == _capacity) reserve(_capacity == 0 ? 1 : _capacity * 2);
        iterator it = begin() + offset;
        if (it != end()) {
            std::uninitialized_copy(it, end(), it + 1);
        }
        new (it.operator->()) T(std::forward<Args>(args)...);
        ++_size;
        return it;
    }
    // erase (single): move'аем хвост влево, разрушаем последний.
    Vector<T>::iterator erase(const_iterator pos) {
        ptrdiff_t offset = pos - cbegin();
        iterator it = begin() + offset;
        if (it != end()) {
            std::move(it + 1, end(), it);
            (--_size, (_array + _size)->~T());
        }
        return it;
    }
    // erase (range): move'аем хвост, разрушаем удалённые.
    Vector<T>::iterator erase(const_iterator first, const_iterator last) {
        ptrdiff_t count = last - first;
        if (count <= 0) return iterator(const_cast<T*>(last.operator->()));
        iterator it_first = begin() + (first - cbegin());
        iterator it_last = begin() + (last - cbegin());
        std::move(it_last, end(), it_first);
        destroy_elements(it_first + (end() - it_last), _array + _size);
        _size -= count;
        return it_first;
    }
    // clear: разрушаем все, размер=0, но не освобождаем память.
    void clear() {
        destroy_elements(_array, _array + _size);
        _size = 0;
    }
    // swap: noexcept, просто меняем поля.
    void swap(Vector& other) noexcept {
        std::swap(_array, other._array);
        std::swap(_size, other._size);
        std::swap(_capacity, other._capacity);
    }
    // assign (count, value): clear + fill.
    void assign(size_t count, const T& value) {
        clear();
        if (count > _capacity) reserve(count);
        std::uninitialized_fill(_array, _array + count, value);
        _size = count;
    }
    // assign (range): clear + copy.
    template <class InputIt>
    void assign(InputIt first, InputIt last) {
        clear();
        size_t count = std::distance(first, last);
        if (count > _capacity) reserve(count);
        std::uninitialized_copy(first, last, _array);
        _size = count;
    }
    // assign (init_list): делегируем.
    void assign(std::initializer_list<T> init) {
        assign(init.begin(), init.end());
    }
    // Итераторы: просто возвращаем с _array и _array + _size.
    Vector<T>::iterator begin() { return iterator(_array); }
    Vector<T>::iterator end() { return iterator(_array + _size); }
    Vector<T>::const_iterator begin() const { return const_iterator(_array); }
    Vector<T>::const_iterator end() const { return const_iterator(_array + _size); }
    Vector<T>::const_iterator cbegin() const { return const_iterator(_array); }
    Vector<T>::const_iterator cend() const { return const_iterator(_array + _size); }
    Vector<T>::reverse_iterator rbegin() { return reverse_iterator(end()); }
    Vector<T>::reverse_iterator rend() { return reverse_iterator(begin()); }
    Vector<T>::const_reverse_iterator rbegin() const { return const_reverse_iterator(cend()); }
    Vector<T>::const_reverse_iterator rend() const { return const_reverse_iterator(cbegin()); }
    Vector<T>::const_reverse_iterator crbegin() const { return const_reverse_iterator(cend()); }
    Vector<T>::const_reverse_iterator crend() const { return const_reverse_iterator(cbegin()); }
    // I/O: перегрузка << и >>.
    // Для << выводим элементы в формате [a, b, c] для читаемости.
    // Для >> читаем space-separated значения, очищая вектор сначала.
    // Это не стандарт для std::vector, но полезно для вашего исходного кода.
    friend std::ostream& operator<<(std::ostream& os, const Vector& vec) {
        os << "[";
        for (size_t i = 0; i < vec._size; ++i) {
            os << vec._array[i];
            if (i + 1 < vec._size) os << ", ";
        }
        os << "]";
        return os;
    }
    friend std::istream& operator>>(std::istream& is, Vector& vec) {
        vec.clear();
        T val;
        while (is >> val) {
            vec.push_back(val);
        }
        return is;
    }
};