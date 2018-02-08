MPICXX = mpic++
CXXFLAGS = -Wall -g 
LDFLAGS = -pthread 

OBJ = main.o Runtime.o Worker.o RuntimeTask.o

tdomp: $(OBJ)
	$(MPICXX) $(CXXFLAGS) -o tdomp  $(OBJ) $(LDFLAGS)

#currently a dirty fix for header files. If a header is touched all files
# are compiled again.
%.o: %.cpp *.h
	$(MPICXX) $(CXXFLAGS)  -c $<

.PHONY: clean
clean:
	rm -rf tdomp $(OBJ)
