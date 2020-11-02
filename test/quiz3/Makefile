TARGET=quiz3
include ../shared/Makefile.shared

$(TARGET): $(TARGET).o
	$(CC) $(TARGET).o -lm $(SNIPER_LDFLAGS) -o $(TARGET)

run_$(TARGET):
	../../run-sniper -n 1 -c quiz3_nehalem.cfg --roi -- ./quiz3

CLEAN_EXTRA=viz
