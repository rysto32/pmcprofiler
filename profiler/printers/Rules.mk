
LIB :=	printers

SRCS := \
	CallchainProfilePrinter.cpp \
	ProfilePrinter.cpp \

TESTS := \
	ProfilePrinter \

TEST_PROFILEPRINTER_SRCS := \
	ProfilePrinter \

TEST_PROFILEPRINTER_LIBS := \
	sharedptr \
	frame \
	samples \


