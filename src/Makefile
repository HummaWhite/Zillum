target = test.exe
flags = -std=c++17 -g -lgdi32 -O3
CC = g++

src = $(wildcard ./*.cpp ./Accelerator/*.cpp ./Buffer/*.cpp ./Display/*.cpp ./Environment/*.cpp ./Integrator/*.cpp \
	./Material/*.cpp ./Math/*.cpp ./Microfacet/*.cpp ./Sampler/*.cpp ./Scene/*.cpp ./Surface/*.cpp ./stb_image/*.cpp)

obj = $(patsubst %.cpp, %.o, $(src))

inc = $(patsubst %.cpp, %.h, $(src))

all : $(target)

$(target) : $(obj)
	$(CC) $(obj) -o $(target) $(flags)

%.o : %.cpp
	$(CC) -c $< -o $@ $(flags)

%.cpp : %.h

clean :
	rm ./*.o ./Accelerator/*.o ./Buffer/*.o ./Display/*.o ./Environment/*.o ./Integrator/*.o \
	./Material/*.o ./Math/*.o ./Microfacet/*.o ./Sampler/*.o ./Scene/*.o
