CXX=g++
CXXFLAGS=-g -O0 -Wall
INC_DIRS=-I . -I ./H3Table -I ./H3QPack -I ../../msquic/src/inc
LIB_DIRS=-L ../../msquic/artifacts/bin/linux/x64_Debug_openssl
LIB_NAME=msquic
BIN_NAME=H3Client
DEF_FLAGS=-DPTR_SIZE=8 -DLITTLE_ENDIAN_ARCH=1

SRCS=./H3Client.c ./H3Table/H3Table.c ./H3QPack/H3QPack.c ./Huffman/HuffmanDecode.c ./Huffman/HuffmanEncode.c ./H3Parse/H3Parse.c
OBJS=$(patsubst %.c, %.o, $(SRCS))

%.o: %.c
	$(CXX) $(CXXFLAGS) $(DEF_FLAGS) $(INC_DIRS) -o $@ -c $^ 

BIN_NAME: $(OBJS)
	$(CXX) $(CXXFLAGS) $(DEF_FLAGS) $(INC_DIRS) -o $(BIN_NAME) $(OBJS) $(LIB_DIRS) -l$(LIB_NAME)

clean:
	@rm -f $(OBJS) $(BIN_NAME)
