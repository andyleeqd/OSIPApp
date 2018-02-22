CC = g++ -std=c++11
CFLAGS = -I/usr/local/cuda-9.0/include -I../common
LDFLAGS = -L/usr/local/cuda-9.0/lib64 -L/home/zhuoshi/Zs/TR/TensorRT-3.0.2/targets/x86_64-linux-gnu/lib
LDFLAGS += -L/home/zhuoshi/Zs/Bin -L/home/xu/deepstream/lib -L. -L/usr/local/lib
LDFLAGS += -ldeepstream -lpthread -lcudart -lnvparsers -lnvinfer -lcudnn -lcublas -lopencv_core -lopencv_highgui -lopencv_imgcodecs -lopencv_text -lopencv_calib3d -lopencv_freetype -lopencv_tracking
SRC = nvCuvidPerf.cpp

all:
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o dech264_to_rgb
clean:
	rm -rf *.o dech264_to_rgb


