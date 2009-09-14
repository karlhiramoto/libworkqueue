libtoolize --copy --force --automake
aclocal --force && \
automake --gnu --copy --foreign  --include-deps --add-missing && \
autoconf
autoheader --force


