
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

TEST_CALLCHAIN_LIBS= \
	gmock

TEST_CALLFRAME_OBJS=Callframe.o

TEST_EVENTFACTORY_OBJS= \
	EventFactory.o

TEST_EVENTFACTORY_WRAPFUNCS= \
	open=mock_open \
	close=mock_close \

TEST_EVENTFACTORY_LIBS= \
	gmock


.include "gtest.mk"
