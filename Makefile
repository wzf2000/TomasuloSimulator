inputs := $(patsubst sample/%.asm,input/%.in,$(wildcard sample/*.asm))
TEST ?= 1

input: $(inputs)

simulate: test
	python simulator.py -i output/output$(TEST).out

test: input/input$(TEST).in main
	mkdir -p output
	./main $< > output/output$(TEST).out

main: main.cpp defines.hpp utils.hpp machinestate.hpp
	g++ main.cpp -o main -std=c++11

input/input%.in: sample/input%.asm simulator.py
	mkdir -p input
	python assembler.py -i $< -o $@

clean:
	rm *.in *.out main

.PHONY: clean input test simulate
