all:
	( cd lib ; $(MAKE) all )
	( cd bin ; $(MAKE) all )

clean:
	rm -f *~
	( cd lib ; $(MAKE) clean )
	( cd bin ; $(MAKE) clean )
	( cd test ; $(MAKE) clean )

test: all
	(cd test ; $(MAKE) test )
