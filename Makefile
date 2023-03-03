all:  cxxproject
	
cxxproject: cxxproject.cpp Makefile
	$(CXX) -Wall -Wextra -O2 cxxproject.cpp -o $@
	
clean:
	rm -f cxxproject
	
install: cxxproject
	install -D cxxproject $(DESTDIR)/usr/local/bin
	

