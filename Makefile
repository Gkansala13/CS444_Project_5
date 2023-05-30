mkfs: mkfs.o simfs.a
	gcc -Wall -Wextra -o $@ $^ 

mkfs.o: mkfs.c
	gcc -Wall -Wextra -c $<

simfs.a: block.o free.o inode.o image.o mkfs.o
	ar rcs $@ $^

image.o: image.c
	gcc -Wall -Wextra -c $<

block.o: block.c
	gcc -Wall -Wextra -c $<

free.o: free.c
	gcc -Wall -Wextra -c $<

inode.o: inode.c
	gcc -Wall -Wextra -c $<

pack.o: pack.c
	gcc -Wall -Wextra -c $<

ls.o: ls.c
	gcc -Wall -Wextra -c $<

simfs_test: simfs_test.o simfs.a
	gcc -Wall -Wextra -o $@ $^

simfs_test.o: simfs_test.c
	gcc -Wall -Wextra -c $< -DCTEST_ENABLE

.PHONY: test

test: simfs_test
	./simfs_test

clean: 
	rm -f *.o
