.PHONY: all test clean example

all: test

json_str_test:
json_str_test: json_str_test.c json_str.c include/json_str.h
	$(CC) -Wall -Wextra -o $@ json_str_test.c json_str.c

test: json_str_test
	./json_str_test 0 | tee /dev/stderr | python -mjson.tool
	./json_str_test 1 | tee /dev/stderr | python -mjson.tool
	./json_str_test 2 | tee /dev/stderr | python -mjson.tool
	./json_str_test 3 | tee /dev/stderr | python -mjson.tool
	./json_str_test 4 | tee /dev/stderr | python -mjson.tool
	./json_str_test 5 | tee /dev/stderr | python -mjson.tool
	./json_str_test 6 | tee /dev/stderr | python -mjson.tool
	./json_str_test 7 | tee /dev/stderr | python -mjson.tool
	./json_str_test 8 | tee /dev/stderr | python -mjson.tool
	./json_str_test 9 | tee /dev/stderr | python -mjson.tool
	./json_str_test 10 | tee /dev/stderr | python -mjson.tool
	./json_str_test 11 | tee /dev/stderr | python -mjson.tool
	./json_str_test 12 | tee /dev/stderr | python -mjson.tool
	./json_str_test 13 | tee /dev/stderr | python -mjson.tool
	./json_str_test 14 | tee /dev/stderr | python -mjson.tool
	./json_str_test 15 | tee /dev/stderr | python -mjson.tool
	./json_str_test 16 | tee /dev/stderr | python -mjson.tool
	./json_str_test 17 | tee /dev/stderr | python -mjson.tool
	./json_str_test 18 | tee /dev/stderr | python -mjson.tool
	./json_str_test 19 | tee /dev/stderr | python -mjson.tool
	./json_str_test 20 | tee /dev/stderr | python -mjson.tool
	./json_str_test 21 | tee /dev/stderr | python -mjson.tool
	./json_str_test 22 | tee /dev/stderr | python -mjson.tool
	./json_str_test 23 | tee /dev/stderr | python -mjson.tool
	./json_str_test 24 | tee /dev/stderr | python -mjson.tool
	./json_str_test 25 | tee /dev/stderr | python -mjson.tool
	./json_str_test 26 | tee /dev/stderr | python -mjson.tool
	./json_str_test 27 | tee /dev/stderr | python -mjson.tool
	./json_str_test 28 | tee /dev/stderr | python -mjson.tool

example: example.c json_str.c include/json_str.h
	$(CC) -Wall -Wextra -o $@ $^
	./$@

clean:
	rm -vf example json_str_test
