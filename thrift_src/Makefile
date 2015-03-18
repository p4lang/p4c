runtime:
	thrift -r --gen cpp runtime.thrift
	thrift -r --gen py runtime.thrift

clean:
	rm -rf gen-cpp
	rm -rf gen-py

copy_cpp:
	cp gen-cpp/*.cpp ../modules/bm_runtime/src/
	cp gen-cpp/*.h ../modules/bm_runtime/src/

.PHONY: runtime clean copy_cpp
