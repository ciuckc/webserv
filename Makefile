NAME = webserv

BUILD_DIR = build/

all: $(NAME)

$(BUILD_DIR):
	mkdir -p $@

$(NAME): $(BUILD_DIR)
	cmake -B $(BUILD_DIR) -S .
	cmake --build build --target all
	ln -s build/compile_commands.json compile_commands.json

clean:
	rm -rf $(BUILD_DIR)

fclean: clean
	rm -rf $(NAME)
	rm -rf compile_commands.json

re: fclean
	$(MAKE)

.PHONY: all clean fclean re
