# MT25088
CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread
LDFLAGS = -pthread

# Binary names
SERVER_A1 = server_a1
SERVER_A2 = server_a2
SERVER_A3 = server_a3
CLIENT_B = client_b

# Source files
SERVER_A1_SRC = server_a1.c
SERVER_A2_SRC = server_a2.c
SERVER_A3_SRC = server_a3.c
CLIENT_B_SRC = client_b.c
COMMON_H = common.h

.PHONY: all clean

all: $(SERVER_A1) $(SERVER_A2) $(SERVER_A3) $(CLIENT_B)

$(SERVER_A1): $(SERVER_A1_SRC) $(COMMON_H)
	$(CC) $(CFLAGS) -o $(SERVER_A1) $(SERVER_A1_SRC) $(LDFLAGS)

$(SERVER_A2): $(SERVER_A2_SRC) $(COMMON_H)
	$(CC) $(CFLAGS) -o $(SERVER_A2) $(SERVER_A2_SRC) $(LDFLAGS)

$(SERVER_A3): $(SERVER_A3_SRC) $(COMMON_H)
	$(CC) $(CFLAGS) -o $(SERVER_A3) $(SERVER_A3_SRC) $(LDFLAGS)

$(CLIENT_B): $(CLIENT_B_SRC) $(COMMON_H)
	$(CC) $(CFLAGS) -o $(CLIENT_B) $(CLIENT_B_SRC) $(LDFLAGS)

clean:
	rm -f $(SERVER_A1) $(SERVER_A2) $(SERVER_A3) $(CLIENT_B)
	rm -f *.o
	rm -rf experiment_data_v3
	rm -f final_results_v3.csv

# Help target
help:
	@echo "Available targets:"
	@echo "  all    - Build all servers and client (default)"
	@echo "  clean  - Remove all built files and experimental data"
	@echo "  help   - Show this help message"
