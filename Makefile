CXX = g++
CXXFLAGS = -std=c++17 -Iinclude -Wall -Wextra

TARGETS = test_lexer test_parser test_store test_integration test_array test_multi_module

all: $(TARGETS)

test_multi_module: tests/test_multi_module.cpp include/Interpreter.hpp include/Parser.hpp include/ObjectStore.hpp
	$(CXX) $(CXXFLAGS) tests/test_multi_module.cpp -o test_multi_module

test_lexer: tests/test_lexer.cpp include/Lexer.hpp
	$(CXX) $(CXXFLAGS) tests/test_lexer.cpp -o test_lexer

test_parser: tests/test_parser.cpp include/Parser.hpp include/AST.hpp include/Lexer.hpp
	$(CXX) $(CXXFLAGS) tests/test_parser.cpp -o test_parser

test_store: tests/test_object_store.cpp include/ObjectStore.hpp
	$(CXX) $(CXXFLAGS) tests/test_object_store.cpp -o test_store

test_integration: tests/test_integration.cpp include/Interpreter.hpp include/Parser.hpp include/ObjectStore.hpp
	$(CXX) $(CXXFLAGS) tests/test_integration.cpp -o test_integration

test_array: tests/test_array.cpp include/Interpreter.hpp include/Parser.hpp include/ObjectStore.hpp
	$(CXX) $(CXXFLAGS) tests/test_array.cpp -o test_array

clean:
	rm -f $(TARGETS)
