all:
	@echo "make doc   :  Make Doxygen documentation"
	@echo "make check :  Make and run unit tests (incomplete)"

check:
	make -C targets/utests/modtest

doc:
	doxygen

clean:
	@rm -rf doc


.PHONY: doc
