libtoolize --copy --force --automake
aclocal --force && \
autoheader --force
automake --gnu --copy --foreign  --include-deps --add-missing && \
autoconf


