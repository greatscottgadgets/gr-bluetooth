NAME	= libbs
CFLAGS	= -Wall -g -fPIC -MD
OBJS	= bs.o
LDFLAGS	= -shared

all: $(NAME).so $(NAME).a test

$(NAME).so: $(OBJS)
	$(CC) $(CFLAGS) -o $(@) $(<) $(LDFLAGS)

$(NAME).a: $(OBJS)
	ar cru $(@) *.o
	ranlib $(@)

test: $(NAME).so test.o
	$(CC) $(CFLAGS) -o $(@) test.o $(NAME).a

clean:
	rm -f $(NAME).so $(NAME).a test *.o *.d

-include *.d
