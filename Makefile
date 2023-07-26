.PHONY: all clean

BIN_DIR = ./build

all: server client multiclient

$(BIN_DIR)/server.o: ./Server/server.c | $(BIN_DIR)
	gcc -c ./Server/server.c -o $@

$(BIN_DIR)/server: $(BIN_DIR)/server.o
	gcc $< -o $@ -lpthread

$(BIN_DIR)/client.o: ./Client/client.c | $(BIN_DIR)
	gcc -c ./Client/client.c -o $@

$(BIN_DIR)/client: $(BIN_DIR)/client.o
	gcc $< -o $@ -lpthread

$(BIN_DIR)/multiclient.o: ./Client/multiclient.c | $(BIN_DIR)
	gcc -c ./Client/multiclient.c -o $@

$(BIN_DIR)/multiclient: $(BIN_DIR)/multiclient.o
	gcc $< -o $@ -lpthread

server: $(BIN_DIR)/server
client: $(BIN_DIR)/client
multiclient: $(BIN_DIR)/multiclient

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BIN_DIR)

