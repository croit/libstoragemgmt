
lsmdir = $(LSM_PYSO_DIR)/lsm

lsm_PYTHON = \
	lsm/__init__.py \
	lsm/_client.py \
	lsm/_common.py \
	lsm/_data.py \
	lsm/_transport.py \
	lsm/version.py \
	lsm/_iplugin.py \
	lsm/_local_disk.py \
	lsm/_pluginrunner.py

if WITH_PYTHON3
_PY_CLIB_INIT_NAME = "PyInit__clib"
else
_PY_CLIB_INIT_NAME = "init_clib"
endif

pyso_LTLIBRARIES = lsm/_clib.la
pysodir = $(LSM_PYSO_DIR)/lsm
lsm__clib_la_CFLAGS = $(PYTHON_CFLAGS) -I$(top_srcdir)/c_binding/include \
		      -Wno-cast-function-type
lsm__clib_la_SOURCES = lsm/_clib.c
lsm__clib_la_LDFLAGS = $(PYTHON_LIBS) \
		       -module -avoid-version -export-symbols-regex \
		       $(_PY_CLIB_INIT_NAME)
lsm__clib_la_LIBADD = $(top_builddir)/c_binding/libstoragemgmt.la
