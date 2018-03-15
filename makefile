FLAGS=-pthread -Wall -Wextra -Werror -g -std=c11

main: main.c
	gcc $(FLAGS) $^ -o $@

clean:
	rm -vf main
