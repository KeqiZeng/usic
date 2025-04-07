#Makefile

PROJECT_NAME = usic

.PHONY: run release test install clean

run:
	@cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
	@cp ./build/compile_commands.json .
	@cmake --build build && echo "\n[32m========== output ==========[0m\n" && ./build/${PROJECT_NAME}

release:
	@cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
	@cp ./build/compile_commands.json .
	@cmake --build build

test:
	@cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
	@cmake --build build --target ${PROJECT_NAME}_tests
	@./build/tests/${PROJECT_NAME}_tests

install:
	@install -m 755 ./build/${PROJECT_NAME} /usr/local/bin
	@echo "\033[32m${PROJECT_NAME} has been installed into /usr/local/bin\033[0m"

uninstall:
	@rm -f /usr/local/bin/${PROJECT_NAME}
	@echo "\033[32m${PROJECT_NAME} has been uninstalled from /usr/local/bin\033[0m"

clean:
	@rm -rf build
	@rm -rf compile_commands.json
