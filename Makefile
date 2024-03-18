#Makefile

.PHONY: build_debug build_release run test install clean

build_debug:
	@cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
	@cp ./build/compile_commands.json .
	@cmake --build build
	@./build/usic_tests

build_release:
	@cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
	@cp ./build/compile_commands.json .
	@cmake --build build

run:
	@./build/usic

test:
	@./build/usic_tests

install:
	@install -m 755 ./build/usic /usr/local/bin
	@echo "\033[32music has been installed into /usr/local/bin\033[0m"

with_script:
	@cp ./scripts/usic_play_list.sh /usr/local/bin/usic_play_list
	@cp ./scripts/usic_add_to_list.sh /usr/local/bin/usic_add_to_list
	@cp ./scripts/usic_create_default_playList.sh /usr/local/bin/usic_create_default_playList
	@cp ./scripts/usic_remove_from_list.sh /usr/local/bin/usic_remove_from_list
	@echo "\033[32mScripts have been installed into /usr/local/bin\033[0m"

clean:
	@rm -rf build
	@rm -rf compile_commands.json
