include $(NJS)/nxt/Makefile.conf

PARSERSRCS= njs-vm.c
PARSEROBJS= $(subst .c,.o,$(PARSERSRCS))

njs-vm.o: njs-vm.c
	$(CC) $(CFLAGS) -I${NJS}/nxt -I${NJS}/njs \
	-I/usr/local/include -Wall -Werror -O0 -g -c $< -o $@

njs-vm: $(PARSEROBJS)
	$(CC) $(CFLAGS) -L${NJS}/build \
$(PARSEROBJS) -o $@ -lnxt -lnjs -lm $(NXT_PCRE_CFLAGS) $(NXT_PCRE_LIB)

clean: 
	rm njs-vm.o
