#Makefile

PROJECT_NAME = usic

.PHONY: build_debug build_release run test install clean

build_debug:
	@cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
	@cp ./build/compile_commands.json .
	@cmake --build build

build_release:
	@cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
	@cp ./build/compile_commands.json .
	@cmake --build build

run:
	@./build/${PROJECT_NAME}

test:
	@cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
	@cmake --build build --target ${PROJECT_NAME}_tests
	@./build/tests/${PROJECT_NAME}_tests

install:
	@install -m 755 ./build/${PROJECT_NAME} /usr/local/bin
	@echo "\033[32m${PROJECT_NAME} has been installed into /usr/local/bin\033[0m"

with_script:
	@cp ./scripts/usic_play_list.sh /usr/local/bin/usic_play_list
	@cp ./scripts/usic_add_to_list.sh /usr/local/bin/usic_add_to_list
	@cp ./scripts/usic_create_default_playList.sh /usr/local/bin/usic_create_default_playList
	@cp ./scripts/usic_remove_from_list.sh /usr/local/bin/usic_remove_from_list
	@echo "\033[32mScripts have been installed into /usr/local/bin\033[0m"

clean:
	@rm -rf build
	@rm -rf compile_commands.json
