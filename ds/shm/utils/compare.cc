#include "utils/compare.h"

#include <cstdlib>
#include <cstring>

#include "shm/mempool.h"
#include "shm/mm.h"

template <typename T>
bool key_equal(const T& a, const T& b) {
  return a == b;
}

template <>
bool key_equal<const char*>(const char* const& a, const char* const& b) {
  return strcmp(a, b) == 0;
}

template <typename T>
int key_compare(const T& a, const T& b) {
  if (a < b) return -1;
  if (a > b) return 1;
  return 0;
}

template <>
int key_compare<const char*>(const char* const& a, const char* const& b) {
  return strcmp(a, b);
}

template <typename T>
void assign_to_shm(T& a, const T& b) {
  a = b;
}

template <>
void assign_to_shm<const char*>(const char*& a, const char* const& b) {
  int len = strlen(b) + 1;
  char* tmp = (char*)cacheable.malloc(len);
  strcpy(tmp, b);
  a = tmp;
}

template <typename T>
void assign_to_local(T& a, const T& b) {
  a = b;
}

template <>
void assign_to_local<const char*>(const char*& a, const char* const& b) {
  int len = strlen(b) + 1;
  char* tmp = (char*)malloc(len);
  strcpy(tmp, b);
  a = tmp;
}