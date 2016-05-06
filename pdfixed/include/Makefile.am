ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

pdf_headers = \
bm/pdfixed/pd_pre.h \
bm/pdfixed/pd_mirroring.h \
bm/pdfixed/pd_static.h \
bm/pdfixed/pd_common.h \
bm/pdfixed/pd_swap.h

# these are needed when compiling the P4-dependent PD
pdf_int_headers = \
bm/pdfixed/int/pd_conn_mgr.h \
bm/pdfixed/int/pd_helpers.h \
bm/pdfixed/int/pd_notifications.h

nobase_include_HEADERS = $(pdf_headers) $(pdf_int_headers)
