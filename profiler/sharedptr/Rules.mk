
LIB:= sharedptr

SRCS=	\
	SharedString.cpp \

TESTS := \
	SharedPtr \
	SharedString \

TEST_SHAREDSTRING_SRCS= \
	SharedString.cpp \

TEST_SHAREDSTRING_WRAPFUNCS= \
	_Znwm=mock_new \
	_ZdlPv=mock_delete \
