
SPEC=P4Runtime-Spec

all: html pdf build

build:
	mkdir -p build

# Note: Brute-force image rendering; could use more precise file-by-file make rules
images: build assets/*.odg
	soffice --convert-to svg --outdir build assets/*.odg > /dev/null 2>&1
	soffice --convert-to png --outdir build assets/*.odg > /dev/null 2>&1

html: ${SPEC}.mdk p4.json assets/* images
	madoko -vv --png --odir=build ${SPEC}.mdk

pdf: ${SPEC}.mdk p4.json assets/* images
	madoko --pdf -vv --png --odir=build $<

clean:
	${RM} -rf build
