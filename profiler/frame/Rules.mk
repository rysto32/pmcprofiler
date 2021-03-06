
SUBDIRS := \
	callchainFactory

LIB :=	frame

SRCS := \
	Callchain.cpp \
	Callframe.cpp \

TESTS := \
	Callchain \
	Callframe \
	InlineFrame \

TEST_CALLCHAIN_SRCS := \
	Callchain.cpp \
	Callframe.cpp \

TEST_CALLCHAIN_LIBS := \
	sharedptr \

TEST_CALLCHAIN_STDLIBS := \
	gmock \

TEST_CALLFRAME_SRCS := \
	Callframe.cpp \

TEST_CALLFRAME_LIBS := \
	sharedptr \

TEST_INLINEFRAME_LIBS := \
	sharedptr \
