TARGET = bin/dbview
SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))

run: clean default
	./$(TARGET) -f ./mynewfile.db -n
	xxd ./mynewfile.db
	./$(TARGET) -f ./mynewfile.db

default: $(TARGET)

clean:
	rm -rf obj/*.o
	rm -rf bin/*
	rm -f *.db

$(TARGET): $(OBJ)
	gcc -o $@ $?

obj/%.o : src/%.c
	gcc -c $< -o $@ -Iinclude
