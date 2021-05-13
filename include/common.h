#ifndef __COMMON_H__
#define __COMMON_H__

#include <cstdio>
#include <ctime>
#include <string>
#include <vector>

#define iter(vec, id) for (unsigned int id = 0; id < vec.size(); ++id)


#define iter_range(vec, id, start, end) \
    for (unsigned int id = start; id < end; ++id)

std::vector<std::string> split(char *text, char delimiter);

template <class T>
std::vector<T> operator+(std::vector<T> &op1, std::vector<T> op2)
{
    std::vector<T> result(op1.begin(), op1.end());
    iter(op2, i) { result.push_back(op2[i]); }
    return result;
}

template <class T>
std::vector<T> operator+=(std::vector<T> &op1, std::vector<T> op2)
{
    iter(op2, i) { op1.push_back(op2[i]); }
    return op1;
}

/**
 * timeConverter() - convert text to time_t
 */
time_t timeConverter(char *text);

#define cudaCheck(err, msg)   \
    if (err != cudaSuccess) { \
        perror(msg);          \
        exit(-1);             \
    }



int cmpint(const void *a, const void *b);

int cmpdouble(const void *a, const void *b);

#endif
