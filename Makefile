# from http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/
#

CXXFLAGS = -g -Wall -Wextra -pthread -std=c++14

DEPDIR := .d
$(shell mkdir -p $(DEPDIR) >/dev/null)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
COMPILE.cc = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
POSTCOMPILE = @mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d && touch $@

%.o : %.c
%.o : %.c $(DEPDIR)/%.d
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

%.o : %.cpp
%.o : %.cpp $(DEPDIR)/%.d
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

SRCS = ciSpy.cpp common.cpp filesystem.cpp network.cpp pwm.cpp

all: ciSpy

.PHONY: test
test: all
	$(MAKE) -C test all

.PHONY: test-clean
test-clean:
	$(MAKE) -C test clean

clean: test-clean
	rm -f *.o ciSpy
	rm -f $(DEPDIR)/*

ciSpy: common.o pwm.o network.o filesystem.o ciSpy.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -lpthread $^ -o $@

include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS))))
