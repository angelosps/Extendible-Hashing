INCLUDE_PATH = ./include/
LIBRARY_PATH = ./lib/

P_OBJS = ./build/primary_hash_main.o ./build/primary_hash_file.o ./build/common.o
P_SOURCE = ./src/primary_hash_main.c ./src/primary_hash_file.c ./src/common.c
P_OUT = ./build/bin/primary
S_OBJS = ./build/secondary_hash_main.o ./build/primary_hash_file.o ./build/secondary_hash_file.o ./build/common.o
S_SOURCE = ./src/secondary_hash_main.c ./src/primary_hash_file.c ./src/secondary_hash_file.c ./src/common.c
S_OUT = ./build/bin/secondary

DB = *.db

primary: $(P_OBJS)
		gcc -L $(LIBRARY_PATH) -Wl,-rpath,$(LIBRARY_PATH) $(P_OBJS) -lbf -o $(P_OUT) -O2	

secondary: $(S_OBJS)
		gcc -L $(LIBRARY_PATH) -Wl,-rpath,$(LIBRARY_PATH) $(S_OBJS) -lbf -o $(S_OUT) -O2	

./build/primary_hash_main.o: ./src/primary_hash_main.c ./include/primary_hash_file.h
		gcc -I $(INCLUDE_PATH) ./src/primary_hash_main.c -c -o $@

./build/secondary_hash_main.o: ./src/secondary_hash_main.c ./include/primary_hash_file.h ./include/secondary_hash_file.h
		gcc -I $(INCLUDE_PATH) ./src/secondary_hash_main.c -c -o $@

./build/primary_hash_file.o: ./src/primary_hash_file.c ./include/primary_hash_file.h
		gcc -I $(INCLUDE_PATH) ./src/primary_hash_file.c -c -o $@

./build/secondary_hash_file.o: ./src/secondary_hash_file.c ./include/primary_hash_file.h ./include/secondary_hash_file.h
		gcc -I $(INCLUDE_PATH) ./src/secondary_hash_file.c -c -o $@

./build/common.o: ./src/common.c ./include/common.h
		gcc -I $(INCLUDE_PATH) ./src/common.c -c -o $@

runp:
	@echo "Removing previous db files..."
	rm -f $(DB)
	@echo "Running primary extendible hashing..."
	./build/bin/primary

runs:
	@echo "Removing previous db files..."
	rm -f $(DB)
	@echo "Running secondary extendible hashing..."
	./build/bin/secondary

# clean house
clean:
	@echo "Cleaning..."
	rm -f $(P_OBJS) $(S_OBJS) $(P_OUT) $(S_OUT) $(DB)