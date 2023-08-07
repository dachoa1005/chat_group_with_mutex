.PHONY: all clean

BIN_DIR = ./build

all: server client clone-client ser-test

$(BIN_DIR)/server.o: ./Server/server.c | $(BIN_DIR)
	gcc -c -Wall ./Server/server.c -o $@

$(BIN_DIR)/server: $(BIN_DIR)/server.o
	gcc $< -Wall -o $@ -lpthread

$(BIN_DIR)/ser-test.o: ./Server/ser-test.c | $(BIN_DIR)
	gcc -c -Wall ./Server/ser-test.c -o $@

$(BIN_DIR)/ser-test: $(BIN_DIR)/ser-test.o
	gcc $< -Wall -o $@ -lpthread


$(BIN_DIR)/client.o: ./Client/client.c | $(BIN_DIR)
	gcc -c -Wall ./Client/client.c -o $@

$(BIN_DIR)/client: $(BIN_DIR)/client.o
	gcc $< -Wall -o $@ -lpthread


$(BIN_DIR)/clone-client.o: ./Client/clone-client.c | $(BIN_DIR)
	gcc -c ./Client/clone-client.c -o $@

$(BIN_DIR)/clone-client: $(BIN_DIR)/clone-client.o
	gcc $< -o $@ -lpthread

server: $(BIN_DIR)/server
client: $(BIN_DIR)/client
clone-client: $(BIN_DIR)/clone-client
ser-test: $(BIN_DIR)/ser-test


$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BIN_DIR)

