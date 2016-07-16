OBJS := COPYING.o main.o atom.o embed.o list.o parse.o

CFLAGS := -O2 -flto -Wall -pedantic
LDFLAGS := -fwhole-program -flto

lisp: $(OBJS)
	$(LINK.o) $(OBJS) -lm $(OUTPUT_OPTION)

%.o: %.TXT
	$(LD) -r -b binary $< $(OUTPUT_OPTION)

clean:
	rm -f lisp $(OBJS)
