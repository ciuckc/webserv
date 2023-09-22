NAME = build

all: $(NAME)

$(NAME):
	cmake -B build -S .
	make -C build

clean:
	make clean -C build
	rm -rf build

re: clean
	$(MAKE)

.PHONY: re clean build
