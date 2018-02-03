
LIB:= image

SRCS:=	\
	Image.cpp \

SUBDIRS := \
	factory


TESTS := \
	Image \

TEST_IMAGE_SRCS := \
	Image.cpp \

TEST_IMAGE_LIBS := \
	imagefactory \
	sharedptr \

TEST_IMAGE_STDLIBS := \
	gmock \
