APP_NAME=simd

OBJS=simd.o 

CXX = g++ -m64 -std=c++11 -g
CXXFLAGS = -I. -mfma -mavx 

default: $(APP_NAME)

$(APP_NAME): $(OBJS) 
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)
	
%.o: %.cpp
	$(CXX) $< $(CXXFLAGS) -c -o $@

clean:
	/bin/rm -rf *~ *.o $(APP_NAME) *.class