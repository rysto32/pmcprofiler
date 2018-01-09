
TOPDIR=../

.undefine TEST_PROGS

.for test in ${TESTS}

.undefine TEST_OBJS
.for obj in ${TEST_${test}_OBJS}
TEST_OBJS+=${TOPDIR}/${obj}
.endfor
TEST_PROGS += ${test}.testprog

${test}.testprog: ${test}.cxxtest.o ${TEST_OBJS}
	${CXX} $> -o $@

SRCS += ${test}.cxxtest.cpp

${test}.cxxtest.cpp: ${test}.cxxtest
	cxxtestgen.py --error-printer -o $@ $>

.endfor

test: ${TEST_PROGS}
.for test in ${TEST_PROGS}
	@./${test}
.endfor

CFLAGS += -I/usr/local/include
