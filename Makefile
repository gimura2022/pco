NAME = pco

POSTFIX ?= usr/local
DESTDIR ?= /

.PHONY: all
all: $(NAME).h lib$(NAME).a lib$(NAME).so

$(NAME).h: $(NAME).prt.h $(NAME).c
	> $(NAME).h
	echo "#ifndef _pco_h" >> $(NAME).h
	echo "#define _pco_h" >> $(NAME).h
	echo "" >> $(NAME).h
	cat $(NAME).prt.h >> $(NAME).h
	echo "" >> $(NAME).h
	echo "#ifdef PCO_IMPLEMENTATION" >> $(NAME).h
	echo "" >> $(NAME).h
	cat $(NAME).c >> $(NAME).h
	echo "" >> $(NAME).h
	echo "#endif" >> $(NAME).h
	echo "#endif" >> $(NAME).h

lib$(NAME).a: $(NAME).o
	$(AR) rcs lib$(NAME).a $(NAME).o

lib$(NAME).so: $(NAME).c
	$(CC) -fpic -shared -o lib$(NAME).so $(NAME).c

.PHONY: clean
clean:
	$(RM) lib$(NAME).so
	$(RM) lib$(NAME).a
	$(RM) *.o
	$(RM) $(NAME).h
	$(RM) README

.PHONY: install
install: all
	install -d $(DESTDIR)$(POSTFIX)/lib/
	install -d $(DESTDIR)$(POSTFIX)/include/
	install -m 775 lib$(NAME).so $(DESTDIR)$(POSTFIX)/lib/
	install -m 644 lib$(NAME).a $(DESTDIR)$(POSTFIX)/lib/
	install -m 644 $(NAME).h $(DESTDIR)$(POSTFIX)/include/

.PHONY: uninstall
uninstall:
	$(RM) $(DESTDIR)$(POSTFIX)/lib/lib$(NAME).a
	$(RM) $(DESTDIR)$(POSTFIX)/lib/lib$(NAME).so

README: $(NAME).1
	mandoc -mdoc -T ascii $(NAME).1 | col -b > README
