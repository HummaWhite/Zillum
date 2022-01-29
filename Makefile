target = Zillum.exe
compFlags = -std=c++17 -O3 -g
linkFlags = -lgdi32
CC = g++

src = $(wildcard ./src/*.cpp ./src/Accelerator/*.cpp ./src/Integrator/*.cpp \
	./src/Material/*.cpp ./src/Math/*.cpp ./src/Sampler/*.cpp ./src/Scene/*.cpp \
	./src/Scene/Environment/*.cpp ./src/Scene/Camera/*.cpp ./src/Scene/Shape/*.cpp ./ext/stb_image/*.cpp)

obj = $(patsubst %.cpp, %.o, $(src))

inc = $(patsubst %.cpp, %.h, $(src))

all : $(target)

$(target) : $(obj)
	$(CC) $(obj) -o $(target) $(linkFlags)

%.o : %.cpp
	$(CC) -c $< -o $@ $(compFlags)

%.cpp : %.h

clean :
	rm ./src/*.o ./src/Accelerator/*.o ./src/Integrator/*.o \
	./src/Material/*.o ./src/Math/*.o ./src/Sampler/*.o ./src/Scene/*.o \
	./src/Scene/Environment/*.o ./src/Scene/Camera/*.o ./src/Scene/Shape/*.o ./ext/stb_image/*.o