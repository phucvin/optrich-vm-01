CXX = g++
CXXFLAGS = -std=c++17 -Iinclude -Wall -Wextra

TARGETS = test_lexer test_parser test_store test_integration test_array test_multi_module

SRCS = src/MemoryStore.cpp src/Interpreter.cpp src/Parser.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGETS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test_multi_module: tests/test_multi_module.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) tests/test_multi_module.cpp $(OBJS) -o test_multi_module

test_lexer: tests/test_lexer.cpp include/Lexer.hpp
	$(CXX) $(CXXFLAGS) tests/test_lexer.cpp -o test_lexer

test_parser: tests/test_parser.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) tests/test_parser.cpp src/Parser.o src/MemoryStore.o src/Interpreter.o -o test_parser

test_store: tests/test_memory_store.cpp src/MemoryStore.o
	$(CXX) $(CXXFLAGS) tests/test_memory_store.cpp src/MemoryStore.o -o test_store

test_integration: tests/test_integration.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) tests/test_integration.cpp $(OBJS) -o test_integration

test_array: tests/test_array.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) tests/test_array.cpp $(OBJS) -o test_array

clean:
	rm -f $(TARGETS) src/*.o
