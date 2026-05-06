.PHONY: server
server: build/server

build/server: src/main.cpp src/parser.h
	mkdir -p $(@D)
	g++ -o build/server src/main.cpp

debug: build/server_d

build/server_d: src/main.cpp src/parser.h
	mkdir -p $(@D)
	g++ -o build/server_d src/main.cpp -g

clean:
	rm -rf build/ compile_commands.json
