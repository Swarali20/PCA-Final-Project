APP_NAME=parallel
OBJS += parallel.o
# OBJS += main.o

CXX = mpic++
CXXFLAGS = -I. -O3 -std=c++11#-Wall -Wextra

default: $(APP_NAME)

$(APP_NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

%.o: %.cpp
	$(CXX) $< $(CXXFLAGS) -c -o $@

clean:
	/bin/rm -rf *~ *.o $(APP_NAME) *.class