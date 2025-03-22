mainTargets = main.o LargeNumbers.o
FLAGS := -Wall -g -Os -pedantic

go: $(mainTargets)
	gcc $(mainTargets) -o go $(FLAGS)

main.o: main.c
	gcc -c main.c $(FLAGS)

LargeNumbers.o: LargeNumbers.c
	gcc -c LargeNumbers.c $(FLAGS)

clean:
	rm ./go *.o -f
