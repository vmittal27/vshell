shell: vshell.c str_utils.c str_utils.h
	gcc -Wall -Werror -o vsh vshell.c str_utils.c

clean:
	rm -f vsh *~
