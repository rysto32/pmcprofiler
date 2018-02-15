
LIB :=	samples

SRCS :=	\
	AddressSpace.cpp \
	SampleAggregation.cpp \

SUBDIRS := \
	aggfactory \
	spacefactory \

TESTS := \
	AddressSpace \
	SampleAggregation

TEST_ADDRESSSPACE_SRCS := \
	AddressSpace.cpp \

TEST_ADDRESSSPACE_LIBS := \
	frame \
	sharedptr \
	imagefactory \

TEST_ADDRESSSPACE_WRAPFUNCS= \
	close=mock_close \
	exit=mock_exit \
	fprintf=mock_fprintf \
	open=mock_open \

TEST_ADDRESSSPACE_STDLIBS= \
	gmock \

TEST_SAMPLEAGGREGATION_SRCS := \
	SampleAggregation.cpp \

TEST_SAMPLEAGGREGATION_LIBS := \
	sharedptr \
