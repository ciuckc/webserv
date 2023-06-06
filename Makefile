NAME = webserv

BUILD_DIR = build

COMP_COM = compile_commands.json

all: $(NAME)

$(BUILD_DIR):
	mkdir -p $@

$(NAME): $(BUILD_DIR)
	cmake -B $(BUILD_DIR) -S .
	cmake --build $(BUILD_DIR)
	ln -s $(BUILD_DIR)/$(COMP_COM) $(COMP_COM)

compile: $(NAME) $(BUILD_DIR)
	cmake --build $(BUILD_DIR) --target $(NAME)

clean:
	rm -rf $(BUILD_DIR) $(COMP_COM)


fclean: clean
	rm -rf $(NAME)

re: fclean all

.PHONY: all clean fclean re compile
