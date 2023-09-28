NAME = webserv

all: $(NAME)

$(NAME):
	cmake -B build -S .
	make -C build -j

clean:
	rm -rf ./build/CMakeFiles/webserv.dir/

fclean:
	make clean -C build
	rm -rf build

re: fclean
	$(MAKE)

.PHONY: re fclean clean build $(NAME)
