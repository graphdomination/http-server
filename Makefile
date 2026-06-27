CXX=c++
CXXFLAGS=-Wall -Wextra -Wpedantic -std=c++20
LINK=-lssl -lcrypto
SRC=*.hpp *.cpp

run:
	$(CXX) $(CXXFLAGS) $(SRC) -o server $(LINK) && ./server 8080