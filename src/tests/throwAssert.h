#ifdef _TEST
#ifndef THROW_ASSERT_H
#define THROW_ASSERT_H
#include <string>

#define throwAssert(COND) throwAssertInner(COND, __FILE__, __LINE__, #COND)

using namespace std;

void throwAssertInner(bool cond, string file, int line, string condString);

#endif /* THROW_ASSERT_H */
#endif /* _TEST */
