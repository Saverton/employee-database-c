TARGET = bin/dbview
SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))

run: clean default
	./$(TARGET) -f ./mynewfile.db -n
	./$(TARGET) -f ./mynewfile.db -a "John Meadows,980 Sills Mill Rd.,50"
	./$(TARGET) -f ./mynewfile.db -a "Ginger Meyer,1021 Parkerville Rd.,35" -l
	./$(TARGET) -f ./mynewfile.db -r "John Meadows" -l

default: $(TARGET)

clean:
	rm -rf obj/*.o
	rm -rf bin/*
	rm -f *.db

$(TARGET): $(OBJ)
	gcc -o $@ $?

obj/%.o : src/%.c
	gcc -c $< -o $@ -Iinclude
