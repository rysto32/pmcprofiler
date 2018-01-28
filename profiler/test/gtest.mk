
TOPDIR=../

.undefine TEST_PROGS

.for test in ${TESTS}

.undefine TEST_OBJS
.for obj in ${TEST_${test:tu}_OBJS}
TEST_OBJS+=${TOPDIR}/${obj}
.endfor
TEST_PROGS += ${test}.testprog

${test}.testprog: ${test}.gtest.o ${TEST_OBJS}
	${CXX} -Wl,-L/usr/local/lib $> -lgtest -lgtest_main -lpthread -o $@

SRCS += ${test}.gtest.cpp

CLEANFILES +=${test}.gtest.o ${test}.testprog

.PHONY: test.${test}

test.${test}: ${test}.testprog
	@./${test}.testprog

.endfor

.PHONY: test

test: ${TEST_PROGS}
.for test in ${TEST_PROGS}
	@./${test}
.endfor

CXXFLAGS=-I/usr/local/include -I${TOPDIR} -std=c++17

.include <bsd.prog.mk>
