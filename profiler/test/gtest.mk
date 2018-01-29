
TOPDIR=../

.undefine TEST_PROGS

.for test in ${TESTS}

TEST_${test:tu}_OBJPATHS=
.ifdef TEST_${test:tu}_WRAPFUNCS

TEST_${test:tu}_REDEFINE_SYMS=
.for sym in ${TEST_${test:tu}_WRAPFUNCS}
TEST_${test:tu}_REDEFINE_SYMS += --redefine-sym ${sym}
.endfor

.for obj in ${TEST_${test:tu}_OBJS}
TEST_${test:tu}_OBJPATHS+=${obj:R}.${test}.test.o

${obj:R}.${test}.test.o: ${TOPDIR}/${obj}
	objcopy ${TEST_${test:tu}_REDEFINE_SYMS} $> $@
.endfor

CLEANFILES += ${TEST_${test:tu}_OBJPATHS}
.else
.for obj in ${TEST_${test:tu}_OBJS}
TEST_${test:tu}_OBJPATHS+=${TOPDIR}/${obj}
.endfor
.endif

TEST_${test:tu}_LIBARGS=
.for lib in ${TEST_${test:tu}_LIBS}
TEST_${test:tu}_LIBARGS+=-l${lib}
.endfor

TEST_${test:tu}_LIBARGS+=-lgtest -lgtest_main -lpthread

TEST_PROGS += ${test}.testprog

${test}.testprog: ${TEST_${test:tu}_OBJPATHS} ${test}.gtest.o
	${CXX} -Wl,-L/usr/local/lib ${.ALLSRC} ${TEST_${test:tu}_LIBARGS} -o $@

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
