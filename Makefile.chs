all: install datainstall

install: .install
	echo "calling make install"
	@make -C ../../.. all install

.install: *.cpp *.h
	touch .install

datainstall: .datainstall
	echo "calling make datainstall"
	@make -C ../../.. datainstall

.datainstall: *.xml *.rgb
	touch .datainstall

