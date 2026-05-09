srcfiles := src/main.cpp
interfaces := src/parser.h src/thread_pool.h

.PHONY: server
server: build/server 

build/server: $(srcfiles) $(interfaces)
	mkdir -p $(@D)
	g++ -o build/server $(srcfiles) -std=c++20 -Wall -pedantic

debug: build/server_d 

build/server_d: $(srcfiles) $(interfaces)
	mkdir -p $(@D)
	g++ -o build/server_d $(srcfiles) -g -std=c++20 -Wall -pedantic

clean:
	rm -rf build/ compile_commands.json
