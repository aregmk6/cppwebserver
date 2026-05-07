srcfiles := src/main.cpp
interfaces := src/parser.h

.PHONY: server
server: build/server $(interfaces)

build/server: $(srcfiles) 
	mkdir -p $(@D)
	g++ -o build/server src/main.cpp -std=c++20 

debug: build/server_d $(interfaces)

build/server_d: $(srcfiles) 
	mkdir -p $(@D)
	g++ -o build/server_d src/main.cpp -g -std=c++20

clean:
	rm -rf build/ compile_commands.json
