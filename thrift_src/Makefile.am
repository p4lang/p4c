BUILT_SOURCES = gen-cpp/runtime_constants.cpp gen-cpp/runtime_constants.h gen-cpp/Runtime.cpp gen-cpp/Runtime.h gen-cpp/runtime_types.cpp gen-cpp/runtime_types.h

AUTOMAKE_OPTIONS = foreign no-dependencies subdir-objects

noinst_LIBRARIES = libruntimestubs.a
libruntimestubs_a_SOURCES = \
	${BUILT_SOURCES}

${BUILT_SOURCES} : runtime.thrift
	$(THRIFT) --gen cpp -r $<
	$(THRIFT) --gen py -r $<

clean-local:
	rm -rf gen-cpp gen-py
