target = test.exe
flags = -std=c++17 -g -lgdi32 -O3
CC = g++

src = $(wildcard ./src/*.cpp ./src/Accelerator/*.cpp ./src/Buffer/*.cpp ./src/Display/*.cpp ./src/Integrator/*.cpp \
	./src/Material/*.cpp ./src/Material/Microfacet/*.cpp ./src/Math/*.cpp ./src/Sampler/*.cpp ./src/Scene/*.cpp \
	./src/Scene/Environment/*.cpp ./src/Scene/Camera/*.cpp ./src/Scene/Shape/*.cpp ./src/stb_image/*.cpp)

obj = $(patsubst %.cpp, %.o, $(src))

inc = $(patsubst %.cpp, %.h, $(src))

all : $(target)

$(target) : $(obj)
	$(CC) $(obj) -o $(target) $(flags)

%.o : %.cpp
	$(CC) -c $< -o $@ $(flags)

%.cpp : %.h

clean :
	rm ./src/*.o ./src/Accelerator/*.o ./src/Buffer/*.o ./src/Display/*.o ./src/Integrator/*.o \
	./src/Material/*.o ./src/Material/Microfacet/*.o ./src/Math/*.o ./src/Sampler/*.o ./src/Scene/*.o \
	./src/Scene/Environment/*.o ./src/Scene/Camera/*.o ./src/Scene/Shape/*.o ./src/stb_image/*.o)