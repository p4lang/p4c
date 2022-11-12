iwyu_mappings := gcc.libc.imp libcxx.imp stl.c.headers.imp stl.c.headers.imp boost-1.75-all.imp

all:  $(iwyu_mappings)

gcc.libc.imp:
	curl -s https://raw.githubusercontent.com/include-what-you-use/include-what-you-use/master/gcc.libc.imp -o $@

libcxx.imp:
	curl -s https://raw.githubusercontent.com/include-what-you-use/include-what-you-use/master/libcxx.imp -o $@

stl.c.headers.imp:
	curl -s https://raw.githubusercontent.com/include-what-you-use/include-what-you-use/master/stl.c.headers.imp -o $@

boost-all.imp:
	curl -s https://raw.githubusercontent.com/include-what-you-use/include-what-you-use/master/boost-all.imp -o $@

boost-1.75-all.imp:
	curl -s https://raw.githubusercontent.com/include-what-you-use/include-what-you-use/master/boost-1.75-all.imp -o $@

clean:
	rm -f $(iwyu_mappings)

refresh: clean all

.PHONY: all clean refresh
