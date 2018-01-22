MPICXX = mpic++
CXXFLAGS  = -Wall -g 
LDFLAGS = -pthread

OBJ = main.o roles.o functions.o

tdomp: $(OBJ)
	$(MPICXX) $(CXXFLAGS) -o tdomp  $(OBJ) $(LDFLAGS)

#currently a dirty fix for header files. If a header is touched all files
# are compiled again.
%.o: %.cpp *.h
	$(MPICXX) -g -c $<
