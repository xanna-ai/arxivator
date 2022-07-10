all:
	gcc -o new_archivator -fsanitize=address ./new_archivator.c
arch:
	./new_archivator -a dir dir.arch
disarch:
	./new_archivator -d dir.arch dir1
check:
	cppcheck ./new_archivator.c
