CXX = g++
CXXFLAGS = -O3 -s
LDFLAGS = -lgmp -lgmpxx

.PHONY: all
all: wiener

wiener: wiener.cc
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(<) -o $(@)

.PHONY: test
test: wiener
	./wiener-test

.PHONY: clean
clean:
	rm -f wiener

# vim:ts=4 sts=4 sw=4 noet
