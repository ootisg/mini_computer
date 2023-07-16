build_emulator: emulator/emulator.c emulator/emulator.h build_io build_memory
	gcc emulator/emulator.c emulator/emulator.h -o a.out
build_io: io/io.c io/io.h
	gcc -c io/io.c io/io.h
build_memory: memory/memory.c memory/memory.h
	gcc -c memory/memory.c memory/memory.h
build_assembler: assembler/assembler.c assembler/assembler.h
	gcc -c assembler/assembler.c assembler/assembler.h
