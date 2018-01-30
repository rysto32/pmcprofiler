
TESTS= \
	Callchain \
	Callframe \
	EventFactory \
	InlineFrame \
	SharedPtr \
	SharedString \

TEST_CALLCHAIN_OBJS= \
	Callchain.o \
	Callframe.o \
	SharedString.o \

TEST_CALLCHAIN_LIBS= \
	gmock

TEST_CALLFRAME_OBJS= \
	Callframe.o \
	SharedString.o \

TEST_EVENTFACTORY_OBJS= \
	EventFactory.o \
	SharedString.o \

TEST_EVENTFACTORY_WRAPFUNCS= \
	open=mock_open \
	close=mock_close \

TEST_EVENTFACTORY_LIBS= \
	gmock

TEST_INLINEFRAME_OBJS= \
	SharedString.o \

TEST_SHAREDSTRING_OBJS= \
	SharedString.o \

.include "gtest.mk"
