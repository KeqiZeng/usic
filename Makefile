#Makefile

PROJECT_NAME = usic

.PHONY: run release test install clean

run:
	@cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
	@cmake --build build && echo "\n[32m========== output ==========[0m\n" && ./build/${PROJECT_NAME}

release:
	@cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
	@ln -s ./build/compile_commands.json ./compile_commands.json
	@cmake --build build

test:
	@cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
	@./build/tests/${PROJECT_NAME}_tests

install:
	@install -m 755 ./build/${PROJECT_NAME} /usr/local/bin
	@echo "\033[32m${PROJECT_NAME} has been installed into /usr/local/bin\033[0m"

uninstall:
	@rm -f /usr/local/bin/${PROJECT_NAME}
	@echo "\033[32m{PROJECT_NAME} has been uninstalled from /usr/local/bin\033[0m"

# with_script:
# 	@cp ./scripts/usic_play_list.sh /usr/local/bin/usic_play_list
# 	@cp ./scripts/usic_add_to_list.sh /usr/local/bin/usic_add_to_list
# 	@cp ./scripts/usic_create_default_playList.sh /usr/local/bin/usic_create_default_playList
# 	@cp ./scripts/usic_remove_from_list.sh /usr/local/bin/usic_remove_from_list
# 	@echo "\033[32mScripts have been installed into /usr/local/bin\033[0m"

clean:
	@rm -rf build
	@rm -rf compile_commands.json
