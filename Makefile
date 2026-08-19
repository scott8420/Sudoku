# Makefile — build the pure sudoku::model engine and its selftest with nothing
# but a C++17 compiler. No CMake, no gtkmm, no external libraries. This is the
# "a student lifted model/ into their own project" build path; if it works with
# a bare `make` here, it works anywhere.
#
#   make            # build ./selftest
#   make run        # build and run it
#   make clean

CXX      ?= c++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Iinclude

MODEL_SRC := \
  src/model/Grid.cpp \
  src/model/WorkGrid.cpp \
  src/model/Techniques.cpp \
  src/model/Solver.cpp \
  src/model/Rater.cpp \
  src/model/Generator.cpp

SELFTEST_SRC := src/model/selftest.cpp

selftest: $(MODEL_SRC) $(SELFTEST_SRC)
	$(CXX) $(CXXFLAGS) $(MODEL_SRC) $(SELFTEST_SRC) -o selftest

.PHONY: run clean
run: selftest
	./selftest

clean:
	rm -f selftest
