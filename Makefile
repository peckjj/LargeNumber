mainTargets = main.o LargeNumbers.o
FLAGS := -Wall -g -Os -pedantic

go: $(mainTargets)
	gcc $(mainTargets) -o go $(FLAGS)

main.o: main.c
	gcc -c main.c $(FLAGS)

LargeNumbers.o: LargeNumbers.c
	gcc -c LargeNumbers.c $(FLAGS)

test:
	make clean
	make
	echo "expect 680564733841876926926749214863536422911"
	./go 1ffffffffffffffff 36893488147419103231

clean:
	rm ./go *.o -f
