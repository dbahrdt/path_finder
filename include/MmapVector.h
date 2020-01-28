//
// Created by sokol on 27.01.20.
//

#ifndef MASTER_ARBEIT_MMAPVECTOR_H
#define MASTER_ARBEIT_MMAPVECTOR_H
#include <cstddef>
#include <vector>
#include <string>
#include <sys/mman.h>
#include <iostream>
namespace pathFinder {

    template<typename T, typename Allocator>
    class MmapVector {
    private:
        T *_data;
        size_t _size;
    public:
        MmapVector();
        MmapVector(const std::vector<T> &ramVector, const char *filename);

        T &operator[](size_t index);

        T *begin();

        T *end();

        size_t size();

        void push_back(T element);
    };

    template<typename T, typename Allocator>
    MmapVector<T, Allocator>::MmapVector(const std::vector<T> &ramVector, const char *filename) {
        FILE *file = fopen64(filename, "w+");
        fwrite(ramVector.data(), sizeof(T), ramVector.size(), file);
        fflush(file);

        _data = (T *) mmap64(nullptr, sizeof(T) * ramVector.size(), PROT_READ | PROT_WRITE, MAP_SHARED, fileno(file),
                             0);
        _size = ramVector.size();
    }
    template<typename T, typename Allocator>
    T &MmapVector<T, Allocator>::operator[](size_t index) {
        return _data[index];
    }
    template<typename T, typename Allocator>
    T *MmapVector<T, Allocator>::begin() {
        return _data;
    }
    template<typename T, typename Allocator>
    T *MmapVector<T, Allocator>::end() {
        return _data + _size;
    }
    template<typename T, typename Allocator>
    size_t MmapVector<T, Allocator>::size() {
        return _size;
    }
    template<typename T, typename Allocator>
    MmapVector<T, Allocator>::MmapVector() {

    }

    template<typename T, typename Allocator>
    void MmapVector<T, Allocator>::push_back(T element) {

    }

}
#endif //MASTER_ARBEIT_MMAPVECTOR_H
