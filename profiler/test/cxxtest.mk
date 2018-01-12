
TOPDIR=../

.undefine TEST_PROGS

.for test in ${TESTS}

.undefine TEST_OBJS
.for obj in ${TEST_${test:tu}_OBJS}
TEST_OBJS+=${TOPDIR}/${obj}
.endfor
TEST_PROGS += ${test}.testprog

${test}.testprog: ${test}.cxxtest.o ${TEST_OBJS}
	${CXX} $> -o $@

SRCS += ${test}.cxxtest.cpp

${test}.cxxtest.cpp: ${test}.test
	cxxtestgen.py --error-printer -o $@ $>

CLEANFILES +=${test}.cxxtest.o ${test}.cxxtest.cpp ${test}.testprog

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
