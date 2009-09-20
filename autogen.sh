libtoolize --copy --force --automake
aclocal --force -I m4 && \
autoheader --force
automake --gnu --copy --foreign  --include-deps --add-missing && \
autoconf


