CXX = g++
CXXFLAGS = -std=c++17 -Iinclude -Wall -Wextra

TARGETS = test_lexer test_parser test_store test_integration test_array test_multi_module test_memory_span run_testdata

SRCS = src/MemoryStore.cpp src/Interpreter.cpp src/Parser.cpp src/Lexer.cpp src/AST.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGETS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test_multi_module: tests/test_multi_module.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) tests/test_multi_module.cpp $(OBJS) -o test_multi_module

test_lexer: tests/test_lexer.cpp src/Lexer.o
	$(CXX) $(CXXFLAGS) tests/test_lexer.cpp src/Lexer.o -o test_lexer

test_parser: tests/test_parser.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) tests/test_parser.cpp $(OBJS) -o test_parser

test_store: tests/test_memory_store.cpp src/MemoryStore.o
	$(CXX) $(CXXFLAGS) tests/test_memory_store.cpp src/MemoryStore.o -o test_store

test_integration: tests/test_integration.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) tests/test_integration.cpp $(OBJS) -o test_integration

test_array: tests/test_array.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) tests/test_array.cpp $(OBJS) -o test_array

test_memory_span: tests/test_memory_span.cpp src/MemoryStore.o
	$(CXX) $(CXXFLAGS) tests/test_memory_span.cpp src/MemoryStore.o -o test_memory_span

run_testdata: testdata/run_testdata.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) testdata/run_testdata.cpp $(OBJS) -o run_testdata

clean:
	rm -f $(TARGETS) src/*.o
