srcfiles := src/main.cpp
interfaces := src/parser.h

.PHONY: server
server: build/server $(interfaces)

build/server: $(srcfiles) 
	mkdir -p $(@D)
	g++ -o build/server src/main.cpp

debug: build/server_d $(interfaces)

build/server_d: $(srcfiles) 
	mkdir -p $(@D)
	g++ -o build/server_d src/main.cpp -g

clean:
	rm -rf build/ compile_commands.json
